#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ScoreRenderer.h"

namespace ui
{
const juce::Colour lapis { 0xff17285e }, lapisDeep { 0xff0f1b42 }, gold { 0xffc69a2b }, cinnabar { 0xffb3261e },
                   panel { 0xfff3f4f7 }, inkC { 0xff15151a }, mute { 0xff5f6170 }, line { 0xffd9dbe3 };

class LookAndFeel : public juce::LookAndFeel_V4
{
public:
    LookAndFeel();
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    juce::Font getTextButtonFont (juce::TextButton&, int h) override;
    void drawTabButton (juce::TabBarButton&, juce::Graphics&, bool over, bool down) override;
};

// Section heading inside a panel
class Heading : public juce::Component
{
public:
    explicit Heading (juce::String t) : text (std::move (t)) {}
    void paint (juce::Graphics& g) override;
    juce::String text;
};

// A palette button that draws its neume
class NeumeButton : public juce::Button
{
public:
    NeumeButton (int delta, int ascentGlyph, juce::String label);
    void paintButton (juce::Graphics&, bool over, bool down) override;
    int delta, glyph;
    juce::String label;
};
} // namespace ui

class PsaltikonEditor;

// ---------------------------------------------------------------- score view
class ScoreView : public juce::Component
{
public:
    ScoreView (PsaltikonEditor& e);
    void paint (juce::Graphics&) override;
    void resized() override { relayout(); }
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;
    void relayout();
    void setWidthForViewport (int w);
    void ensureVisible (int ev);

    byz::ScoreLayout layout;
    byz::ScoreStyle style;
    int playEv = -1;
private:
    PsaltikonEditor& ed;
    int viewportWidth = 800;
};

// ---------------------------------------------------------------- scale ladder
class LadderView : public juce::Component
{
public:
    enum Style { Theory, Parallage };
    LadderView (PsaltikonEditor& e, Style s) : ed (e), style (s) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    int lowDeg = -3, highDeg = 10;
    int highlight = INT_MIN;
    int genusOverride = -1;
private:
    float yOf (int moria) const;
    PsaltikonEditor& ed;
    Style style;
};

// ---------------------------------------------------------------- formula list
class FormulaList : public juce::Component
{
public:
    explicit FormulaList (PsaltikonEditor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void refresh();
    juce::String filter;          // function name or empty for all
    bool cadencesOnly = false;
    int preferredHeight() const { return (int) rows.size() * rowH + 8; }
private:
    struct Row { const byz::Formula* f; };
    std::vector<Row> rows;
    static constexpr int rowH = 46;
    PsaltikonEditor& ed;
};

// ---------------------------------------------------------------- tabs
class ScoreTab : public juce::Component
{
public:
    explicit ScoreTab (PsaltikonEditor& e);
    void resized() override;
    void paint (juce::Graphics&) override;
    void syncFromSelection();
    ScoreView score;
    juce::Viewport scoreViewport;
    juce::TextEditor lyric;
private:
    PsaltikonEditor& ed;
    juce::Component palette;
    juce::Viewport paletteViewport;
    juce::OwnedArray<ui::NeumeButton> neumes;
    juce::OwnedArray<juce::Component> owned;
    juce::ToggleButton replaceMode { "Replace selected note" }, keepPitches { "Keep following pitches" }, showPar { "Show parallage" };
    juce::ComboBox ascent, expr, fthora, accidental, isonMark;
    juce::TextButton gorgon { "Gorgon" }, digorgon { "Digorgon" }, argon { "Argon" }, apli { "Apli" }, dipli { "Dipli" }, tripli { "Tripli" },
                     klasma { "Klasma" }, rest { "Rest" }, martyria { "Martyria" }, undoB { "Undo" }, redoB { "Redo" }, delB { "Delete" },
                     layText { "Lay syllables from selection" };
    juce::TextEditor layEditor;
    juce::Label lyricLabel, hint;
    // bottom bar
    juce::TextEditor title;
    juce::TextButton newB { "New" }, openB { "Open" }, saveB { "Save" }, pdfB { "Export PDF" }, wavB { "Export WAV" }, autoIsonB { "Generate ison" };
    juce::ComboBox wavWhat;
    juce::Label status;
    friend class PsaltikonEditor;
};

class ModeTab : public juce::Component
{
public:
    explicit ModeTab (PsaltikonEditor& e);
    void resized() override;
    void paint (juce::Graphics&) override;
    void refresh();
    LadderView ladder;
private:
    PsaltikonEditor& ed;
    juce::Label title, info;
    juce::TextButton apechema { "Intonation (apechema)" }, up { "Scale up" }, down { "Scale down" }, withIson { "Scale with ison" };
    FormulaList formulas;
    juce::Viewport formulaView;
};

class MelosTab : public juce::Component
{
public:
    explicit MelosTab (PsaltikonEditor& e);
    void resized() override;
    void paint (juce::Graphics&) override;
    void refresh();
private:
    PsaltikonEditor& ed;
    juce::ComboBox fnFilter;
    FormulaList formulas;
    juce::Viewport formulaView;
    juce::TextEditor text;
    juce::ComboBox styleBox;
    juce::TextButton compose { "Compose (replace score)" }, append { "Compose and append" };
    juce::TextButton stepUp { "Step up" }, stepDown { "Step down" }, ornament { "Ornament" }, invert { "Invert" }, retro { "Retrograde" },
                     melisma { "Make melismatic" }, simplify { "Simplify" }, cadence { "End with final cadence" };
    juce::Label composeHead, devHead, devHint;
};

class ParallageTab : public juce::Component
{
public:
    explicit ParallageTab (PsaltikonEditor& e);
    void resized() override;
    void paint (juce::Graphics&) override;
    void refresh();
    LadderView ladder;
private:
    void newQuestion();
    void playQuestion();
    PsaltikonEditor& ed;
    juce::TextButton asc { "Ascending" }, desc { "Descending" }, both { "Up and down" };
    juce::ComboBox exKind; juce::Slider exLen;
    juce::TextButton exGen { "Generate exercise" }, exPlay { "Play" }, exSend { "Send to score" };
    juce::Label exText;
    std::vector<int> exercise;
    juce::TextButton qNew { "New question" }, qRepeat { "Hear again" };
    juce::OwnedArray<juce::TextButton> answers;
    juce::Label qText, qScore;
    int qFirst = 0, qSecond = 0, right = 0, asked = 0;
    bool answered = true;
    juce::ComboBox mDeg, mGenA, mGenB;
    juce::TextButton mA { "Play A" }, mB { "Play B" }, mAB { "A then B" }, tones { "Hear 12 / 10 / 8 moria" };
    juce::Label mText;
    juce::Label h1, h2, h3, h4;
    std::mt19937 rng { 7 };
};

class VoiceTab : public juce::Component
{
public:
    explicit VoiceTab (PsaltikonEditor& e);
    void resized() override;
    void paint (juce::Graphics&) override;
    void refresh();
private:
    PsaltikonEditor& ed;
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    struct LabeledSlider { juce::Slider s; juce::Label l; std::unique_ptr<SA> a; };
    juce::OwnedArray<LabeledSlider> voiceSliders, isonSliders;
    juce::ComboBox profile, isonMode, isonVowel, isonNote;
    std::unique_ptr<CA> profileA, isonModeA, isonVowelA;
    juce::ToggleButton attractions { "Melodic attractions (Zo drawn to Ke)" }, singPar { "Sing parallage syllables instead of the text" },
                       hostSync { "Follow host transport" }, drone { "Drone on (manual mode)" };
    std::unique_ptr<BA> attractionsA, singParA, hostSyncA, droneA;
    juce::TextButton genIson { "Generate ison for score" }, clearIson { "Clear ison indications" }, testVoice { "Hear this voice" };
    juce::Label h1, h2, note, profileL, isonModeL, isonVowelL, isonNoteL;
};

// ---------------------------------------------------------------- editor
class PsaltikonEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit PsaltikonEditor (PsaltikonProcessor&);
    ~PsaltikonEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

    PsaltikonProcessor& proc;
    byz::Composition& comp() { return proc.comp; }
    const byz::Variant& variant() { return proc.comp.var(); }

    // editing
    void edit (const std::function<void (byz::Composition&)>& fn, bool keepSelection = true);
    void refreshAll();
    void setStatus (const juce::String&);
    int selFrom = -1, selTo = -1;
    int selLo() const { return std::min (selFrom, selTo); }
    int selHi() const { return std::max (selFrom, selTo); }
    void select (int from, int to);
    void insertNeume (int delta);
    void insertDegrees (const std::vector<int>& degs);

    // audition
    void playDegrees (const std::vector<int>& degs, bool parallage, std::vector<juce::String> syl = {}, int genusOverride = -1,
                      bool withIson = false, double bpm = 0);
    void togglePlay (bool fromSelection);

    std::unique_ptr<juce::FileChooser> chooser;
    ScoreTab* scoreTab = nullptr;
    ModeTab* modeTab = nullptr;
    MelosTab* melosTab = nullptr;
    ParallageTab* parTab = nullptr;
    VoiceTab* voiceTab = nullptr;

private:
    void timerCallback() override;
    void modeChanged();
    ui::LookAndFeel lnf;
    juce::Label brand;
    juce::ComboBox modeBox, variantBox;
    juce::TextButton playB { "Play" }, playSelB { "Play from selection" }, stopB { "Stop" };
    juce::Slider tempo;
    juce::Label tempoL;
    juce::TabbedComponent tabs { juce::TabbedButtonBar::TabsAtTop };
    int lastVersion = -1, lastEv = -2;
    bool lastPlaying = false;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PsaltikonEditor)
};
