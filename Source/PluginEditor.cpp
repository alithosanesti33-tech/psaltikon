#include "PluginEditor.h"
#include "Exporters.h"

using namespace byz;

static juce::String U (const char* s) { return juce::String (juce::CharPointer_UTF8 (s)); }
static juce::String U (const std::string& s) { return juce::String (juce::CharPointer_UTF8 (s.c_str())); }
static juce::Font uiFont (float h, bool bold = false) { return juce::Font (juce::FontOptions (h, bold ? juce::Font::bold : juce::Font::plain)); }

static juce::String degName (int d) { return U (degreeGreek (d)) + "  " + U (degreeLatin (d)); }

// =====================================================================  look and feel
namespace ui
{
LookAndFeel::LookAndFeel()
{
    setColourScheme ({ panel, juce::Colours::white, juce::Colours::white, line, inkC, lapis, juce::Colours::white, lapis, inkC });
    setColour (juce::TextButton::buttonColourId, juce::Colours::white);
    setColour (juce::TextButton::buttonOnColourId, lapis);
    setColour (juce::TextButton::textColourOffId, inkC);
    setColour (juce::TextButton::textColourOnId, juce::Colours::white);
    setColour (juce::ComboBox::backgroundColourId, juce::Colours::white);
    setColour (juce::ComboBox::outlineColourId, line);
    setColour (juce::ComboBox::textColourId, inkC);
    setColour (juce::ComboBox::arrowColourId, mute);
    setColour (juce::TextEditor::backgroundColourId, juce::Colours::white);
    setColour (juce::TextEditor::outlineColourId, line);
    setColour (juce::TextEditor::focusedOutlineColourId, gold);
    setColour (juce::TextEditor::textColourId, inkC);
    setColour (juce::Label::textColourId, inkC);
    setColour (juce::Slider::thumbColourId, lapis);
    setColour (juce::Slider::trackColourId, gold);
    setColour (juce::Slider::backgroundColourId, line);
    setColour (juce::Slider::textBoxTextColourId, inkC);
    setColour (juce::Slider::textBoxOutlineColourId, line);
    setColour (juce::ToggleButton::textColourId, inkC);
    setColour (juce::ToggleButton::tickColourId, lapis);
    setColour (juce::ToggleButton::tickDisabledColourId, mute);
    setColour (juce::PopupMenu::backgroundColourId, juce::Colours::white);
    setColour (juce::PopupMenu::textColourId, inkC);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, lapis);
    setColour (juce::ScrollBar::thumbColourId, juce::Colour (0xffb9bccb));
    setColour (juce::TabbedComponent::backgroundColourId, panel);
    setColour (juce::TabbedComponent::outlineColourId, juce::Colours::transparentBlack);
    setColour (juce::TabbedButtonBar::tabOutlineColourId, juce::Colours::transparentBlack);
}

void LookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool on = b.getToggleState();
    g.setColour (on ? lapis : (down ? juce::Colour (0xffe8e9ef) : juce::Colours::white));
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (over ? gold : (on ? lapis : line));
    g.drawRoundedRectangle (r, 5.0f, over ? 1.6f : 1.0f);
}

juce::Font LookAndFeel::getTextButtonFont (juce::TextButton&, int h) { return uiFont (std::min (15.0f, h * 0.48f)); }

void LookAndFeel::drawTabButton (juce::TabBarButton& b, juce::Graphics& g, bool over, bool)
{
    auto r = b.getLocalBounds().toFloat();
    const bool front = b.isFrontTab();
    g.setColour (front ? panel : (over ? lapis.brighter (0.15f) : lapisDeep));
    g.fillRect (r.reduced (1.0f, 0).withTrimmedTop (4.0f));
    if (front) { g.setColour (gold); g.fillRect (r.withHeight (3.0f).withY (r.getBottom() - 3.0f).reduced (14.0f, 0)); }
    g.setColour (front ? inkC : juce::Colours::white.withAlpha (0.85f));
    g.setFont (uiFont (15.0f, front));
    g.drawText (b.getButtonText(), r.withTrimmedTop (4.0f), juce::Justification::centred, false);
}

void Heading::paint (juce::Graphics& g)
{
    g.setColour (lapis);
    g.setFont (uiFont (14.0f, true));
    g.drawText (text, getLocalBounds().withTrimmedBottom (4), juce::Justification::bottomLeft, false);
    g.setColour (line);
    g.fillRect (0, getHeight() - 2, getWidth(), 1);
}

NeumeButton::NeumeButton (int d, int gl, juce::String l) : juce::Button (l), delta (d), glyph (gl), label (std::move (l)) {}

void NeumeButton::paintButton (juce::Graphics& g, bool over, bool down)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (down ? juce::Colour (0xffeef0f5) : juce::Colours::white);
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (over ? gold : line);
    g.drawRoundedRectangle (r, 6.0f, over ? 1.6f : 1.0f);
    Event e; e.delta = delta; e.ascentGlyph = glyph;
    const float gw = neumeWidth (e), s = 1.15f;
    g.setColour (inkC);
    Pen P { g, r.getCentreX() - gw * s * 0.5f, r.getY() + 30.0f, s };
    drawQuantity (P, e);
    g.setColour (mute);
    g.setFont (uiFont (11.0f));
    g.drawText (label, r.withTrimmedTop (r.getHeight() - 18.0f), juce::Justification::centred, true);
    g.setColour (delta > 0 ? lapis : (delta < 0 ? cinnabar : mute));
    g.setFont (uiFont (11.0f, true));
    g.drawText (delta > 0 ? "+" + juce::String (delta) : juce::String (delta), r.reduced (6.0f, 4.0f), juce::Justification::topRight, false);
}
} // namespace ui

// =====================================================================  score view
ScoreView::ScoreView (PsaltikonEditor& e) : ed (e)
{
    setWantsKeyboardFocus (true);
    style.scale = 1.15f;
}

void ScoreView::setWidthForViewport (int w)
{
    viewportWidth = std::max (300, w);
    relayout();
}

void ScoreView::relayout()
{
    layout = layoutScore (ed.comp(), (float) viewportWidth, style);
    const int h = std::max (minHeight, (int) std::ceil (layout.height));
    if (getWidth() != viewportWidth || getHeight() != h) setSize (viewportWidth, h);
    repaint();
}

void ScoreView::paint (juce::Graphics& g)
{
    g.fillAll (style.paper);
    paintScore (g, ed.comp(), layout, style, ed.selFrom, ed.selTo, playEv);
}

void ScoreView::ensureVisible (int ev)
{
    for (auto& c : layout.cells)
        if (c.ev == ev)
            if (auto* vp = findParentComponentOfClass<juce::Viewport>())
            {
                const int top = (int) c.y, bottom = (int) (c.y + layout.rowH);
                if (top < vp->getViewPositionY() || bottom > vp->getViewPositionY() + vp->getViewHeight())
                    vp->setViewPosition (0, std::max (0, top - 20));
            }
}

void ScoreView::mouseDown (const juce::MouseEvent& e)
{
    grabKeyboardFocus();
    const int hit = byz::hitTest (layout, e.position);
    if (hit < 0) { ed.select (-1, -1); return; }
    if (e.mods.isShiftDown() && ed.selFrom >= 0) ed.select (ed.selFrom, hit);
    else ed.select (hit, hit);
    // audition the clicked note
    if (! ed.proc.isPlaying() || ed.proc.previewing())
    {
        auto d = derive (ed.comp(), true);
        if (! ed.comp().events[(size_t) hit].pause)
        {
            auto t = std::make_shared<Timeline>();
            t->bpm = 90; t->total = 1.0;
            const auto& ev = ed.comp().events[(size_t) hit];
            const int v = isContinuation (ev.lyric) ? 0 : lyricVowel (ev.lyric, 0);
            t->ctrls.push_back ({ 0.0, CPitch, (float) d.moria[(size_t) hit], 0.0f });
            t->ctrls.push_back ({ 0.0, CVowel, (float) v, 0 });
            t->ctrls.push_back ({ 0.0, CLevel, 1.0f, 0.04f });
            t->ctrls.push_back ({ 0.0, CVibrato, 1.0f, 0.7f });
            t->ctrls.push_back ({ 0.75, CLevel, 0.0f, 0.2f });
            ed.proc.playPreview (t);
        }
    }
}

void ScoreView::mouseDoubleClick (const juce::MouseEvent&)
{
    if (ed.scoreTab) ed.scoreTab->lyric.grabKeyboardFocus();
}

bool ScoreView::keyPressed (const juce::KeyPress& k)
{
    const auto code = k.getKeyCode();
    const auto ch = k.getTextCharacter();
    const bool cmd = k.getModifiers().isCommandDown();
    const bool shift = k.getModifiers().isShiftDown();
    auto& c = ed.comp();
    const int n = (int) c.events.size();

    if (cmd && (code == 'Z' || code == 'z')) { if (shift) ed.proc.redoEdit(); else ed.proc.undoEdit(); ed.refreshAll(); return true; }
    if (cmd && (code == 'Y' || code == 'y')) { ed.proc.redoEdit(); ed.refreshAll(); return true; }
    if (code == juce::KeyPress::spaceKey) { ed.togglePlay (shift); return true; }

    const juce::String shifted (")!@#$%^&");
    if (! cmd && ch >= '0' && ch <= '7') { ed.insertNeume ((int) (ch - '0') * (shift ? -1 : 1)); return true; }
    if (! cmd && shifted.containsChar (ch)) { ed.insertNeume (-shifted.indexOfChar (ch)); return true; }

    if (code == juce::KeyPress::leftKey || code == juce::KeyPress::rightKey)
    {
        if (n == 0) return true;
        const int dir = code == juce::KeyPress::leftKey ? -1 : 1;
        int to = juce::jlimit (0, n - 1, (ed.selTo < 0 ? (dir > 0 ? -1 : n) : ed.selTo) + dir);
        if (shift && ed.selFrom >= 0) ed.select (ed.selFrom, to); else ed.select (to, to);
        ensureVisible (to);
        return true;
    }
    if ((code == juce::KeyPress::backspaceKey || code == juce::KeyPress::deleteKey) && ed.selFrom >= 0)
    {
        const int lo = ed.selLo(), hi = ed.selHi();
        ed.edit ([lo, hi] (Composition& cc)
        {
            // keep the notes after the deletion at their pitch
            const int after = hi + 1 < (int) cc.events.size() ? degreeBefore (cc, hi + 2) : INT_MIN;
            cc.events.erase (cc.events.begin() + lo, cc.events.begin() + hi + 1);
            if (after != INT_MIN && lo < (int) cc.events.size() && ! cc.events[(size_t) lo].pause)
            {
                if (lo == 0) { cc.startDeg = after - cc.events[0].delta; }
                else cc.events[(size_t) lo].delta = juce::jlimit (-7, 7, after - degreeBefore (cc, lo));
            }
        }, false);
        ed.select (std::max (0, lo - 1), std::max (0, lo - 1));
        if (ed.comp().events.empty()) ed.select (-1, -1);
        return true;
    }
    if (ed.selFrom < 0) return false;
    const int lo = ed.selLo(), hi = ed.selHi();
    auto each = [&] (std::function<void (Event&)> f) { ed.edit ([lo, hi, f] (Composition& cc) { for (int i = lo; i <= hi; ++i) f (cc.events[(size_t) i]); }); };
    switch (juce::CharacterFunctions::toLowerCase (ch))
    {
        case 'g': each ([] (Event& e) { e.time = e.time == Gorgon ? TimeNone : Gorgon; }); return true;
        case 'd': each ([] (Event& e) { e.time = e.time == Digorgon ? TimeNone : Digorgon; }); return true;
        case 'a': each ([] (Event& e) { e.ext = (e.ext + 1) % 4; }); return true;
        case 'k': each ([] (Event& e) { e.klasma = ! e.klasma; }); return true;
        case 'm': each ([] (Event& e) { e.martyria = ! e.martyria; }); return true;
        case 'p': ed.edit ([hi] (Composition& cc) { Event p; p.pause = true; cc.events.insert (cc.events.begin() + hi + 1, p); }); ed.select (hi + 1, hi + 1); return true;
        default: break;
    }
    if (code == juce::KeyPress::returnKey) { if (ed.scoreTab) ed.scoreTab->lyric.grabKeyboardFocus(); return true; }
    return false;
}

// =====================================================================  ladder
float LadderView::yOf (int m) const
{
    const auto& v = ed.variant();
    const int g = genusOverride >= 0 ? genusOverride : v.genus;
    const float lo = (float) moriaOf (lowDeg, g), hi = (float) moriaOf (highDeg, g);
    return juce::jmap ((float) m, lo, hi, (float) getHeight() - 22.0f, 22.0f);
}

void LadderView::paint (juce::Graphics& g)
{
    const auto& v = ed.variant();
    const int gen = genusOverride >= 0 ? genusOverride : v.genus;
    auto r = getLocalBounds().toFloat();
    g.setColour (juce::Colours::white);
    g.fillRoundedRectangle (r, 8.0f);
    g.setColour (ui::line);
    g.drawRoundedRectangle (r.reduced (0.5f), 8.0f, 1.0f);
    const float spineX = style == Parallage ? r.getWidth() * 0.42f : r.getWidth() * 0.36f;
    g.setColour (ui::lapis.withAlpha (0.25f));
    g.fillRect (spineX - 2.0f, yOf (moriaOf (highDeg, gen)), 4.0f, yOf (moriaOf (lowDeg, gen)) - yOf (moriaOf (highDeg, gen)));
    for (int d = lowDeg; d <= highDeg; ++d)
    {
        const int m = moriaOf (d, gen);
        const float y = yOf (m);
        const bool isBase = d == v.base, isDom = byz::contains (v.dominants, d), isIson = byz::contains (v.isonNotes, d);
        const auto col = isBase ? ui::cinnabar : (isDom ? ui::gold : ui::lapis);
        if (d == highlight) { g.setColour (ui::gold.withAlpha (0.18f)); g.fillRoundedRectangle (6.0f, y - 13.0f, r.getWidth() - 12.0f, 26.0f, 5.0f); }
        g.setColour (col);
        const float w = isBase ? 64.0f : (isDom ? 52.0f : 36.0f);
        g.fillRoundedRectangle (spineX - w * 0.5f, y - 2.5f, w, 5.0f, 2.5f);
        if (style == Parallage)
        {
            g.setColour (ui::inkC);
            g.setFont (byz::serifFont (22.0f));
            g.drawText (U (degreeGreek (d)), juce::Rectangle<float> (spineX + 40.0f, y - 14.0f, 90.0f, 28.0f), juce::Justification::centredLeft, false);
            g.setColour (ui::mute);
            g.setFont (uiFont (12.0f));
            g.drawText (U (degreeLatin (d)) + "   " + U (noteLetter (d)), juce::Rectangle<float> (spineX + 112.0f, y - 10.0f, 90.0f, 20.0f), juce::Justification::centredLeft, false);
            g.drawText (juce::String (m - moriaOf (v.base, gen)) + " m", juce::Rectangle<float> (8.0f, y - 10.0f, spineX - 44.0f, 20.0f), juce::Justification::centredRight, false);
        }
        else
        {
            g.setColour (ui::inkC);
            g.setFont (byz::serifFont (18.0f));
            g.drawText (U (degreeGreek (d)), juce::Rectangle<float> (spineX + 38.0f, y - 12.0f, 70.0f, 24.0f), juce::Justification::centredLeft, false);
            g.setColour (ui::mute);
            g.setFont (uiFont (12.0f));
            juce::String right = U (degreeLatin (d)) + U (u8"   ϑ ") + U (noteLetter (d)) + "   " + juce::String (juce::roundToInt (centsOfMoria (m - moriaOf (v.base, gen)))) + U (u8"¢");
            g.drawText (right, juce::Rectangle<float> (spineX + 100.0f, y - 10.0f, r.getWidth() - spineX - 104.0f, 20.0f), juce::Justification::centredLeft, false);
            if (isBase || isIson || isDom)
            {
                g.setColour (col);
                g.setFont (uiFont (11.0f, true));
                g.drawText (isBase ? "base" : (isIson ? "ison / dom." : "dominant"), juce::Rectangle<float> (8.0f, y - 10.0f, spineX - 40.0f, 20.0f), juce::Justification::centredLeft, false);
            }
        }
        if (d < highDeg) // interval to the next degree
        {
            const int m2 = moriaOf (d + 1, gen);
            const float ym = (y + yOf (m2)) * 0.5f;
            g.setColour (ui::cinnabar.withAlpha (0.85f));
            g.setFont (uiFont (11.0f, true));
            g.drawText (juce::String (m2 - m), juce::Rectangle<float> (spineX - 34.0f, ym - 8.0f, 26.0f, 16.0f), juce::Justification::centredRight, false);
        }
    }
}

void LadderView::mouseDown (const juce::MouseEvent& e)
{
    const auto& v = ed.variant();
    const int gen = genusOverride >= 0 ? genusOverride : v.genus;
    int best = lowDeg; float bd = 1e9f;
    for (int d = lowDeg; d <= highDeg; ++d) { float dy = std::abs (yOf (moriaOf (d, gen)) - e.position.y); if (dy < bd) { bd = dy; best = d; } }
    highlight = best;
    repaint();
    ed.playDegrees ({ best }, true, {}, genusOverride);
}

// =====================================================================  formula list
void FormulaList::refresh()
{
    rows.clear();
    for (auto& f : ed.variant().formulas)
    {
        const juce::String fn (f.function);
        if (cadencesOnly && ! fn.contains ("cadence") && fn != "Opening") continue;
        if (filter.isNotEmpty() && fn != filter) continue;
        rows.push_back ({ &f });
    }
    setSize (getWidth(), preferredHeight());
    repaint();
}

void FormulaList::paint (juce::Graphics& g)
{
    auto w = (float) getWidth();
    for (size_t i = 0; i < rows.size(); ++i)
    {
        const auto* f = rows[i].f;
        const float y = 4.0f + (float) i * rowH;
        auto r = juce::Rectangle<float> (0, y, w, (float) rowH - 6.0f);
        g.setColour (juce::Colours::white);
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (ui::line);
        g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.0f);
        const juce::String fn (f->function);
        g.setColour (fn.contains ("Final") ? ui::cinnabar : (fn.contains ("Medial") ? ui::gold.darker (0.3f) : ui::lapis));
        g.setFont (uiFont (11.0f, true));
        g.drawText (fn, r.reduced (10.0f, 3.0f).withHeight (14.0f), juce::Justification::topLeft, false);
        g.setColour (ui::inkC);
        g.setFont (uiFont (14.0f));
        g.drawText (f->name, r.reduced (10.0f, 3.0f).withTrimmedTop (15.0f), juce::Justification::centredLeft, true);
        // contour
        const float cx = w * 0.46f, cw = w * 0.28f;
        int lo = *std::min_element (f->degs.begin(), f->degs.end()), hi = *std::max_element (f->degs.begin(), f->degs.end());
        if (hi == lo) hi = lo + 1;
        juce::Path p;
        for (size_t k = 0; k < f->degs.size(); ++k)
        {
            float px = cx + cw * (float) k / (float) std::max<size_t> (1, f->degs.size() - 1);
            float py = juce::jmap ((float) f->degs[k], (float) lo, (float) hi, r.getBottom() - 8.0f, r.getY() + 8.0f);
            if (k == 0) p.startNewSubPath (px, py); else p.lineTo (px, py);
            g.setColour (ui::lapis); g.fillEllipse (px - 2.5f, py - 2.5f, 5.0f, 5.0f);
        }
        g.setColour (ui::lapis.withAlpha (0.6f));
        g.strokePath (p, juce::PathStrokeType (1.5f));
        g.setColour (ui::mute);
        g.setFont (byz::serifFont (12.0f));
        juce::String names;
        for (int d : f->degs) names << U (noteGreek (d)) << " ";
        g.drawText (names, juce::Rectangle<float> (cx, r.getBottom() - 15.0f, cw, 14.0f), juce::Justification::centred, true);
        // buttons
        auto bPlay = juce::Rectangle<float> (w - 132.0f, y + 8.0f, 58.0f, (float) rowH - 22.0f);
        auto bIns = juce::Rectangle<float> (w - 68.0f, y + 8.0f, 60.0f, (float) rowH - 22.0f);
        g.setColour (ui::lapis); g.fillRoundedRectangle (bPlay, 4.0f);
        g.setColour (ui::gold); g.fillRoundedRectangle (bIns, 4.0f);
        g.setColour (juce::Colours::white); g.setFont (uiFont (12.0f, true));
        g.drawText ("Play", bPlay, juce::Justification::centred, false);
        g.drawText ("Insert", bIns, juce::Justification::centred, false);
    }
    if (rows.empty())
    {
        g.setColour (ui::mute); g.setFont (uiFont (13.0f));
        g.drawText ("No formulas for this filter.", getLocalBounds(), juce::Justification::centred, false);
    }
}

void FormulaList::mouseDown (const juce::MouseEvent& e)
{
    const int i = (int) ((e.position.y - 4.0f) / rowH);
    if (i < 0 || i >= (int) rows.size()) return;
    const float w = (float) getWidth();
    if (e.position.x >= w - 132.0f && e.position.x < w - 74.0f) ed.playDegrees (rows[(size_t) i].f->degs, true);
    else if (e.position.x >= w - 68.0f) ed.insertDegrees (rows[(size_t) i].f->degs);
    else ed.playDegrees (rows[(size_t) i].f->degs, true);
}

// =====================================================================  score tab
ScoreTab::ScoreTab (PsaltikonEditor& e) : score (e), ed (e)
{
    addAndMakeVisible (scoreViewport);
    scoreViewport.setViewedComponent (&score, false);
    scoreViewport.setScrollBarsShown (true, false);
    addAndMakeVisible (paletteViewport);
    paletteViewport.setViewedComponent (&palette, false);
    paletteViewport.setScrollBarsShown (true, false);

    const int deltas[] = { 1, 2, 3, 4, 5, 6, 7, 0, -1, -2, -3, -4, -5, -6, -7 };
    const char* names[] = { "Oligon", "Olig. + kentima", "Olig. + kentimata", "Ypsili (right)", "Ypsili (middle)", "Ypsili (left)", "Ypsili + kentima",
                            "Ison", "Apostrophos", "Elafron", "Elafron + apostr.", "Chamili", "Chamili + apostr.", "Chamili + elafron", "Cham. + el. + ap." };
    for (int i = 0; i < 15; ++i)
    {
        auto* b = neumes.add (new ui::NeumeButton (deltas[i], 0, names[i]));
        const int d = deltas[i];
        b->onClick = [this, d] { ed.insertNeume (d); };
        b->setTooltip ("Insert this neume after the selection (key " + juce::String (std::abs (d)) + (d < 0 ? ", with Shift)" : ")"));
        palette.addAndMakeVisible (b);
    }
    auto head = [this] (const char* t) { auto* h = new ui::Heading (t); owned.add (h); palette.addAndMakeVisible (h); return h; };
    head ("Quantity neumes"); head ("Rhythm"); head ("Signs on the selected notes"); head ("Text");
    for (auto* t : { &replaceMode, &keepPitches, &showPar }) palette.addAndMakeVisible (t);
    keepPitches.setToggleState (true, juce::dontSendNotification);
    showPar.setToggleState (true, juce::dontSendNotification);
    showPar.onClick = [this] { score.style.showParallage = showPar.getToggleState(); score.relayout(); };

    ascent.addItem ("Ascending step drawn as oligon", 1); ascent.addItem ("... as petasti (swelling)", 2); ascent.addItem ("... as kentimata (light)", 3);
    ascent.setSelectedId (1, juce::dontSendNotification);
    expr.addItem ("No expression sign", 1); expr.addItem ("Vareia (stress)", 2); expr.addItem ("Psifiston (accent)", 3);
    expr.addItem ("Omalon (gentle oscillation)", 4); expr.addItem ("Antikenoma (approach from below)", 5); expr.addItem ("Eteron (bind to next)", 6);
    fthora.addItem ("No fthora", 1);
    for (int gi = 0; gi < 4; ++gi) fthora.addItem ("Fthora: " + juce::String (genusName (gi)), gi + 2);
    accidental.addItem ("No accidental", 1); accidental.addItem (U (u8"Ὕφεσις (−4 moria)"), 2); accidental.addItem (U (u8"Δίεσις (+4 moria)"), 3);
    isonMark.addItem ("No ison indication", 1);
    for (int d = -3; d <= 10; ++d) isonMark.addItem ("Ison on " + degName (d), d + 100);
    for (auto* cb : { &ascent, &expr, &fthora, &accidental, &isonMark }) { palette.addAndMakeVisible (cb); cb->setSelectedId (1, juce::dontSendNotification); }

    auto each = [this] (std::function<void (Event&)> f)
    {
        if (ed.selFrom < 0) { ed.setStatus ("Select a note first (click it in the score)."); return; }
        const int lo = ed.selLo(), hi = ed.selHi();
        ed.edit ([lo, hi, f] (Composition& c) { for (int i = lo; i <= hi; ++i) f (c.events[(size_t) i]); });
    };
    ascent.onChange = [this, each] { const int g = ascent.getSelectedId() - 1; if (ed.selFrom >= 0) each ([g] (Event& e) { if (e.delta == 1) e.ascentGlyph = g; }); };
    expr.onChange = [this, each] { const int x = expr.getSelectedId() - 1; each ([x] (Event& e) { if (! e.pause) e.expr = x; }); };
    fthora.onChange = [this, each] { const int f = fthora.getSelectedId() - 2; each ([f] (Event& e) { if (! e.pause) e.fthora = f; }); };
    accidental.onChange = [this, each] { const int a = accidental.getSelectedId() == 2 ? -1 : (accidental.getSelectedId() == 3 ? 1 : 0); each ([a] (Event& e) { if (! e.pause) e.accidental = a; }); };
    isonMark.onChange = [this] {
        if (ed.selFrom < 0) return;
        const int id = isonMark.getSelectedId(); const int lo = ed.selLo();
        ed.edit ([lo, id] (Composition& c) { c.events[(size_t) lo].ison = id <= 1 ? kNoIson : id - 100; });
    };

    for (auto* b : { &gorgon, &digorgon, &argon, &apli, &dipli, &tripli, &klasma, &rest, &martyria, &undoB, &redoB, &delB, &layText }) palette.addAndMakeVisible (b);
    gorgon.onClick = [each] { each ([] (Event& e) { if (! e.pause) e.time = e.time == Gorgon ? TimeNone : Gorgon; }); };
    digorgon.onClick = [each] { each ([] (Event& e) { if (! e.pause) e.time = e.time == Digorgon ? TimeNone : Digorgon; }); };
    argon.onClick = [each] { each ([] (Event& e) { if (! e.pause) e.time = e.time == Argon ? TimeNone : Argon; }); };
    apli.onClick = [each] { each ([] (Event& e) { e.ext = e.ext == 1 ? 0 : 1; }); };
    dipli.onClick = [each] { each ([] (Event& e) { e.ext = e.ext == 2 ? 0 : 2; }); };
    tripli.onClick = [each] { each ([] (Event& e) { e.ext = e.ext == 3 ? 0 : 3; }); };
    klasma.onClick = [each] { each ([] (Event& e) { e.klasma = ! e.klasma; }); };
    martyria.onClick = [each] { each ([] (Event& e) { if (! e.pause) e.martyria = ! e.martyria; }); };
    rest.onClick = [this] {
        const int at = ed.selFrom >= 0 ? ed.selHi() + 1 : (int) ed.comp().events.size();
        ed.edit ([at] (Composition& c) { Event p; p.pause = true; c.events.insert (c.events.begin() + at, p); });
        ed.select (at, at);
    };
    undoB.onClick = [this] { ed.proc.undoEdit(); ed.refreshAll(); };
    redoB.onClick = [this] { ed.proc.redoEdit(); ed.refreshAll(); };
    delB.onClick = [this] { score.keyPressed (juce::KeyPress (juce::KeyPress::deleteKey)); };
    gorgon.setTooltip ("Gorgon (G): this note and the one before share one beat");
    digorgon.setTooltip ("Digorgon (D): three notes in one beat");
    argon.setTooltip ("Argon: the note before is quick, this note is lengthened");
    apli.setTooltip ("Apli (A cycles): add one beat");
    klasma.setTooltip ("Klasma (K): add one beat");
    martyria.setTooltip ("Martyria (M): print a martyria after the note");
    rest.setTooltip ("Leimma (P): a rest; the voice breathes here");

    palette.addAndMakeVisible (lyricLabel);
    lyricLabel.setText ("Syllable of the selected note (Enter = next note, _ = melisma):", juce::dontSendNotification);
    lyricLabel.setFont (uiFont (12.0f)); lyricLabel.setColour (juce::Label::textColourId, ui::mute);
    palette.addAndMakeVisible (lyric);
    lyric.setFont (uiFont (17.0f));
    auto applyLyric = [this] (bool advance)
    {
        if (ed.selFrom < 0) return;
        const int i = ed.selLo();
        const auto t = lyric.getText();
        if (ed.comp().events[(size_t) i].lyric != t) ed.edit ([i, t] (Composition& c) { c.events[(size_t) i].lyric = t; });
        if (advance)
        {
            int nx = i + 1;
            while (nx < (int) ed.comp().events.size() && ed.comp().events[(size_t) nx].pause) ++nx;
            if (nx < (int) ed.comp().events.size()) { ed.select (nx, nx); score.ensureVisible (nx); lyric.grabKeyboardFocus(); lyric.selectAll(); }
        }
    };
    lyric.onReturnKey = [applyLyric] { applyLyric (true); };
    lyric.onFocusLost = [applyLyric] { applyLyric (false); };

    palette.addAndMakeVisible (layEditor);
    layEditor.setFont (uiFont (15.0f));
    layEditor.setMultiLine (true);
    layEditor.setReturnKeyStartsNewLine (true);
    layEditor.setText (U (u8"Κύ-ρι-ε ἐ-κέ-κρα-ξα πρὸς σέ"), false);
    layEditor.setTooltip ("Syllables separated by spaces or hyphens; _ holds the previous syllable over one more note.");
    layText.onClick = [this] {
        juce::StringArray tok;
        for (auto w : juce::StringArray::fromTokens (layEditor.getText().replace ("\n", " "), " ", ""))
            for (auto s2 : juce::StringArray::fromTokens (w, "-", "")) if (s2.isNotEmpty()) tok.add (s2);
        int i = ed.selFrom >= 0 ? ed.selLo() : 0;
        ed.edit ([&tok, i] (Composition& c) mutable
        {
            for (auto& t : tok)
            {
                while (i < (int) c.events.size() && c.events[(size_t) i].pause) ++i;
                if (i >= (int) c.events.size()) break;
                c.events[(size_t) i].lyric = t == "_" ? juce::String() : t;
                ++i;
            }
        });
    };

    palette.addAndMakeVisible (hint);
    hint.setFont (uiFont (12.0f));
    hint.setColour (juce::Label::textColourId, ui::mute);
    hint.setText ("Keys: 0-7 ascend, Shift+0-7 descend, arrows move, Shift-click or Shift+arrows select a range, "
                  "Delete removes, G gorgon, D digorgon, A apli, K klasma, M martyria, P rest, Space play, Cmd+Z undo.",
                  juce::dontSendNotification);
    hint.setJustificationType (juce::Justification::topLeft);

    // bottom bar
    for (auto* c : std::initializer_list<juce::Component*> { &title, &newB, &openB, &saveB, &pdfB, &wavB, &wavWhat, &autoIsonB, &status }) addAndMakeVisible (c);
    title.setFont (uiFont (16.0f));
    title.setText (ed.comp().title, false);
    title.applyFontToAllText (uiFont (16.0f));
    title.setIndents (8, 6);
    title.onTextChange = [this] { ed.comp().title = title.getText(); score.repaint(); };
    wavWhat.addItem ("Melody + ison", 1); wavWhat.addItem ("Melody only", 2); wavWhat.addItem ("Ison only", 3);
    wavWhat.setSelectedId (1, juce::dontSendNotification);
    status.setFont (uiFont (12.0f));
    status.setColour (juce::Label::textColourId, ui::mute);
    autoIsonB.onClick = [this] { ed.edit ([] (Composition& c) { autoIson (c); }); ed.setStatus ("Ison indications generated from the melody. Edit any of them with 'Ison on ...'."); };
    newB.onClick = [this] {
        ed.edit ([] (Composition& c) { Composition n; n.mode = c.mode; n.variant = c.variant; n.bpm = c.bpm; n.startDeg = n.var().base; n.title = "Untitled"; c = n; }, false);
        ed.select (-1, -1);
        ed.setStatus ("New score. Undo brings back the previous one.");
    };
    saveB.onClick = [this] {
        ed.chooser = std::make_unique<juce::FileChooser> ("Save composition", juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                                                          .getChildFile (ed.comp().title.isEmpty() ? juce::String ("Untitled") : ed.comp().title).withFileExtension ("psaltikon"), "*.psaltikon");
        ed.chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting, [this] (const juce::FileChooser& fc)
        {
            auto f = fc.getResult();
            if (f == juce::File()) return;
            const bool ok = f.replaceWithText (juce::JSON::toString (toVar (ed.comp())));
            ed.setStatus (ok ? "Saved " + f.getFileName() : "Could not save " + f.getFullPathName());
        });
    };
    openB.onClick = [this] {
        ed.chooser = std::make_unique<juce::FileChooser> ("Open composition", juce::File::getSpecialLocation (juce::File::userDocumentsDirectory), "*.psaltikon;*.json");
        ed.chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this] (const juce::FileChooser& fc)
        {
            auto f = fc.getResult();
            if (! f.existsAsFile()) return;
            Composition c;
            if (! fromVar (juce::JSON::parse (f.loadFileAsString()), c)) { ed.setStatus ("That file is not a Psaltikon composition."); return; }
            ed.edit ([c] (Composition& cc) { cc = c; }, false);
            ed.select (-1, -1);
            ed.setStatus ("Opened " + f.getFileName());
        });
    };
    pdfB.onClick = [this] {
        ed.chooser = std::make_unique<juce::FileChooser> ("Export PDF", juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                                                          .getChildFile (ed.comp().title.isEmpty() ? juce::String ("Score") : ed.comp().title).withFileExtension ("pdf"), "*.pdf");
        ed.chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting, [this] (const juce::FileChooser& fc)
        {
            auto f = fc.getResult();
            if (f == juce::File()) return;
            auto pages = renderPages (ed.comp(), score.style.showParallage);
            const bool ok = writePdf (pages, f.withFileExtension ("pdf"), ed.comp().title);
            ed.setStatus (ok ? "Exported " + juce::String ((int) pages.size()) + " page(s) to " + f.getFileName() : "Could not write the PDF.");
        });
    };
    wavB.onClick = [this] {
        ed.chooser = std::make_unique<juce::FileChooser> ("Export audio", juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                                                          .getChildFile (ed.comp().title.isEmpty() ? juce::String ("Chant") : ed.comp().title).withFileExtension ("wav"), "*.wav");
        ed.chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting, [this] (const juce::FileChooser& fc)
        {
            auto f = fc.getResult();
            if (f == juce::File()) return;
            f = f.withFileExtension ("wav");
            const int what = wavWhat.getSelectedId() - 1;
            auto c = ed.comp(); auto o = ed.proc.timelineOptions(); auto p = ed.proc.performParams();
            ed.setStatus ("Rendering " + f.getFileName() + " ...");
            juce::Component::SafePointer<PsaltikonEditor> safe (&ed);
            juce::Thread::launch ([c, o, p, what, f, safe]
            {
                const bool ok = renderWav (c, o, p, what, f);
                juce::MessageManager::callAsync ([safe, ok, f] { if (safe) safe->setStatus (ok ? "Exported " + f.getFileName() : "Could not write " + f.getFullPathName()); });
            });
        });
    };
}

void ScoreTab::paint (juce::Graphics& g)
{
    g.fillAll (ui::panel);
    g.setColour (ui::line);
    g.fillRect (paletteViewport.getRight(), 0, 1, getHeight());
}

void ScoreTab::resized()
{
    auto r = getLocalBounds();
    auto bottom = r.removeFromBottom (44).reduced (10, 7);
    title.setBounds (bottom.removeFromLeft (260)); bottom.removeFromLeft (8);
    for (auto* b : { &newB, &openB, &saveB }) { b->setBounds (bottom.removeFromLeft (62)); bottom.removeFromLeft (4); }
    bottom.removeFromLeft (8);
    autoIsonB.setBounds (bottom.removeFromLeft (118)); bottom.removeFromLeft (8);
    pdfB.setBounds (bottom.removeFromLeft (96)); bottom.removeFromLeft (4);
    wavB.setBounds (bottom.removeFromLeft (96)); bottom.removeFromLeft (4);
    wavWhat.setBounds (bottom.removeFromLeft (130)); bottom.removeFromLeft (8);
    status.setBounds (bottom);

    paletteViewport.setBounds (r.removeFromLeft (336));
    scoreViewport.setBounds (r.reduced (0));
    score.minHeight = scoreViewport.getHeight();
    score.setWidthForViewport (scoreViewport.getMaximumVisibleWidth());

    // palette contents
    const int W = 316;
    int y = 6;
    auto H = [&] (int idx) { owned[idx]->setBounds (10, y, W - 10, 26); y += 30; };
    H (0);
    for (int i = 0; i < 15; ++i) neumes[i]->setBounds (10 + (i % 3) * 102, y + (i / 3) * 62, 98, 58);
    y += 5 * 62 + 4;
    ascent.setBounds (10, y, W - 10, 26); y += 30;
    replaceMode.setBounds (10, y, W - 10, 22); y += 22;
    keepPitches.setBounds (10, y, W - 10, 22); y += 28;
    H (1);
    gorgon.setBounds (10, y, 98, 28); digorgon.setBounds (112, y, 98, 28); argon.setBounds (214, y, 98, 28); y += 32;
    apli.setBounds (10, y, 74, 28); dipli.setBounds (88, y, 74, 28); tripli.setBounds (166, y, 72, 28); klasma.setBounds (242, y, 70, 28); y += 32;
    rest.setBounds (10, y, 98, 28); martyria.setBounds (112, y, 98, 28); delB.setBounds (214, y, 98, 28); y += 32;
    undoB.setBounds (10, y, 150, 28); redoB.setBounds (162, y, 150, 28); y += 36;
    H (2);
    for (auto* cb : { &expr, &fthora, &accidental, &isonMark }) { cb->setBounds (10, y, W - 10, 26); y += 30; }
    y += 4;
    H (3);
    lyricLabel.setBounds (10, y, W - 10, 18); y += 20;
    lyric.setBounds (10, y, W - 10, 30); y += 36;
    layEditor.setBounds (10, y, W - 10, 56); y += 60;
    layText.setBounds (10, y, W - 10, 28); y += 34;
    showPar.setBounds (10, y, W - 10, 22); y += 26;
    hint.setBounds (10, y, W - 10, 96); y += 100;
    palette.setSize (W + 6, y);
}

void ScoreTab::syncFromSelection()
{
    const auto& c = ed.comp();
    if (ed.selFrom < 0 || ed.selLo() >= (int) c.events.size())
    {
        lyric.setText ({}, false);
        for (auto* cb : { &expr, &fthora, &accidental, &isonMark }) cb->setSelectedId (1, juce::dontSendNotification);
        for (auto* b : { &gorgon, &digorgon, &argon, &apli, &dipli, &tripli, &klasma, &martyria }) b->setToggleState (false, juce::dontSendNotification);
        return;
    }
    const auto& e = c.events[(size_t) ed.selLo()];
    if (! lyric.hasKeyboardFocus (true)) lyric.setText (e.lyric, false);
    expr.setSelectedId (e.expr + 1, juce::dontSendNotification);
    fthora.setSelectedId (e.fthora + 2, juce::dontSendNotification);
    accidental.setSelectedId (e.accidental < 0 ? 2 : (e.accidental > 0 ? 3 : 1), juce::dontSendNotification);
    isonMark.setSelectedId (e.ison == kNoIson ? 1 : e.ison + 100, juce::dontSendNotification);
    if (e.delta == 1) ascent.setSelectedId (e.ascentGlyph + 1, juce::dontSendNotification);
    gorgon.setToggleState (e.time == Gorgon, juce::dontSendNotification);
    digorgon.setToggleState (e.time == Digorgon, juce::dontSendNotification);
    argon.setToggleState (e.time == Argon, juce::dontSendNotification);
    apli.setToggleState (e.ext == 1, juce::dontSendNotification);
    dipli.setToggleState (e.ext == 2, juce::dontSendNotification);
    tripli.setToggleState (e.ext == 3, juce::dontSendNotification);
    klasma.setToggleState (e.klasma, juce::dontSendNotification);
    martyria.setToggleState (e.martyria, juce::dontSendNotification);
}

// =====================================================================  mode tab
ModeTab::ModeTab (PsaltikonEditor& e) : ladder (e, LadderView::Theory), ed (e), formulas (e)
{
    addAndMakeVisible (ladder);
    addAndMakeVisible (title);
    addAndMakeVisible (info);
    title.setFont (byz::serifFont (30.0f));
    info.setFont (uiFont (14.0f));
    info.setJustificationType (juce::Justification::topLeft);
    for (auto* b : { &apechema, &up, &down, &withIson }) addAndMakeVisible (b);
    apechema.onClick = [this] {
        const auto& v = ed.variant();
        juce::StringArray syl; syl.addTokens (U (v.apechema), "-", "");
        std::vector<juce::String> s (syl.begin(), syl.end());
        ed.playDegrees (v.apechemaMel, false, s, -1, true, 72);
    };
    up.onClick = [this] { const int b = ed.variant().base; std::vector<int> d; for (int k = 0; k <= 7; ++k) d.push_back (b + k); ed.playDegrees (d, true); };
    down.onClick = [this] { const int b = ed.variant().base; std::vector<int> d; for (int k = 7; k >= 0; --k) d.push_back (b + k); ed.playDegrees (d, true); };
    withIson.onClick = [this] { const int b = ed.variant().base; std::vector<int> d; for (int k = 0; k <= 7; ++k) d.push_back (b + k); for (int k = 6; k >= 0; --k) d.push_back (b + k); ed.playDegrees (d, true, {}, -1, true); };
    formulas.cadencesOnly = true;
    formulaView.setViewedComponent (&formulas, false);
    formulaView.setScrollBarsShown (true, false);
    addAndMakeVisible (formulaView);
}

void ModeTab::paint (juce::Graphics& g)
{
    g.fillAll (ui::panel);
    g.setColour (ui::lapis);
    g.setFont (uiFont (14.0f, true));
    g.drawText ("Openings and cadences of this mode", formulaView.getBounds().translated (0, -24).withHeight (20), juce::Justification::centredLeft, false);
}

void ModeTab::resized()
{
    auto r = getLocalBounds().reduced (14);
    ladder.setBounds (r.removeFromLeft (360));
    r.removeFromLeft (18);
    title.setBounds (r.removeFromTop (44));
    info.setBounds (r.removeFromTop (190));
    auto row = r.removeFromTop (32);
    for (auto* b : { &apechema, &up, &down, &withIson }) { b->setBounds (row.removeFromLeft (row.getWidth() >= 640 ? 160 : 130)); row.removeFromLeft (8); }
    r.removeFromTop (36);
    formulaView.setBounds (r);
    formulas.setSize (formulaView.getMaximumVisibleWidth(), formulas.preferredHeight());
}

void ModeTab::refresh()
{
    const auto& c = ed.comp();
    const auto& md = modes()[(size_t) c.mode];
    const auto& v = c.var();
    title.setText (U (md.greek) + "  " + juce::String (juce::CharPointer_UTF8 (u8"·")) + "  " + md.english, juce::dontSendNotification);
    juce::String t;
    t << "Form: " << v.name << "      Genus: " << genusName (v.genus) << " (" << U (genusGreek (v.genus)) << ")\n";
    t << "Base note: " << degName (v.base) << "\n";
    juce::StringArray doms, isons;
    for (int d : v.dominants) doms.add (U (degreeGreek (d)));
    for (int d : v.isonNotes) isons.add (U (degreeGreek (d)));
    t << "Dominant notes: " << doms.joinIntoString (", ") << "      Ison notes: " << isons.joinIntoString (", ") << "\n";
    juce::StringArray iv;
    for (int k = 0; k < 7; ++k) iv.add (juce::String (moriaOf (v.base + k + 1, v.genus) - moriaOf (v.base + k, v.genus)));
    t << "Intervals upward from the base (moria, 72 to the octave): " << iv.joinIntoString ("  ") << "\n";
    t << "Apechema (intonation): " << U (v.apechema).replace ("-", "") << "\n\n";
    t << md.character;
    info.setText (t, juce::dontSendNotification);
    ladder.lowDeg = std::min (v.base - 3, -1);
    ladder.highDeg = ladder.lowDeg + 12;
    ladder.repaint();
    formulas.refresh();
    resized();
}

// =====================================================================  melos tab
MelosTab::MelosTab (PsaltikonEditor& e) : ed (e), formulas (e)
{
    addAndMakeVisible (fnFilter);
    fnFilter.addItem ("All formulas", 1);
    for (int i = 0; i < kNumFunctions; ++i) fnFilter.addItem (kFunctions[i], i + 2);
    fnFilter.setSelectedId (1, juce::dontSendNotification);
    fnFilter.onChange = [this] { formulas.filter = fnFilter.getSelectedId() <= 1 ? juce::String() : juce::String (kFunctions[fnFilter.getSelectedId() - 2]); formulas.refresh(); };
    formulaView.setViewedComponent (&formulas, false);
    formulaView.setScrollBarsShown (true, false);
    addAndMakeVisible (formulaView);

    addAndMakeVisible (composeHead); addAndMakeVisible (devHead); addAndMakeVisible (devHint);
    composeHead.setText ("Set a text to a melody in this mode", juce::dontSendNotification);
    devHead.setText ("Develop the selected passage (stays in the mode)", juce::dontSendNotification);
    for (auto* l : { &composeHead, &devHead }) { l->setFont (uiFont (14.0f, true)); l->setColour (juce::Label::textColourId, ui::lapis); }
    devHint.setFont (uiFont (12.0f)); devHint.setColour (juce::Label::textColourId, ui::mute);
    devHint.setText ("Select notes in the Score tab (click, then Shift-click) and apply. Each change can be undone.", juce::dontSendNotification);

    addAndMakeVisible (text);
    text.setMultiLine (true); text.setReturnKeyStartsNewLine (true);
    text.setFont (uiFont (16.0f));
    text.setText (U (u8"Κύ-ρι-ε ἐ-κέ-κρα-ξα πρὸς σέ, εἰ-σά-κου-σόν μου· εἰ-σά-κου-σόν μου, Κύ-ρι-ε."), false);
    addAndMakeVisible (styleBox);
    styleBox.addItem ("Syllabic (one note per syllable)", 1); styleBox.addItem ("Neumatic (one to three notes)", 2); styleBox.addItem ("Melismatic (long melismas)", 3);
    styleBox.setSelectedId (2, juce::dontSendNotification);
    for (auto* b : { &compose, &append, &stepUp, &stepDown, &ornament, &invert, &retro, &melisma, &simplify, &cadence }) addAndMakeVisible (b);

    auto make = [this] { auto& c = ed.comp(); return composeFromText (text.getText(), styleBox.getSelectedId() - 1, c.mode, c.variant, c.bpm, (unsigned) juce::Random::getSystemRandom().nextInt()); };
    compose.onClick = [this, make] {
        auto n = make(); auto title = ed.comp().title;
        ed.edit ([n, title] (Composition& c) { c = n; c.title = title; }, false);
        ed.select (-1, -1);
        ed.setStatus ("Composed a new melody. Press Compose again for another version, or Undo.");
    };
    append.onClick = [this, make] {
        auto n = make();
        ed.edit ([n] (Composition& c)
        {
            if (! c.events.empty()) { Event p; p.pause = true; c.events.push_back (p); }
            auto d = derive (n, false);
            std::vector<int> degs; std::vector<juce::String> ly;
            const size_t at = c.events.size();
            for (size_t i = 0; i < n.events.size(); ++i) { degs.push_back (d.deg[i]); ly.push_back (n.events[i].lyric); }
            insertDegrees (c, (int) at, degs, ly);
            for (size_t i = 0; i < n.events.size(); ++i)
            {
                auto& e = c.events[at + i]; const auto& s = n.events[i];
                e.pause = s.pause; e.time = s.time; e.ext = s.ext; e.martyria = s.martyria;
                if (s.pause) e.delta = 0;
            }
        });
    };

    auto range = [this] (std::function<std::vector<int> (std::vector<int>)> f, const char* what)
    {
        if (ed.selFrom < 0 || ed.selLo() == ed.selHi()) { ed.setStatus ("Select two or more notes in the Score tab first."); return; }
        const int lo = ed.selLo(), hi = ed.selHi();
        ed.edit ([lo, hi, f] (Composition& c)
        {
            auto degs = rangeDegrees (c, lo, hi);
            for (int i = lo; i <= hi; ++i) if (c.events[(size_t) i].pause) return;  // keep it simple across rests
            auto nd = f (degs);
            replaceRange (c, lo, hi, nd);
        });
        ed.setStatus (juce::String (what) + " applied to the selection.");
    };
    stepUp.onClick = [this] { if (ed.selFrom < 0) return; const int lo = ed.selLo(), hi = ed.selHi(); ed.edit ([lo, hi] (Composition& c) { shiftRange (c, lo, hi, 1); }); };
    stepDown.onClick = [this] { if (ed.selFrom < 0) return; const int lo = ed.selLo(), hi = ed.selHi(); ed.edit ([lo, hi] (Composition& c) { shiftRange (c, lo, hi, -1); }); };
    invert.onClick = [range] { range ([] (std::vector<int> d) { const int a = d.front(); for (auto& x : d) x = 2 * a - x; return d; }, "Inversion"); };
    retro.onClick = [range] { range ([] (std::vector<int> d) { std::reverse (d.begin(), d.end()); return d; }, "Retrograde"); };
    ornament.onClick = [range] { range ([] (std::vector<int> d)
    {
        std::vector<int> o;
        for (size_t k = 0; k < d.size(); ++k)
        {
            o.push_back (d[k]);
            if (k + 1 < d.size() && k % 2 == 0) o.push_back (d[k + 1] <= d[k] ? d[k] + 1 : d[k] - 1);
        }
        return o;
    }, "Ornamentation"); };
    melisma.onClick = [range] { range ([] (std::vector<int> d)
    {
        std::vector<int> o;
        for (size_t k = 0; k < d.size(); ++k)
        {
            o.push_back (d[k]);
            if (k + 1 < d.size())
            {
                const int nx = d[k + 1];
                if (nx == d[k]) { o.push_back (d[k] + 1); o.push_back (d[k]); }
                else { const int dir = nx > d[k] ? 1 : -1; o.push_back (d[k] - dir); o.push_back (d[k]); for (int x = d[k] + dir; x != nx; x += dir) o.push_back (x); }
            }
        }
        return o;
    }, "Melismatic expansion"); };
    simplify.onClick = [range] { range ([] (std::vector<int> d)
    {
        std::vector<int> o;
        for (int x : d)
        {
            if (! o.empty() && o.back() == x) continue;
            if (o.size() >= 2 && o[o.size() - 2] == x) { o.pop_back(); continue; }
            o.push_back (x);
        }
        return o;
    }, "Simplification"); };
    cadence.onClick = [this] {
        const Formula* fin = nullptr;
        for (auto& f : ed.variant().formulas) if (juce::String (f.function) == "Final cadence") { fin = &f; break; }
        if (! fin) return;
        auto degs = fin->degs;
        ed.edit ([degs] (Composition& c)
        {
            const int n = (int) c.events.size();
            const int k = (int) degs.size();
            if (n >= k && std::none_of (c.events.end() - k, c.events.end(), [] (const Event& e) { return e.pause; })) replaceRange (c, n - k, n - 1, degs);
            else insertDegrees (c, n, degs);
            c.events.back().ext = std::max (c.events.back().ext, 2);
        });
        ed.setStatus ("The score now ends with the final cadence of the mode.");
    };
}

void MelosTab::paint (juce::Graphics& g)
{
    g.fillAll (ui::panel);
    g.setColour (ui::lapis);
    g.setFont (uiFont (14.0f, true));
    g.drawText ("Formula library: " + U (modes()[(size_t) ed.comp().mode].shortGreek) + ", " + ed.variant().name,
                juce::Rectangle<int> (14, 10, 420, 24), juce::Justification::centredLeft, false);
}

void MelosTab::resized()
{
    auto r = getLocalBounds().reduced (14);
    auto left = r.removeFromLeft (juce::jmax (420, r.getWidth() / 2));
    left.removeFromTop (26);
    fnFilter.setBounds (left.removeFromTop (28).removeFromLeft (220));
    left.removeFromTop (8);
    formulaView.setBounds (left);
    formulas.setSize (formulaView.getMaximumVisibleWidth(), formulas.preferredHeight());
    r.removeFromLeft (20);
    composeHead.setBounds (r.removeFromTop (26));
    text.setBounds (r.removeFromTop (110));
    r.removeFromTop (8);
    auto row = r.removeFromTop (30);
    styleBox.setBounds (row.removeFromLeft (250)); row.removeFromLeft (8);
    r.removeFromTop (8);
    row = r.removeFromTop (30);
    compose.setBounds (row.removeFromLeft (190)); row.removeFromLeft (8); append.setBounds (row.removeFromLeft (170));
    r.removeFromTop (26);
    devHead.setBounds (r.removeFromTop (26));
    devHint.setBounds (r.removeFromTop (34));
    juce::TextButton* bs[] = { &stepUp, &stepDown, &ornament, &invert, &retro, &melisma, &simplify, &cadence };
    for (int i = 0; i < 8; i += 2)
    {
        row = r.removeFromTop (30);
        bs[i]->setBounds (row.removeFromLeft (row.getWidth() / 2 - 4));
        row.removeFromLeft (8);
        bs[i + 1]->setBounds (row);
        r.removeFromTop (6);
    }
}

void MelosTab::refresh() { formulas.refresh(); resized(); repaint(); }

// =====================================================================  parallage tab
ParallageTab::ParallageTab (PsaltikonEditor& e) : ladder (e, LadderView::Parallage), ed (e)
{
    addAndMakeVisible (ladder);
    for (auto* l : { &h1, &h2, &h3, &h4, &h5 }) { addAndMakeVisible (l); l->setFont (uiFont (14.0f, true)); l->setColour (juce::Label::textColourId, ui::lapis); }
    h1.setText ("Modal sequences (sung on the parallage syllables)", juce::dontSendNotification);
    h2.setText ("Exercises", juce::dontSendNotification);
    h3.setText ("Interval recognition: name the second note", juce::dontSendNotification);
    h4.setText ("Microtonal comparison", juce::dontSendNotification);
    h5.setText ("Compose with the parallage syllables", juce::dontSendNotification);
    addAndMakeVisible (sylEd); addAndMakeVisible (sylHint); addAndMakeVisible (sylHear); addAndMakeVisible (sylAdd);
    sylEd.setFont (uiFont (17.0f));
    sylEd.setText (U (u8"Πα Βου Γα Δι Κε Δι Γα Βου Γα Βου Πα"), false);
    sylHint.setFont (uiFont (12.0f));
    sylHint.setColour (juce::Label::textColourId, ui::mute);
    sylHint.setText (U (u8"Greek or Latin (Νη Πα Βου Γα Δι Κε Ζω / Ni Pa Vou ...). Each goes to the nearest octave; add ′ or ' for the upper octave, a comma for the lower."), juce::dontSendNotification);
    sylHear.onClick = [this] { std::vector<int> d; if (parseSyllables (d)) ed.playDegrees (d, true, {}, -1, true); };
    sylAdd.onClick = [this] {
        std::vector<int> d;
        if (! parseSyllables (d)) return;
        std::vector<juce::String> syl;
        for (int x : d) syl.push_back (U (noteGreek (x)));
        ed.edit ([d, syl] (Composition& c) { byz::insertDegrees (c, (int) c.events.size(), d, syl); });
        ed.setStatus ("Added " + juce::String ((int) d.size()) + " notes from the syllables.");
    };
    for (auto* b : { &asc, &desc, &both, &exGen, &exPlay, &exSend, &qNew, &qRepeat, &mA, &mB, &mAB, &tones }) addAndMakeVisible (b);
    asc.onClick = [this] { const int b = ed.variant().base; std::vector<int> d; for (int k = 0; k <= 7; ++k) d.push_back (b + k); ed.playDegrees (d, true); };
    desc.onClick = [this] { const int b = ed.variant().base; std::vector<int> d; for (int k = 7; k >= 0; --k) d.push_back (b + k); ed.playDegrees (d, true); };
    both.onClick = [this] { const int b = ed.variant().base; std::vector<int> d; for (int k = 0; k <= 7; ++k) d.push_back (b + k); for (int k = 6; k >= 0; --k) d.push_back (b + k); ed.playDegrees (d, true); };

    addAndMakeVisible (exKind); addAndMakeVisible (exLen); addAndMakeVisible (exText);
    exKind.addItem ("Scale of the mode", 1); exKind.addItem ("Stepwise melody", 2); exKind.addItem ("Leaps (3rds to 5ths)", 3); exKind.addItem ("Cadential formulas", 4);
    exKind.setSelectedId (2, juce::dontSendNotification);
    exLen.setRange (6, 24, 1); exLen.setValue (10, juce::dontSendNotification);
    exLen.setTextBoxStyle (juce::Slider::TextBoxRight, false, 40, 22);
    exText.setFont (byz::serifFont (20.0f));
    exText.setJustificationType (juce::Justification::topLeft);
    exGen.onClick = [this] {
        exercise = makeExercise (ed.variant(), exKind.getSelectedId() - 1, (int) exLen.getValue(), rng);
        juce::String g, l;
        for (int d : exercise) { g << U (degreeGreek (d)) << "  "; l << U (degreeLatin (d)) << "  "; }
        exText.setText (g + "\n" + l, juce::dontSendNotification);
    };
    exPlay.onClick = [this] { if (exercise.empty()) exGen.onClick(); ed.playDegrees (exercise, true, {}, -1, true); };
    exSend.onClick = [this] {
        if (exercise.empty()) exGen.onClick();
        std::vector<juce::String> syl;
        for (int d : exercise) syl.push_back (U (noteGreek (d)));
        auto ex = exercise;
        ed.edit ([ex, syl] (Composition& c) { if (! c.events.empty()) { Event p; p.pause = true; c.events.push_back (p); } insertDegrees (c, (int) c.events.size(), ex, syl); });
        ed.setStatus ("Exercise added to the score with its parallage syllables as text.");
    };

    for (int i = 0; i < 8; ++i)
    {
        auto* b = answers.add (new juce::TextButton());
        addAndMakeVisible (b);
        b->onClick = [this, i] {
            if (answered) return;
            const int guess = ed.variant().base + i;
            answered = true; ++asked;
            const int gen = ed.variant().genus;
            const int diff = moriaOf (qSecond, gen) - moriaOf (qFirst, gen);
            if (guess == qSecond) { ++right; qText.setText ("Right: " + U (degreeGreek (qSecond)) + ", " + juce::String (diff > 0 ? "+" : "") + juce::String (diff) + " moria from " + U (degreeGreek (qFirst)) + ".", juce::dontSendNotification); }
            else qText.setText ("It was " + U (degreeGreek (qSecond)) + " (" + juce::String (diff > 0 ? "+" : "") + juce::String (diff) + " moria from " + U (degreeGreek (qFirst)) + "), not " + U (degreeGreek (guess)) + ".", juce::dontSendNotification);
            qScore.setText (juce::String (right) + " / " + juce::String (asked), juce::dontSendNotification);
            ed.playDegrees ({ qFirst, qSecond }, true);
        };
    }
    addAndMakeVisible (qText); addAndMakeVisible (qScore);
    qText.setFont (uiFont (14.0f));
    qScore.setFont (uiFont (18.0f, true)); qScore.setJustificationType (juce::Justification::centredRight);
    qText.setText ("Press New question: the first note is sung on its syllable, the second on 'a'.", juce::dontSendNotification);
    qNew.onClick = [this] { newQuestion(); };
    qRepeat.onClick = [this] { playQuestion(); };

    for (auto* cb : { &mDeg, &mGenA, &mGenB }) addAndMakeVisible (cb);
    for (int d = 0; d < 7; ++d) mDeg.addItem (degName (d), d + 1);
    for (int gi = 0; gi < 4; ++gi) { mGenA.addItem (genusName (gi), gi + 1); mGenB.addItem (genusName (gi), gi + 1); }
    mDeg.setSelectedId (3, juce::dontSendNotification); mGenA.setSelectedId (1, juce::dontSendNotification); mGenB.setSelectedId (3, juce::dontSendNotification);
    addAndMakeVisible (mText);
    mText.setFont (uiFont (14.0f));
    mText.setJustificationType (juce::Justification::topLeft);
    auto updateM = [this] {
        const int d = mDeg.getSelectedId() - 1, a = mGenA.getSelectedId() - 1, b = mGenB.getSelectedId() - 1;
        const int ma = moriaOf (d, a), mb = moriaOf (d, b);
        mText.setText (U (degreeGreek (d)) + ": " + genusName (a) + " " + juce::String (ma) + " moria above Ni (" + juce::String (centsOfMoria (ma), 1) + U (u8"¢)") + ";  "
                       + genusName (b) + " " + juce::String (mb) + " moria (" + juce::String (centsOfMoria (mb), 1) + U (u8"¢)") + ".  Difference "
                       + juce::String (mb - ma) + " moria = " + juce::String (centsOfMoria (mb - ma), 1) + U (u8" cents."), juce::dontSendNotification);
    };
    for (auto* cb : { &mDeg, &mGenA, &mGenB }) cb->onChange = updateM;
    updateM();
    mA.onClick = [this] { ed.playDegrees ({ mDeg.getSelectedId() - 1 }, true, {}, mGenA.getSelectedId() - 1); };
    mB.onClick = [this] { ed.playDegrees ({ mDeg.getSelectedId() - 1 }, true, {}, mGenB.getSelectedId() - 1); };
    mAB.onClick = [this] {
        // two notes in different genera: build the timeline directly
        const int d = mDeg.getSelectedId() - 1;
        auto t = std::make_shared<Timeline>(); t->bpm = 60; t->total = 4.0;
        const int v = vowelIndex (parallageVowel (d));
        t->ctrls.push_back ({ 0.0, CPitch, (float) moriaOf (d, mGenA.getSelectedId() - 1), 0 });
        t->ctrls.push_back ({ 0.0, CVowel, (float) v, 0 });
        t->ctrls.push_back ({ 0.0, CArtic, (float) parallageConsonant (d), 0.5f });
        t->ctrls.push_back ({ 0.0, CLevel, 1.0f, 0.05f });
        t->ctrls.push_back ({ 0.0, CVibrato, 1.0f, 1.8f });
        t->ctrls.push_back ({ 1.8, CLevel, 0.0f, 0.1f });
        t->ctrls.push_back ({ 2.1, CPitch, (float) moriaOf (d, mGenB.getSelectedId() - 1), 0 });
        t->ctrls.push_back ({ 2.1, CArtic, (float) parallageConsonant (d), 0.5f });
        t->ctrls.push_back ({ 2.1, CLevel, 1.0f, 0.05f });
        t->ctrls.push_back ({ 2.1, CVibrato, 1.0f, 1.8f });
        t->ctrls.push_back ({ 3.9, CLevel, 0.0f, 0.3f });
        ed.proc.playPreview (t);
    };
    tones.onClick = [this] { ed.playDegrees ({ 0, 1, 2, 3 }, true, {}, Diatonic, false, 50); };
}

bool ParallageTab::parseSyllables (std::vector<int>& degs)
{
    const auto& c = ed.comp();
    int prev = c.events.empty() ? ed.variant().base : derive (c, false).deg.back();
    int bad = -1;
    if (parseParallage (sylEd.getText(), prev, degs, bad)) { sylHint.setColour (juce::Label::textColourId, ui::mute); return true; }
    auto toks = juce::StringArray::fromTokens (sylEd.getText(), " ", ""); toks.removeEmptyStrings();
    sylHint.setColour (juce::Label::textColourId, ui::cinnabar);
    sylHint.setText (bad >= 0 && bad < toks.size() ? "Not a parallage syllable: " + toks[bad] : juce::String ("Type some syllables first."), juce::dontSendNotification);
    return false;
}

void ParallageTab::newQuestion()
{
    const int b = ed.variant().base;
    std::uniform_int_distribution<int> u (0, 7);
    qFirst = b + u (rng);
    do qSecond = b + u (rng); while (qSecond == qFirst);
    answered = false;
    qText.setText ("Starting on " + U (degreeGreek (qFirst)) + ". Which note follows?", juce::dontSendNotification);
    playQuestion();
}

void ParallageTab::playQuestion()
{
    if (answered && asked == 0) return;
    ed.playDegrees ({ qFirst, qSecond }, false, { U (noteLatin (qFirst)), "a" }, -1, false, 60);
}

void ParallageTab::refresh()
{
    const auto& v = ed.variant();
    for (int i = 0; i < 8; ++i) answers[i]->setButtonText (U (degreeGreek (v.base + i)));
    ladder.lowDeg = v.base - 2; ladder.highDeg = v.base + 9;
    ladder.repaint();
}

void ParallageTab::paint (juce::Graphics& g) { g.fillAll (ui::panel); }

void ParallageTab::resized()
{
    auto r = getLocalBounds().reduced (14);
    ladder.setBounds (r.removeFromLeft (330));
    r.removeFromLeft (20);
    h1.setBounds (r.removeFromTop (24));
    auto row = r.removeFromTop (30);
    for (auto* b : { &asc, &desc, &both }) { b->setBounds (row.removeFromLeft (130)); row.removeFromLeft (8); }
    r.removeFromTop (14);
    h2.setBounds (r.removeFromTop (24));
    row = r.removeFromTop (30);
    exKind.setBounds (row.removeFromLeft (200)); row.removeFromLeft (8);
    exLen.setBounds (row.removeFromLeft (200));
    r.removeFromTop (6);
    row = r.removeFromTop (30);
    for (auto* b : { &exGen, &exPlay, &exSend }) { b->setBounds (row.removeFromLeft (150)); row.removeFromLeft (8); }
    exText.setBounds (r.removeFromTop (52));
    r.removeFromTop (4);
    h3.setBounds (r.removeFromTop (24));
    row = r.removeFromTop (30);
    qNew.setBounds (row.removeFromLeft (130)); row.removeFromLeft (8);
    qRepeat.setBounds (row.removeFromLeft (110)); row.removeFromLeft (8);
    qScore.setBounds (row.removeFromRight (90));
    r.removeFromTop (6);
    row = r.removeFromTop (34);
    const int bw = juce::jmax (50, (row.getWidth() - 7 * 6) / 8);
    for (auto* b : answers) { b->setBounds (row.removeFromLeft (bw)); row.removeFromLeft (6); }
    qText.setBounds (r.removeFromTop (28));
    r.removeFromTop (10);
    h4.setBounds (r.removeFromTop (24));
    row = r.removeFromTop (30);
    mDeg.setBounds (row.removeFromLeft (130)); row.removeFromLeft (8);
    mGenA.setBounds (row.removeFromLeft (150)); row.removeFromLeft (8);
    mGenB.setBounds (row.removeFromLeft (150));
    r.removeFromTop (6);
    row = r.removeFromTop (30);
    for (auto* b : { &mA, &mB, &mAB }) { b->setBounds (row.removeFromLeft (90)); row.removeFromLeft (8); }
    tones.setBounds (row.removeFromLeft (190));
    mText.setBounds (r.removeFromTop (40));
    h5.setBounds (r.removeFromTop (24));
    row = r.removeFromTop (30);
    sylAdd.setBounds (row.removeFromRight (120)); row.removeFromRight (6);
    sylHear.setBounds (row.removeFromRight (70)); row.removeFromRight (8);
    sylEd.setBounds (row);
    sylHint.setBounds (r.removeFromTop (34));
}

// =====================================================================  voice tab
VoiceTab::VoiceTab (PsaltikonEditor& e) : ed (e)
{
    auto& ap = ed.proc.apvts;
    for (auto* l : { &h1, &h2 }) { addAndMakeVisible (l); l->setFont (uiFont (15.0f, true)); l->setColour (juce::Label::textColourId, ui::lapis); }
    h1.setText ("Chanter", juce::dontSendNotification);
    h2.setText ("Ison", juce::dontSendNotification);
    auto mk = [&] (juce::OwnedArray<LabeledSlider>& arr, const char* id, const char* label)
    {
        auto* s = arr.add (new LabeledSlider());
        s->s.setSliderStyle (juce::Slider::LinearHorizontal);
        s->s.setTextBoxStyle (juce::Slider::TextBoxRight, false, 56, 22);
        s->s.setNumDecimalPlacesToDisplay (2);
        s->l.setText (label, juce::dontSendNotification);
        s->l.setFont (uiFont (13.0f));
        addAndMakeVisible (s->s); addAndMakeVisible (s->l);
        s->a = std::make_unique<SA> (ap, id, s->s);
        const bool isInt = juce::String (id) == "isonVoices" || juce::String (id) == "isonOctave";
        s->s.textFromValueFunction = [isInt] (double v) { return isInt ? juce::String (juce::roundToInt (v)) : juce::String (v, 2); };
        s->s.updateText();
    };
    mk (voiceSliders, "register", "Register (semitones)");
    mk (voiceSliders, "resonance", "Resonance");
    mk (voiceSliders, "intensity", "Intensity");
    mk (voiceSliders, "vibrato", "Vibrato depth");
    mk (voiceSliders, "vibRate", "Vibrato speed");
    mk (voiceSliders, "articulation", "Articulation");
    mk (voiceSliders, "breath", "Breath");
    mk (voiceSliders, "ornament", "Ornamentation");
    mk (voiceSliders, "melodyVol", "Melody volume");
    mk (voiceSliders, "reverb", "Church reverb");
    mk (isonSliders, "isonVol", "Ison volume");
    mk (isonSliders, "isonVoices", "Ison voices");
    mk (isonSliders, "isonOctave", "Ison octave");

    for (auto* l : { &profileL, &isonModeL, &isonVowelL, &isonNoteL }) { addAndMakeVisible (l); l->setFont (uiFont (13.0f)); }
    profileL.setText ("Voice", juce::dontSendNotification);
    isonModeL.setText ("Ison follows", juce::dontSendNotification);
    isonVowelL.setText ("Ison vowel", juce::dontSendNotification);
    isonNoteL.setText ("Manual ison note", juce::dontSendNotification);
    for (auto* cb : { &profile, &isonMode, &isonVowel, &isonNote }) addAndMakeVisible (cb);
    profile.addItemList (ap.getParameter ("profile")->getAllValueStrings(), 1);
    isonMode.addItemList (ap.getParameter ("isonMode")->getAllValueStrings(), 1);
    isonVowel.addItemList (ap.getParameter ("isonVowel")->getAllValueStrings(), 1);
    profileA = std::make_unique<CA> (ap, "profile", profile);
    isonModeA = std::make_unique<CA> (ap, "isonMode", isonMode);
    isonVowelA = std::make_unique<CA> (ap, "isonVowel", isonVowel);
    for (int d = -7; d <= 14; ++d) isonNote.addItem (degName (d), d + 100);
    isonNote.onChange = [this] {
        auto* p = ed.proc.apvts.getParameter ("isonNote");
        p->setValueNotifyingHost (p->convertTo0to1 ((float) (isonNote.getSelectedId() - 100)));
    };

    for (auto* t : { &attractions, &singPar, &hostSync, &drone }) addAndMakeVisible (t);
    attractionsA = std::make_unique<BA> (ap, "attractions", attractions);
    singParA = std::make_unique<BA> (ap, "singParallage", singPar);
    hostSyncA = std::make_unique<BA> (ap, "hostSync", hostSync);
    droneA = std::make_unique<BA> (ap, "drone", drone);
    attractions.onClick = [this] { ed.proc.compositionChanged(); };
    for (auto* s : voiceSliders) s->s.onDragEnd = [this] { ed.proc.compositionChanged(); };
    singPar.onClick = [this] { ed.proc.compositionChanged(); };

    for (auto* b : { &genIson, &clearIson, &testVoice }) addAndMakeVisible (b);
    genIson.onClick = [this] { ed.edit ([] (Composition& c) { autoIson (c); }); ed.setStatus ("Ison generated."); };
    clearIson.onClick = [this] { ed.edit ([] (Composition& c) { for (auto& ev : c.events) ev.ison = kNoIson; }); };
    testVoice.onClick = [this] {
        const auto& v = ed.variant();
        std::vector<int> d = v.apechemaMel;
        for (auto& f : v.formulas) if (juce::String (f.function) == "Final cadence") { d.insert (d.end(), f.degs.begin(), f.degs.end()); break; }
        juce::StringArray syl; syl.addTokens (U (v.apechema), "-", "");
        std::vector<juce::String> s (syl.begin(), syl.end());
        while (s.size() < d.size()) s.push_back (U (u8"ἀ"));
        ed.playDegrees (d, false, s, -1, true, 70);
    };

    addAndMakeVisible (note);
    note.setFont (uiFont (12.5f));
    note.setColour (juce::Label::textColourId, ui::mute);
    note.setJustificationType (juce::Justification::topLeft);
    note.setText ("The voices are physically modelled: a glottal pulse whose quality follows vocal effort, cycle-to-cycle jitter and shimmer, "
                  "a cascade vocal tract with the singer's formant and nasal colour, real consonants timed before the beat, breaths, "
                  "natural scoops and portamento measured in moria, and a church acoustic. MIDI input plays the chanter live: white keys are the degrees (middle C = Ni), black keys raise the "
                  "lower note by a diesis. In 'Live' ison mode, keys below C2 set the ison.", juce::dontSendNotification);
}

void VoiceTab::paint (juce::Graphics& g) { g.fillAll (ui::panel); }

void VoiceTab::resized()
{
    auto r = getLocalBounds().reduced (18);
    auto left = r.removeFromLeft (r.getWidth() / 2 - 10);
    r.removeFromLeft (20);
    h1.setBounds (left.removeFromTop (26));
    auto row = left.removeFromTop (30);
    profileL.setBounds (row.removeFromLeft (150)); profile.setBounds (row.removeFromLeft (260));
    left.removeFromTop (8);
    for (auto* s : voiceSliders) { row = left.removeFromTop (30); s->l.setBounds (row.removeFromLeft (150)); s->s.setBounds (row); left.removeFromTop (4); }
    left.removeFromTop (6);
    for (auto* t : { &attractions, &singPar, &hostSync }) t->setBounds (left.removeFromTop (26));
    left.removeFromTop (8);
    testVoice.setBounds (left.removeFromTop (30).removeFromLeft (200));

    h2.setBounds (r.removeFromTop (26));
    row = r.removeFromTop (30); isonModeL.setBounds (row.removeFromLeft (150)); isonMode.setBounds (row.removeFromLeft (240)); r.removeFromTop (6);
    row = r.removeFromTop (30); isonNoteL.setBounds (row.removeFromLeft (150)); isonNote.setBounds (row.removeFromLeft (240)); r.removeFromTop (6);
    row = r.removeFromTop (30); isonVowelL.setBounds (row.removeFromLeft (150)); isonVowel.setBounds (row.removeFromLeft (240)); r.removeFromTop (6);
    for (auto* s : isonSliders) { row = r.removeFromTop (30); s->l.setBounds (row.removeFromLeft (150)); s->s.setBounds (row); r.removeFromTop (4); }
    drone.setBounds (r.removeFromTop (26));
    r.removeFromTop (8);
    row = r.removeFromTop (30);
    genIson.setBounds (row.removeFromLeft (200)); row.removeFromLeft (8); clearIson.setBounds (row.removeFromLeft (190));
    r.removeFromTop (18);
    note.setBounds (r.removeFromTop (150));
}

void VoiceTab::refresh()
{
    const int d = (int) ed.proc.apvts.getParameter ("isonNote")->convertFrom0to1 (ed.proc.apvts.getParameter ("isonNote")->getValue());
    isonNote.setSelectedId (d + 100, juce::dontSendNotification);
}

// =====================================================================  editor
PsaltikonEditor::PsaltikonEditor (PsaltikonProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (root);
    root.painter = [this] (juce::Graphics& g) { paintHeader (g); };
    root.addAndMakeVisible (zoomBox);
    zoomBox.addItem ("Zoom 60%", 60); zoomBox.addItem ("Zoom 70%", 70); zoomBox.addItem ("Zoom 80%", 80); zoomBox.addItem ("Zoom 90%", 90);
    zoomBox.addItem ("Zoom 100%", 100); zoomBox.addItem ("Zoom 115%", 115); zoomBox.addItem ("Zoom 130%", 130);
    zoomBox.setTooltip ("Size of the whole window");
    zoomBox.onChange = [this] {
        const float z = (float) zoomBox.getSelectedId() / 100.0f;
        if (z > 0) setSize (juce::roundToInt (kBaseW * z), juce::roundToInt (kBaseH * z));
    };
    root.addAndMakeVisible (brand);
    brand.setText ("Psaltikon", juce::dontSendNotification);
    brand.setFont (byz::serifFont (28.0f));
    brand.setColour (juce::Label::textColourId, juce::Colours::white);

    for (int m = 0; m < (int) modes().size(); ++m) modeBox.addItem (U (modes()[(size_t) m].shortGreek) + "   " + modes()[(size_t) m].english, m + 1);
    modeBox.onChange = [this] { modeChanged(); };
    variantBox.onChange = [this] {
        const int v = variantBox.getSelectedId() - 1;
        if (v >= 0 && v != comp().variant) edit ([v] (Composition& c) { c.variant = v; });
    };
    root.addAndMakeVisible (modeBox); root.addAndMakeVisible (variantBox);
    for (auto* b : { &playB, &playSelB, &stopB }) root.addAndMakeVisible (b);
    playB.onClick = [this] { togglePlay (false); };
    playSelB.onClick = [this] { togglePlay (true); };
    stopB.onClick = [this] { proc.stopPlayback(); };
    root.addAndMakeVisible (tempo); root.addAndMakeVisible (tempoL);
    tempo.setRange (30, 200, 1);
    tempo.setSliderStyle (juce::Slider::LinearHorizontal);
    tempo.setTextBoxStyle (juce::Slider::TextBoxRight, false, 44, 22);
    tempo.setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
    tempo.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    tempo.onValueChange = [this] { comp().bpm = tempo.getValue(); tempoL.setText (U (u8"Χρόνος · ") + tempoName (comp().bpm), juce::dontSendNotification); proc.compositionChanged(); if (scoreTab) scoreTab->score.repaint(); };
    tempoL.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.85f));
    tempoL.setFont (byz::serifFont (14.0f));
    tempoL.setMinimumHorizontalScale (0.7f);
    tempoL.setJustificationType (juce::Justification::centredRight);

    scoreTab = new ScoreTab (*this);
    modeTab = new ModeTab (*this);
    melosTab = new MelosTab (*this);
    parTab = new ParallageTab (*this);
    voiceTab = new VoiceTab (*this);
    tabs.addTab (U (u8"Score · Σύνθεσις"), ui::panel, scoreTab, true);
    tabs.addTab (U (u8"Mode · Ἦχος"), ui::panel, modeTab, true);
    tabs.addTab (U (u8"Melos · Μέλος"), ui::panel, melosTab, true);
    tabs.addTab (U (u8"Parallage · Παραλλαγή"), ui::panel, parTab, true);
    tabs.addTab (U (u8"Voice & Ison · Ἴσον"), ui::panel, voiceTab, true);
    tabs.setTabBarDepth (38);
    tabs.setOutline (0);
    root.addAndMakeVisible (tabs);

    // text editors were filled before they inherited this look and feel: recolour their text
    std::function<void (juce::Component&)> recolour = [&recolour] (juce::Component& c)
    {
        if (auto* te = dynamic_cast<juce::TextEditor*> (&c)) { te->setColour (juce::TextEditor::textColourId, ui::inkC); te->applyColourToAllText (ui::inkC); }
        for (auto* ch : c.getChildren()) recolour (*ch);
    };
    for (auto* t : std::initializer_list<juce::Component*> { scoreTab, modeTab, melosTab, parTab, voiceTab }) recolour (*t);
    // the palette lives inside a viewport and is reached through it
    recolour (scoreTab->lyric);

    setResizable (true, true);
    setResizeLimits (kBaseW / 2, kBaseH / 2, kBaseW * 2, kBaseH * 2);
    // open at a size that fits the screen (laptops included), never larger than 100%
    float fit = 1.0f;
    if (auto* d = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
        fit = juce::jlimit (0.5f, 1.0f, std::min ((float) d->userArea.getWidth() * 0.9f / kBaseW, (float) d->userArea.getHeight() * 0.82f / kBaseH));
    setSize (juce::roundToInt (kBaseW * fit), juce::roundToInt (kBaseH * fit));
    refreshAll();
    if (auto* t = std::getenv ("PSALTIKON_TAB")) tabs.setCurrentTabIndex (juce::String (t).getIntValue());   // used by screenshot tests
    startTimerHz (30);
}

PsaltikonEditor::~PsaltikonEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void PsaltikonEditor::paint (juce::Graphics& g)
{
    g.fillAll (ui::panel);
}

void PsaltikonEditor::paintHeader (juce::Graphics& g)
{
    const int w = root.getWidth();
    g.fillAll (ui::panel);
    g.setGradientFill (juce::ColourGradient (ui::lapis, 0, 0, ui::lapisDeep, (float) w, 0, false));
    g.fillRect (0, 0, w, 56);
    g.setColour (ui::gold);
    g.fillRect (0, 55, w, 1);
}

void PsaltikonEditor::resized()
{
    // scale so the logical layout (at least the base size in its smaller dimension) fits the window
    uiScale = juce::jlimit (0.45f, 2.5f, std::min ((float) getWidth() / kBaseW, (float) getHeight() / kBaseH));
    root.setTransform (juce::AffineTransform::scale (uiScale));
    root.setBounds (0, 0, juce::roundToInt ((float) getWidth() / uiScale), juce::roundToInt ((float) getHeight() / uiScale));
    zoomBox.setSelectedId (0, juce::dontSendNotification);
    zoomBox.setText ("Zoom " + juce::String (juce::roundToInt (uiScale * 100.0f)) + "%", juce::dontSendNotification);
    layoutRoot();
}

void PsaltikonEditor::layoutRoot()
{
    auto r = root.getLocalBounds();
    auto top = r.removeFromTop (56).reduced (14, 12);
    brand.setBounds (top.removeFromLeft (140));
    modeBox.setBounds (top.removeFromLeft (240)); top.removeFromLeft (8);
    variantBox.setBounds (top.removeFromLeft (180)); top.removeFromLeft (14);
    playB.setBounds (top.removeFromLeft (66)); top.removeFromLeft (6);
    playSelB.setBounds (top.removeFromLeft (146)); top.removeFromLeft (6);
    stopB.setBounds (top.removeFromLeft (60)); top.removeFromLeft (10);
    zoomBox.setBounds (top.removeFromRight (112)); top.removeFromRight (8);
    tempo.setBounds (top.removeFromRight (128));
    tempoL.setBounds (top);
    tabs.setBounds (r);
}

void PsaltikonEditor::setStatus (const juce::String& s) { if (scoreTab) scoreTab->status.setText (s, juce::dontSendNotification); }

void PsaltikonEditor::select (int from, int to)
{
    const int n = (int) comp().events.size();
    if (from < 0 || n == 0) { selFrom = selTo = -1; }
    else { selFrom = juce::jlimit (0, n - 1, from); selTo = juce::jlimit (0, n - 1, to); }
    if (scoreTab) { scoreTab->syncFromSelection(); scoreTab->score.repaint(); }
}

void PsaltikonEditor::edit (const std::function<void (Composition&)>& fn, bool keepSelection)
{
    proc.pushHistory();
    fn (proc.comp);
    if (! keepSelection) selFrom = selTo = -1;
    proc.compositionChanged();
    refreshAll();
}

void PsaltikonEditor::refreshAll()
{
    lastVersion = proc.compVersion.load();
    auto& c = comp();
    modeBox.setSelectedId (c.mode + 1, juce::dontSendNotification);
    variantBox.clear (juce::dontSendNotification);
    const auto& md = modes()[(size_t) c.mode];
    for (int v = 0; v < (int) md.variants.size(); ++v) variantBox.addItem (md.variants[(size_t) v].name, v + 1);
    variantBox.setSelectedId (c.variant + 1, juce::dontSendNotification);
    tempo.setValue (c.bpm, juce::dontSendNotification);
    tempoL.setText (U (u8"Χρόνος · ") + tempoName (c.bpm), juce::dontSendNotification);
    const int n = (int) c.events.size();
    if (selFrom >= n || selTo >= n) { selFrom = selTo = n > 0 ? n - 1 : -1; }
    if (scoreTab)
    {
        if (! scoreTab->title.hasKeyboardFocus (true) && scoreTab->title.getText() != c.title)
        {
            scoreTab->title.setText (c.title, false);
            scoreTab->title.applyFontToAllText (uiFont (16.0f));
        }
        scoreTab->score.relayout();
        scoreTab->syncFromSelection();
    }
    if (modeTab) modeTab->refresh();
    if (melosTab) melosTab->refresh();
    if (parTab) parTab->refresh();
    if (voiceTab) voiceTab->refresh();
}

void PsaltikonEditor::modeChanged()
{
    const int m = modeBox.getSelectedId() - 1;
    if (m < 0 || m == comp().mode) return;
    edit ([m] (Composition& c)
    {
        const bool wasEmpty = c.events.empty();
        const int oldBase = c.var().base;
        c.mode = m; c.variant = 0;
        // keep the melody's relation to the base: move the start by the change of base
        c.startDeg += c.var().base - oldBase;
        if (wasEmpty) c.startDeg = c.var().base;
        for (auto& e : c.events) if (e.ison != kNoIson) e.ison += c.var().base - oldBase;
    });
}

void PsaltikonEditor::insertNeume (int delta)
{
    auto& c = comp();
    const bool replace = scoreTab && scoreTab->replaceMode.getToggleState() && selFrom >= 0 && ! c.events[(size_t) selLo()].pause;
    const bool keep = scoreTab && scoreTab->keepPitches.getToggleState();
    const int asc = scoreTab ? scoreTab->ascent.getSelectedId() - 1 : 0;
    if (replace)
    {
        const int i = selLo();
        edit ([i, delta, keep, asc] (Composition& cc)
        {
            const int old = cc.events[(size_t) i].delta;
            cc.events[(size_t) i].delta = delta;
            if (delta == 1) cc.events[(size_t) i].ascentGlyph = asc;
            if (keep)
                for (int j = i + 1; j < (int) cc.events.size(); ++j)
                    if (! cc.events[(size_t) j].pause) { cc.events[(size_t) j].delta = juce::jlimit (-7, 7, cc.events[(size_t) j].delta + old - delta); break; }
        });
        return;
    }
    const int at = selFrom >= 0 ? selHi() + 1 : (int) c.events.size();
    edit ([at, delta, asc] (Composition& cc)
    {
        Event e; e.delta = delta; if (delta == 1) e.ascentGlyph = asc;
        // keep the next note's pitch when inserting in the middle
        if (at < (int) cc.events.size())
            for (int j = at; j < (int) cc.events.size(); ++j)
                if (! cc.events[(size_t) j].pause) { cc.events[(size_t) j].delta = juce::jlimit (-7, 7, cc.events[(size_t) j].delta - delta); break; }
        cc.events.insert (cc.events.begin() + at, e);
    });
    select (at, at);
    if (scoreTab) scoreTab->score.ensureVisible (at);
    // let the user hear the new note
    auto d = derive (comp(), true);
    if (! proc.isPlaying() || proc.previewing()) playDegrees ({ d.deg[(size_t) at] }, true, {}, d.genus[(size_t) at], false, 110);
}

void PsaltikonEditor::insertDegrees (const std::vector<int>& degs)
{
    const int at = selFrom >= 0 ? selHi() + 1 : (int) comp().events.size();
    edit ([at, degs] (Composition& c) { byz::insertDegrees (c, at, degs); });
    select (at, at + (int) degs.size() - 1);
    setStatus ("Inserted " + juce::String ((int) degs.size()) + " notes. Use the Text tools to add syllables.");
}

void PsaltikonEditor::playDegrees (const std::vector<int>& degs, bool parallage, std::vector<juce::String> syl, int genusOverride, bool withIson, double bpm)
{
    if (degs.empty()) return;
    auto t = sequenceTimeline (variant(), degs, bpm > 0 ? bpm : std::max (60.0, comp().bpm), parallage, syl, genusOverride,
                               withIson ? variant().base : kNoIson, degs.size() == 1 ? 1.6 : 2.0);
    if (! withIson) t->ison.clear();
    proc.playPreview (t);
}

void PsaltikonEditor::togglePlay (bool fromSelection)
{
    if (proc.isPlaying() && ! proc.previewing()) { proc.stopPlayback(); return; }
    proc.playScore (fromSelection && selFrom >= 0 ? selLo() : 0);
}

void PsaltikonEditor::timerCallback()
{
    if (proc.compVersion.load() != lastVersion) refreshAll();
    const bool playing = proc.isPlaying() && ! proc.previewing();
    if (playing != lastPlaying) { playB.setButtonText (playing ? "Pause" : "Play"); lastPlaying = playing; }
    const int ev = playing ? proc.playingEvent() : -1;
    if (ev != lastEv && scoreTab)
    {
        lastEv = ev;
        scoreTab->score.playEv = ev;
        if (ev >= 0) scoreTab->score.ensureVisible (ev);
        scoreTab->score.repaint();
    }
}
