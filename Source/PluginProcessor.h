#pragma once
#include <JuceHeader.h>
#include "Performer.h"

class PsaltikonProcessor : public juce::AudioProcessor, private juce::AsyncUpdater
{
public:
    PsaltikonProcessor();
    ~PsaltikonProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Psaltikon"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    // ---- composition (message thread) ----
    byz::Composition comp;
    std::vector<juce::String> history, future;
    void pushHistory();
    void undoEdit();
    void redoEdit();
    void compositionChanged();         // rebuild timeline after an edit
    byz::TimelineOptions timelineOptions() const;
    byz::PerformParams performParams() const;
    std::atomic<int> compVersion { 0 };            // bumped on every change; the editor watches it

    // ---- transport (message thread -> audio thread) ----
    void playScore (int fromEvent = 0);
    void playPreview (std::shared_ptr<byz::Timeline> t);
    void stopPlayback();
    bool isPlaying() const { return uiPlaying.load(); }
    int playingEvent() const { return uiEvent.load(); }
    bool previewing() const { return uiPreview.load(); }

    // live ison selection from the editor (Manual mode)
    int currentModeIndex() const { return comp.mode; }

private:
    void handleAsyncUpdate() override {}
    std::shared_ptr<const byz::Timeline> scoreTimeline;
    std::vector<std::shared_ptr<const byz::Timeline>> retired;   // keep old timelines alive off the audio thread

    // pending command from the message thread
    struct Command { int kind = 0; std::shared_ptr<const byz::Timeline> tl, score; double from = 0; bool preview = false; };
    juce::SpinLock cmdLock;
    Command pending;
    std::atomic<bool> hasPending { false };

    byz::Performer perf;
    std::atomic<bool> uiPlaying { false }, uiPreview { false };
    std::atomic<int> uiEvent { -1 };
    bool previewActive = false, hostWasPlaying = false;
    std::vector<int> heldKeys;
    std::shared_ptr<const byz::Timeline> audioScore;
    std::atomic<int> modeForAudio { 0 }, variantForAudio { 0 };
    int lastIsonKey = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PsaltikonProcessor)
};
