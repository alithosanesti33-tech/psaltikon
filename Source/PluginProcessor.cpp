#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace byz;

PsaltikonProcessor::PsaltikonProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    comp = sampleComposition();
    compositionChanged();
}

juce::AudioProcessorValueTreeState::ParameterLayout PsaltikonProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;
    auto f = [&] (const char* id, const char* name, float lo, float hi, float def)
    { p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, NormalisableRange<float> (lo, hi), def)); };
    f ("melodyVol", "Melody volume", 0, 1, 0.8f);
    f ("isonVol", "Ison volume", 0, 1, 0.45f);
    f ("reverb", "Church reverb", 0, 1, 0.35f);
    f ("register", "Register (semitones)", -12, 12, 0);
    f ("resonance", "Resonance", 0, 1, 0.5f);
    f ("intensity", "Intensity", 0, 1, 0.6f);
    f ("vibrato", "Vibrato depth", 0, 1.5f, 1.0f);
    f ("vibRate", "Vibrato rate", 0.7f, 1.3f, 1.0f);
    f ("articulation", "Articulation", 0, 1, 0.5f);
    f ("breath", "Breath", 0, 1, 0.5f);
    f ("ornament", "Ornamentation", 0, 1, 0.5f);
    StringArray profiles;
    for (auto& v : voiceProfiles()) profiles.add (v.name);
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "profile", 1 }, "Voice", profiles, 1));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "isonMode", 1 }, "Ison mode",
                 StringArray { "Off", "Follow score", "Manual drone", "Live (MIDI below C2)" }, 1));
    p.push_back (std::make_unique<AudioParameterInt> (ParameterID { "isonNote", 1 }, "Manual ison degree", -7, 14, 1));
    p.push_back (std::make_unique<AudioParameterInt> (ParameterID { "isonVoices", 1 }, "Ison voices", 1, 4, 2));
    p.push_back (std::make_unique<AudioParameterInt> (ParameterID { "isonOctave", 1 }, "Ison octave", -1, 1, 0));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "isonVowel", 1 }, "Ison vowel",
                 StringArray { "a", "e", "i", "o", "u", "m (hum)" }, 3));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "drone", 1 }, "Drone on", false));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "attractions", 1 }, "Melodic attractions", true));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "hostSync", 1 }, "Follow host transport", true));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "singParallage", 1 }, "Sing parallage", false));
    return { p.begin(), p.end() };
}

bool PsaltikonProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void PsaltikonProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    perf.prepare (sampleRate, samplesPerBlock);
    perf.setParams (performParams());
}

TimelineOptions PsaltikonProcessor::timelineOptions() const
{
    TimelineOptions o;
    o.attractions = apvts.getRawParameterValue ("attractions")->load() > 0.5f;
    o.singParallage = apvts.getRawParameterValue ("singParallage")->load() > 0.5f;
    o.articulation = apvts.getRawParameterValue ("articulation")->load();
    o.ornamentation = apvts.getRawParameterValue ("ornament")->load();
    return o;
}

PerformParams PsaltikonProcessor::performParams() const
{
    auto get = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    PerformParams p;
    const auto& vp = voiceProfiles()[(size_t) juce::jlimit (0, 4, (int) get ("profile"))];
    p.voice.niHz = vp.niHz * std::pow (2.0f, get ("register") / 12.0f);
    p.voice.formantScale = vp.formantScale;
    p.voice.brightness = vp.brightness;
    p.voice.vibDepth = vp.vibDepth * get ("vibrato");
    p.voice.vibRate = vp.vibRate * get ("vibRate");
    p.voice.ring = vp.ring;
    p.voice.breath = vp.breath * 2.0f * get ("breath");
    p.voice.nasal = vp.nasal;
    p.voice.resonance = get ("resonance");
    p.voice.intensity = get ("intensity");
    p.melodyGain = get ("melodyVol");
    p.isonGain = get ("isonVol");
    p.reverbWet = get ("reverb");
    p.articulation = get ("articulation");
    p.isonMode = (int) get ("isonMode");
    p.isonVoices = (int) get ("isonVoices");
    p.isonOctave = (int) get ("isonOctave");
    p.isonVowel = (int) get ("isonVowel");
    p.isonManualDeg = (int) get ("isonNote");
    p.droneOn = get ("drone") > 0.5f;
    p.genusForManual = variantOf (modeForAudio.load(), variantForAudio.load()).genus;
    return p;
}

void PsaltikonProcessor::pushHistory()
{
    history.push_back (juce::JSON::toString (toVar (comp), true));
    if (history.size() > 300) history.erase (history.begin());
    future.clear();
}
void PsaltikonProcessor::undoEdit()
{
    if (history.empty()) return;
    future.push_back (juce::JSON::toString (toVar (comp), true));
    fromVar (juce::JSON::parse (history.back()), comp);
    history.pop_back();
    compositionChanged();
}
void PsaltikonProcessor::redoEdit()
{
    if (future.empty()) return;
    history.push_back (juce::JSON::toString (toVar (comp), true));
    fromVar (juce::JSON::parse (future.back()), comp);
    future.pop_back();
    compositionChanged();
}

void PsaltikonProcessor::compositionChanged()
{
    modeForAudio = comp.mode;
    variantForAudio = comp.variant;
    auto tl = buildTimeline (comp, timelineOptions());
    scoreTimeline = tl;
    retired.push_back (tl);
    {
        const juce::SpinLock::ScopedLockType sl (cmdLock);
        pending.score = tl;
        hasPending = true;
    }
    ++compVersion;
    retired.erase (std::remove_if (retired.begin(), retired.end(), [] (const std::shared_ptr<const Timeline>& p) { return p.use_count() <= 1; }), retired.end());
    retired.push_back (tl);
}

void PsaltikonProcessor::playScore (int fromEvent)
{
    compositionChanged();
    double from = 0;
    if (fromEvent > 0 && fromEvent < (int) scoreTimeline->eventStart.size()) from = scoreTimeline->eventStart[(size_t) fromEvent];
    const juce::SpinLock::ScopedLockType sl (cmdLock);
    pending.kind = 1; pending.tl = scoreTimeline; pending.from = from; pending.preview = false;
    hasPending = true;
}

void PsaltikonProcessor::playPreview (std::shared_ptr<Timeline> t)
{
    retired.push_back (t);
    const juce::SpinLock::ScopedLockType sl (cmdLock);
    pending.kind = 1; pending.tl = t; pending.from = 0; pending.preview = true;
    hasPending = true;
}

void PsaltikonProcessor::stopPlayback()
{
    const juce::SpinLock::ScopedLockType sl (cmdLock);
    pending.kind = 2;
    hasPending = true;
}

void PsaltikonProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    if (hasPending.load())
    {
        const juce::SpinLock::ScopedTryLockType sl (cmdLock);
        if (sl.isLocked())
        {
            if (pending.score) audioScore = pending.score;
            if (pending.kind == 1) { perf.play (pending.tl, pending.from); previewActive = pending.preview; }
            else if (pending.kind == 2) perf.stop();
            pending = {};
            hasPending = false;
        }
    }

    perf.setParams (performParams());

    const int isonMode = (int) apvts.getRawParameterValue ("isonMode")->load();
    const bool singPar = apvts.getRawParameterValue ("singParallage")->load() > 0.5f;
    const auto& var = variantOf (modeForAudio.load(), variantForAudio.load());

    // MIDI: white keys are degrees (middle C = Ni), black keys raise the lower note by a diesis.
    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        if (! (m.isNoteOn() || m.isNoteOff())) continue;
        const int key = m.getNoteNumber();
        static const int white[12] = { 0, 0, 1, 1, 2, 3, 3, 4, 4, 5, 5, 6 };
        static const bool black[12] = { false, true, false, true, false, false, true, false, true, false, true, false };
        const bool isIsonKey = isonMode == IsonLive && key < 36;
        const int deg = (key / 12 - (isIsonKey ? 2 : 5)) * 7 + white[key % 12];
        const int alt = black[key % 12] ? 4 : 0;
        const float moria = (float) moriaOf (deg, var.genus, alt);
        if (m.isNoteOn())
        {
            if (isIsonKey) { perf.liveIson (moria); lastIsonKey = key; continue; }
            heldKeys.push_back (key);
            const int vowel = singPar ? vowelIndex (parallageVowel (deg)) : 0;
            perf.liveNoteOn (moria, m.getFloatVelocity(), vowel);
        }
        else
        {
            if (isIsonKey) { if (key == lastIsonKey) { perf.liveIsonOff(); lastIsonKey = -1; } continue; }
            heldKeys.erase (std::remove (heldKeys.begin(), heldKeys.end(), key), heldKeys.end());
            if (heldKeys.empty()) perf.liveNoteOff (true);
            else
            {
                const int k2 = heldKeys.back();
                const int d2 = (k2 / 12 - 5) * 7 + white[k2 % 12];
                perf.liveNoteOn ((float) moriaOf (d2, var.genus, black[k2 % 12] ? 4 : 0), 0.8f, singPar ? vowelIndex (parallageVowel (d2)) : 0);
                perf.liveNoteOff (false);
            }
        }
    }

    // host transport
    double hostBeat = -1, hostBpm = 0;
    bool hostPlaying = false;
    if (apvts.getRawParameterValue ("hostSync")->load() > 0.5f)
        if (auto* ph = getPlayHead())
            if (auto pos = ph->getPosition())
            {
                hostPlaying = pos->getIsPlaying();
                if (hostPlaying)
                {
                    hostBeat = pos->getPpqPosition().orFallback (0.0);
                    hostBpm = pos->getBpm().orFallback (0.0);
                    if (! hostWasPlaying && audioScore) { perf.play (audioScore, hostBeat); previewActive = false; }
                }
                else if (hostWasPlaying && ! previewActive) perf.stop();
            }
    hostWasPlaying = hostPlaying;

    float* L = buffer.getWritePointer (0);
    float* R = buffer.getWritePointer (1);
    perf.process (L, R, buffer.getNumSamples(), (hostPlaying && ! previewActive) ? hostBeat : -1.0, (hostPlaying && ! previewActive) ? hostBpm : 0.0);
    for (int ch = 2; ch < buffer.getNumChannels(); ++ch) buffer.clear (ch, 0, buffer.getNumSamples());

    uiPlaying = perf.isPlaying();
    uiPreview = previewActive && perf.isPlaying();
    uiEvent = previewActive ? -1 : perf.currentEvent();
    midi.clear();
}

void PsaltikonProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("composition", juce::JSON::toString (toVar (comp), true), nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, destData);
}

void PsaltikonProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        auto vt = juce::ValueTree::fromXml (*xml);
        if (vt.isValid())
        {
            const juce::String c = vt.getProperty ("composition").toString();
            vt.removeProperty ("composition", nullptr);
            apvts.replaceState (vt);
            if (c.isNotEmpty() && fromVar (juce::JSON::parse (c), comp))
            {
                history.clear(); future.clear();
                compositionChanged();
            }
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PsaltikonProcessor(); }
juce::AudioProcessorEditor* PsaltikonProcessor::createEditor() { return new PsaltikonEditor (*this); }
