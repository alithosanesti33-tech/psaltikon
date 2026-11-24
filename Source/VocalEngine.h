// Psaltikon — vocal engine.
// Formant-based vocal modelling: a band-limited glottal source with spectral tilt,
// pitch jitter and shimmer, five vowel formants plus a nasal resonance and the
// singer's formant, aspiration noise, consonant onsets (plosive bursts, fricative
// noise, nasal dips), delayed vibrato, and microtonal pitch glides in moria.
#pragma once
#include <JuceHeader.h>
#include "ByzTheory.h"
#include <atomic>

namespace byz
{
struct VoiceProfile
{
    const char* name;
    float niHz;          // pitch of Ni for this voice
    float formantScale;  // vocal tract size
    float brightness;    // 0..1 source tilt
    float vibDepth;      // cents
    float vibRate;       // Hz
    float ring;          // singer's formant
    float breath;        // aspiration
    float nasal;         // nasal resonance (characteristic of the Greek psaltic tone)
};

inline const std::array<VoiceProfile, 5>& voiceProfiles()
{
    static const std::array<VoiceProfile, 5> p {{
        { "Deep resonant bass",          98.0f, 0.92f, 0.35f, 13.0f, 5.0f, 0.35f, 0.05f, 0.10f },
        { "Warm baritone",              110.0f, 1.00f, 0.45f, 15.0f, 5.2f, 0.45f, 0.045f, 0.15f },
        { "Clear tenor",                130.8f, 1.07f, 0.55f, 17.0f, 5.5f, 0.55f, 0.04f, 0.15f },
        { "High-register psaltis",      146.8f, 1.10f, 0.65f,  9.0f, 5.9f, 0.70f, 0.035f, 0.35f },
        { "Restrained ecclesiastical",  116.5f, 1.00f, 0.38f,  5.0f, 4.8f, 0.30f, 0.03f, 0.12f },
    }};
    return p;
}

// Bass-voice formant table (Hz, dB, bandwidth Hz) for a e i o u and a closed hum.
struct VowelSpec { float f[5]; float db[5]; float bw[5]; };
inline const VowelSpec& vowelSpec (int v)
{
    static const VowelSpec t[6] = {
        { { 600, 1040, 2250, 2450, 2750 }, { 0,  -7,  -9,  -9, -20 }, { 60, 70, 110, 120, 130 } }, // a
        { { 400, 1620, 2400, 2800, 3100 }, { 0, -12,  -9, -12, -18 }, { 40, 80, 100, 120, 120 } }, // e
        { { 250, 1750, 2600, 3050, 3340 }, { 0, -30, -16, -22, -28 }, { 60, 90, 100, 120, 120 } }, // i
        { { 400,  750, 2400, 2600, 2900 }, { 0, -11, -21, -20, -40 }, { 40, 80, 100, 120, 120 } }, // o
        { { 350,  600, 2400, 2675, 2950 }, { 0, -20, -32, -28, -36 }, { 40, 80, 100, 120, 120 } }, // u
        { { 250, 1000, 2200, 2800, 3300 }, { 0, -30, -40, -45, -50 }, { 60, 100, 150, 150, 150 } }, // m (hum)
    };
    return t[std::clamp (v, 0, 5)];
}

// Band-limited glottal tables, one per octave so no harmonic folds past Nyquist.
class GlottalTables
{
public:
    static constexpr int kSize = 2048, kLevels = 9;
    void build (double sr)
    {
        if (std::abs (sr - builtFor) < 1.0) return;
        builtFor = sr;
        for (int l = 0; l < kLevels; ++l)
        {
            auto& t = tables[(size_t) l];
            t.assign (kSize + 1, 0.0f);
            const double maxF = 40.0 * std::pow (2.0, l);
            const int N = std::max (1, std::min (400, (int) (0.45 * sr / maxF)));
            for (int h = 1; h <= N; ++h)
            {
                // glottal-flow-derivative-like spectrum with a slight open-quotient ripple
                const double amp = (1.0 / h) * (1.0 + 0.25 * std::cos (h * 0.9));
                const double ph = -0.35 * h;
                for (int j = 0; j < kSize; ++j)
                    t[(size_t) j] += (float) (amp * std::sin (juce::MathConstants<double>::twoPi * h * j / kSize + ph));
            }
            float pk = 0; for (int j = 0; j < kSize; ++j) pk = std::max (pk, std::abs (t[(size_t) j]));
            for (int j = 0; j < kSize; ++j) t[(size_t) j] /= (pk > 0 ? pk : 1.0f);
            t[kSize] = t[0];
        }
    }
    inline float read (double phase, double freq) const
    {
        int l = (int) std::ceil (std::log2 (std::max (freq, 40.0) / 40.0));
        l = std::clamp (l, 0, kLevels - 1);
        const auto& t = tables[(size_t) l];
        const double p = phase * kSize;
        const int i = (int) p;
        const float f = (float) (p - i);
        return t[(size_t) i] + f * (t[(size_t) i + 1] - t[(size_t) i]);
    }
private:
    double builtFor = 0;
    std::array<std::vector<float>, kLevels> tables;
};

struct SVF  // Cytomic TPT state-variable filter, normalised band-pass
{
    float g = 0, k = 1, a1 = 0, a2 = 0, a3 = 0, ic1 = 0, ic2 = 0;
    void set (float fc, float q, float sr)
    {
        fc = std::clamp (fc, 20.0f, 0.45f * sr);
        g = std::tan (juce::MathConstants<float>::pi * fc / sr);
        k = 1.0f / std::max (0.3f, q);
        a1 = 1.0f / (1.0f + g * (g + k)); a2 = g * a1; a3 = g * a2;
    }
    inline float bp (float v0)
    {
        const float v3 = v0 - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2 * v1 - ic1; ic2 = 2 * v2 - ic2;
        return k * v1;
    }
    void reset() { ic1 = ic2 = 0; }
};

struct VoiceParams
{
    float niHz = 110, formantScale = 1, brightness = 0.45f, vibDepth = 15, vibRate = 5.2f;
    float ring = 0.45f, breath = 0.045f, nasal = 0.15f, resonance = 0.5f, intensity = 0.7f;
    float detuneCents = 0;
};

class ChantVoice
{
public:
    void prepare (double sampleRate, const GlottalTables* t, uint32_t seed)
    {
        sr = (float) sampleRate; tables = t; rng.seed (seed);
        for (auto& f : form) f.reset();
        nasalF.reset(); fricHP = 0; tiltLP = 0;
        for (int k = 0; k < 5; ++k) curF[k] = vowelSpec (0).f[k];
        updateFilters();
    }
    void setParams (const VoiceParams& p) { prm = p; }

    void setPitch (float moria, float glideSec)
    {
        tgtM = moria;
        if (glideSec <= 0.0005f || ! active) { curM = moria; }
        glideCoef = glideSec <= 0.0005f ? 1.0f : 1.0f - std::exp (-1.0f / (glideSec * sr / 3.0f));
    }
    void setVowel (int v, float morphSec = 0.045f)
    {
        vowel = std::clamp (v, 0, 5);
        morphCoef = 1.0f - std::exp (-16.0f / (std::max (0.005f, morphSec) * sr));
    }
    void setLevel (float target, float timeSec)
    {
        tgtLvl = target;
        lvlCoef = timeSec <= 0.001f ? 1.0f : 1.0f - std::exp (-1.0f / (timeSec * sr / 3.0f));
        if (target > 0) active = true;
    }
    void articulate (int type, float strength)
    {
        if (type <= 0) return;
        if (type == 1) { burst = 0.9f + 0.5f * strength; dip = 0.25f; }
        else if (type == 2) { fric = 0.55f + 0.4f * strength; fricLen = 0.05f + 0.03f * strength; dip = 0.35f; }
        else dip = 0.45f;
    }
    void accent (float amount, float decaySec) { acc = amount; accCoef = 1.0f - std::exp (-1.0f / (std::max (0.02f, decaySec) * sr)); }
    void vibrato (float scale, float noteSec)
    {
        vibTarget = scale;
        vibDelay = (int) (sr * std::min (0.3f, noteSec * 0.35f));
        vibCur = std::min (vibCur, 0.3f * scale);
    }
    void breathIn() { inhale = 1.0f; }
    void silence() { tgtLvl = 0; lvl = 0; active = false; dip = 1; acc = 1; burst = fric = inhale = 0; }
    bool isSounding() const { return active && (lvl > 1.0e-4f || tgtLvl > 0); }

    // Adds into out (mono). Returns nothing; caller pans.
    void render (float* out, int n)
    {
        if (! active && inhale <= 0.0001f) return;
        const VowelSpec& vs = vowelSpec (vowel);
        const float bright = 0.15f + 0.85f * std::clamp (prm.brightness * (0.6f + 0.6f * prm.intensity), 0.0f, 1.0f);
        const float tiltCoef = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * (900.0f + 5200.0f * bright) / sr);
        const float vibInc = prm.vibRate / sr;
        for (int i = 0; i < n; ++i)
        {
            if (--ctrlCount <= 0)
            {
                ctrlCount = 16;
                for (int k = 0; k < 5; ++k) curF[k] += (vs.f[k] * prm.formantScale - curF[k]) * morphCoef;
                for (int k = 0; k < 5; ++k) curDb[k] += (vs.db[k] - curDb[k]) * morphCoef;
                updateFilters();
                // jitter: slow random walk of a few cents; shimmer: small level wobble
                if (--jitCount <= 0) { jitCount = 24; jitTgt = uni (rng) * 4.0f; shimTgt = uni (rng) * 0.035f; }
                jit += (jitTgt - jit) * 0.08f; shim += (shimTgt - shim) * 0.08f;
                if (vibDelay > 0) vibDelay -= 16; else vibCur += (vibTarget - vibCur) * 0.012f;
            }
            curM += (tgtM - curM) * glideCoef;
            lvl += (tgtLvl - lvl) * lvlCoef;
            dip += (1.0f - dip) * 0.0025f * (48000.0f / sr);
            acc += (1.0f - acc) * accCoef;
            vibPh += vibInc; if (vibPh >= 1) vibPh -= 1;
            const float vibC = std::sin (juce::MathConstants<float>::twoPi * vibPh) * prm.vibDepth * vibCur;
            const double f = prm.niHz * std::pow (2.0, curM / 72.0 + (vibC + jit + prm.detuneCents) / 1200.0);
            phase += f / sr; if (phase >= 1.0) phase -= 1.0;

            float src = tables->read (phase, f);
            tiltLP += (src - tiltLP) * tiltCoef;
            src = tiltLP;
            const float noise = uni (rng);
            const float amp = lvl * dip * acc * (1.0f + shim);
            float exc = src * amp + noise * prm.breath * amp * 0.6f;
            if (burst > 0.001f) { exc += noise * burst * 0.5f; burst *= 0.9965f * (sr > 0 ? 1.0f : 1.0f); if (burst < 0.001f) burst = 0; }

            float y = 0;
            for (int k = 0; k < 5; ++k) y += form[(size_t) k].bp (exc) * gain[k];
            y += nasalF.bp (exc) * prm.nasal * 0.5f;
            y += exc * 0.06f; // a little open-tract energy keeps the low end full

            // fricatives and breaths bypass the formants (high-passed noise)
            float hpIn = 0;
            if (fric > 0.001f) { hpIn += noise * fric; fric *= std::exp (-1.0f / (fricLen * sr)); }
            if (inhale > 0.001f) { hpIn += noise * inhale * 0.06f; inhale *= 0.99985f; }
            fricHP += (hpIn - fricHP) * 0.35f;
            y += (hpIn - fricHP) * 0.35f;

            out[i] += y * 0.55f;
            if (tgtLvl <= 0 && lvl < 1.0e-4f && inhale < 0.001f) active = false;
        }
    }

private:
    void updateFilters()
    {
        const float res = 0.6f + 0.9f * prm.resonance;
        for (int k = 0; k < 5; ++k)
        {
            const VowelSpec& vs = vowelSpec (vowel);
            float bw = vs.bw[k] * prm.formantScale / res;
            form[(size_t) k].set (curF[k], curF[k] / std::max (20.0f, bw), sr);
            float db = curDb[k];
            if (k == 2 || k == 3) db += prm.ring * 9.0f - 2.0f;   // singer's formant
            gain[k] = std::pow (10.0f, db / 20.0f);
        }
        nasalF.set (1900.0f * prm.formantScale, 6.0f, sr);
    }

    float sr = 48000;
    const GlottalTables* tables = nullptr;
    VoiceParams prm;
    std::mt19937 rng { 1 };
    std::uniform_real_distribution<float> uni { -1.0f, 1.0f };
    std::array<SVF, 5> form; SVF nasalF;
    float curF[5] {}, curDb[5] {}, gain[5] {};
    int vowel = 0, ctrlCount = 0, jitCount = 0, vibDelay = 0;
    float curM = 0, tgtM = 0, glideCoef = 1, morphCoef = 0.1f;
    float lvl = 0, tgtLvl = 0, lvlCoef = 0.001f, dip = 1, acc = 1, accCoef = 0.001f;
    float burst = 0, fric = 0, fricLen = 0.06f, inhale = 0, fricHP = 0, tiltLP = 0;
    float jit = 0, jitTgt = 0, shim = 0, shimTgt = 0, vibPh = 0, vibCur = 0, vibTarget = 0;
    double phase = 0;
    bool active = false;
};

// Several human-sounding voices holding the ison, slightly detuned and spread.
class IsonChoir
{
public:
    static constexpr int kMax = 4;
    void prepare (double sampleRate, const GlottalTables* t)
    {
        for (int i = 0; i < kMax; ++i) voices[(size_t) i].prepare (sampleRate, t, 1234u + (uint32_t) i * 77u);
    }
    void setParams (const VoiceParams& base, int count, int vowel)
    {
        n = std::clamp (count, 1, kMax);
        static const float det[] = { 0.0f, 4.5f, -5.0f, 8.0f };
        static const float fs[] = { 1.0f, 0.97f, 1.03f, 0.99f };
        for (int i = 0; i < kMax; ++i)
        {
            auto p = base;
            p.detuneCents = det[i];
            p.formantScale *= fs[i];
            p.vibDepth *= 0.25f;     // the ison is held steady
            p.vibRate *= 0.8f + 0.1f * (float) i;
            p.ring *= 0.6f;
            voices[(size_t) i].setParams (p);
            if (vowel != curVowel) voices[(size_t) i].setVowel (vowel, 0.2f);
        }
        curVowel = vowel;
    }
    void setPitch (float moria, float glideSec) { for (auto& v : voices) v.setPitch (moria, glideSec); }
    void start (float moria)
    {
        for (int i = 0; i < kMax; ++i)
        {
            auto& v = voices[(size_t) i];
            if (! v.isSounding()) { v.setPitch (moria, 0); v.setVowel (curVowel, 0.001f); }
            else v.setPitch (moria, 0.25f);
            v.setLevel (i < n ? 1.0f : 0.0f, 0.25f);
            v.vibrato (1.0f, 10.0f);
        }
    }
    void stop (float t = 0.4f) { for (auto& v : voices) v.setLevel (0, t); }
    void silence() { for (auto& v : voices) v.silence(); }
    bool sounding() const { for (auto& v : voices) if (v.isSounding()) return true; return false; }
    void render (float* L, float* R, int count, float gain)
    {
        static const float pan[] = { 0.0f, -0.45f, 0.45f, -0.15f };
        for (int i = 0; i < kMax; ++i)
        {
            auto& v = voices[(size_t) i];
            if (! v.isSounding()) continue;
            tmp.assign ((size_t) count, 0.0f);
            v.render (tmp.data(), count);
            const float g = gain / std::sqrt ((float) n);
            const float l = g * std::cos ((pan[i] + 1) * juce::MathConstants<float>::pi / 4), r = g * std::sin ((pan[i] + 1) * juce::MathConstants<float>::pi / 4);
            for (int s = 0; s < count; ++s) { L[s] += tmp[(size_t) s] * l; R[s] += tmp[(size_t) s] * r; }
        }
    }
    void reserve (int maxBlock) { tmp.reserve ((size_t) maxBlock); }
private:
    std::array<ChantVoice, kMax> voices;
    std::vector<float> tmp;
    int n = 2, curVowel = 3;
};
} // namespace byz
