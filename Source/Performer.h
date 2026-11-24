// Psaltikon — performs a Timeline (or live MIDI) with the chant voice, the ison choir
// and church reverb. Used both in real time and for offline export.
#pragma once
#include "Composition.h"
#include "VocalEngine.h"

namespace byz
{
enum IsonMode { IsonOff = 0, IsonScore, IsonManual, IsonLive };

struct PerformParams
{
    VoiceParams voice;
    float melodyGain = 0.8f, isonGain = 0.5f, reverbWet = 0.3f, articulation = 0.5f;
    int isonMode = IsonScore, isonVoices = 2, isonVowel = 3, isonOctave = 0, isonManualDeg = 1;
    int genusForManual = Diatonic;
    bool droneOn = false;
    bool renderMelody = true, renderIson = true;
};

class Performer
{
public:
    void prepare (double sampleRate, int maxBlock)
    {
        sr = sampleRate;
        tables.build (sampleRate);
        melody.prepare (sampleRate, &tables, 99u);
        choir.prepare (sampleRate, &tables);
        choir.reserve (kChunk * 2);
        mono.assign ((size_t) std::max (maxBlock, kChunk) + 64, 0.0f);
        reverb.setSampleRate (sampleRate);
        reverb.reset();
        applyParams();
    }

    void setParams (const PerformParams& p)
    {
        const bool droneChanged = p.droneOn != prm.droneOn || p.isonManualDeg != prm.isonManualDeg || p.isonMode != prm.isonMode
                                  || p.isonOctave != prm.isonOctave || p.genusForManual != prm.genusForManual;
        prm = p;
        applyParams();
        if (droneChanged) updateDrone();
    }

    void play (std::shared_ptr<const Timeline> t, double fromBeat)
    {
        tl = std::move (t);
        playing = tl != nullptr;
        if (! playing) return;
        seek (fromBeat);
    }
    void stop()
    {
        playing = false;
        melody.setLevel (0, 0.12f);
        if (prm.isonMode == IsonScore) choir.stop (0.3f);
    }
    bool isPlaying() const { return playing; }
    double beat() const { return curBeat; }
    std::shared_ptr<const Timeline> timeline() const { return tl; }
    void setTempoOverride (double bpm) { tempoOverride = bpm; }

    // ---- live MIDI ----
    void liveNoteOn (float moria, float velocity, int vowel)
    {
        const bool legato = liveHeld > 0;
        ++liveHeld;
        melody.setPitch (moria, legato ? 0.06f : 0.0f);
        if (! legato) { melody.setVowel (vowel, 0.001f); melody.articulate (3, 0.5f); }
        melody.setLevel (0.55f + 0.45f * velocity, legato ? 0.05f : 0.04f);
        melody.vibrato (1.0f, 2.0f);
    }
    void liveNoteOff (bool allReleased)
    {
        liveHeld = std::max (0, liveHeld - 1);
        if (allReleased) { liveHeld = 0; melody.setLevel (0, 0.18f); }
    }
    void liveIson (float moria) { liveIsonM = moria; choir.start (moria + 72.0f * prm.isonOctave); }
    void liveIsonOff() { liveIsonM = -1e9f; choir.stop(); }

    void process (float* L, float* R, int n, double hostBeat = -1.0, double hostBpm = 0.0)
    {
        if (hostBeat >= 0 && playing && tl && std::abs (hostBeat - curBeat) > 0.05) seek (hostBeat);
        int done = 0;
        while (done < n)
        {
            const int c = std::min (kChunk, n - done);
            if (playing && tl) runControls();
            std::fill (mono.begin(), mono.begin() + c, 0.0f);
            if (prm.renderMelody) melody.render (mono.data(), c);
            for (int s = 0; s < c; ++s) { L[done + s] = mono[(size_t) s] * prm.melodyGain; R[done + s] = mono[(size_t) s] * prm.melodyGain; }
            if (prm.renderIson && prm.isonMode != IsonOff) choir.render (L + done, R + done, c, prm.isonGain);
            if (playing && tl)
            {
                const double bpm = hostBpm > 0 ? hostBpm : (tempoOverride > 0 ? tempoOverride : tl->bpm);
                curBeat += c / sr * bpm / 60.0;
                if (curBeat > tl->total + 0.05)
                {
                    playing = false;
                    melody.setLevel (0, 0.35f);
                    if (prm.isonMode == IsonScore) choir.stop (0.8f);
                }
            }
            done += c;
        }
        reverb.processStereo (L, R, n);
        for (int s = 0; s < n; ++s) { L[s] = std::tanh (L[s]); R[s] = std::tanh (R[s]); }
    }

    // current score event (for the cursor), -1 if none
    int currentEvent() const
    {
        if (! playing || ! tl) return -1;
        const auto& st = tl->eventStart;
        auto it = std::upper_bound (st.begin(), st.end(), curBeat);
        return (int) (it - st.begin()) - 1;
    }

    bool tailActive() const { return melody.isSounding() || choir.sounding(); }

private:
    static constexpr int kChunk = 32;

    void applyParams()
    {
        melody.setParams (prm.voice);
        choir.setParams (prm.voice, prm.isonVoices, prm.isonVowel);
        juce::Reverb::Parameters rp;
        rp.roomSize = 0.92f; rp.damping = 0.42f; rp.width = 1.0f;
        rp.wetLevel = prm.reverbWet * 0.6f; rp.dryLevel = 1.0f - prm.reverbWet * 0.45f;
        reverb.setParameters (rp);
    }

    void updateDrone()
    {
        if (prm.isonMode == IsonManual && prm.droneOn)
            choir.start ((float) moriaOf (prm.isonManualDeg, prm.genusForManual) + 72.0f * prm.isonOctave);
        else if (prm.isonMode == IsonManual || prm.isonMode == IsonOff) choir.stop();
        else if (prm.isonMode == IsonScore && ! playing) choir.stop();
    }

    void seek (double b)
    {
        curBeat = std::max (0.0, b);
        const auto& cs = tl->ctrls;
        ci = (size_t) (std::lower_bound (cs.begin(), cs.end(), curBeat, [] (const Ctrl& c, double v) { return c.beat < v; }) - cs.begin());
        // state at the seek point
        int pitchIdx = -1, vowelIdx = -1;
        for (size_t k = 0; k < ci; ++k) { if (cs[k].kind == CPitch) pitchIdx = (int) k; if (cs[k].kind == CVowel) vowelIdx = (int) k; }
        melody.silence();
        if (pitchIdx >= 0 && curBeat < tl->total)
        {
            melody.setPitch (cs[(size_t) pitchIdx].a, 0);
            if (vowelIdx >= 0) melody.setVowel ((int) cs[(size_t) vowelIdx].a, 0.001f);
            melody.setLevel (1, 0.05f);
        }
        ii = 0;
        while (ii < tl->ison.size() && tl->ison[ii].first <= curBeat + 1e-9) ++ii;
        if (prm.isonMode == IsonScore && ! tl->ison.empty())
            choir.start ((float) tl->ison[ii > 0 ? ii - 1 : 0].second + 72.0f * prm.isonOctave);
    }

    void runControls()
    {
        const auto& cs = tl->ctrls;
        while (ci < cs.size() && cs[ci].beat <= curBeat + 1e-9)
        {
            const auto& c = cs[ci++];
            switch (c.kind)
            {
                case CPitch:   melody.setPitch (c.a, c.b); break;
                case CVowel:   melody.setVowel ((int) c.a); break;
                case CArtic:   melody.articulate ((int) c.a, c.b); break;
                case CAccent:  melody.accent (c.a, c.b); break;
                case CLevel:   melody.setLevel (c.a, c.b); break;
                case CVibrato: melody.vibrato (c.a, c.b); break;
                case CBreath:  melody.breathIn(); break;
                default: break;
            }
        }
        if (prm.isonMode == IsonScore)
            while (ii < tl->ison.size() && tl->ison[ii].first <= curBeat + 1e-9)
                choir.start ((float) tl->ison[ii++].second + 72.0f * prm.isonOctave);
    }

    double sr = 48000, curBeat = 0, tempoOverride = 0;
    GlottalTables tables;
    ChantVoice melody;
    IsonChoir choir;
    juce::Reverb reverb;
    PerformParams prm;
    std::shared_ptr<const Timeline> tl;
    size_t ci = 0, ii = 0;
    bool playing = false;
    int liveHeld = 0;
    float liveIsonM = -1e9f;
    std::vector<float> mono;
};
} // namespace byz
