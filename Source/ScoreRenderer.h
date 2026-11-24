// Psaltikon — engraving of Byzantine neumatic notation: quantity neumes, time signs,
// expression signs (hypostases), fthoras, accidentals, ison indications, martyriai and
// the text underlay, with automatic spacing and line wrapping.
// Glyphs are drawn as vector paths (simplified forms of the printed Chrysanthine shapes).
#pragma once
#include <JuceHeader.h>
#include "Composition.h"

namespace byz
{
struct ScoreStyle
{
    juce::Colour paper { 0xffffffff }, ink { 0xff15151a }, red { 0xffb3261e }, faint { 0xff7a7a85 };
    juce::Colour select { 0x40c69a2b }, cursor { 0x3317285e };
    float scale = 1.25f;
    bool showParallage = true;
    bool header = true;
};

inline juce::Font serifFont (float size, bool italic = false)
{
    return juce::Font (juce::FontOptions ("Times New Roman", size, italic ? juce::Font::italic : juce::Font::plain));
}

inline juce::String utf8 (const char* s) { return juce::String (juce::CharPointer_UTF8 (s)); }
inline juce::String utf8 (const std::string& s) { return juce::String (juce::CharPointer_UTF8 (s.c_str())); }

// ---- glyph geometry -------------------------------------------------------
inline float neumeWidth (const Event& e)
{
    if (e.pause) return 14;
    switch (e.delta)
    {
        case 0: return 20;
        case 1: return e.ascentGlyph == AscPetasti ? 26.f : (e.ascentGlyph == AscKentimata ? 18.f : 28.f);
        case 2: case 3: case 4: case 5: case 6: case 7: return 30;
        case -1: return 12; case -2: return 22; case -3: return 34; case -4: return 24;
        case -5: return 36; case -6: return 46; case -7: return 58;
        default: return 28;
    }
}

struct Pen
{
    juce::Graphics& g; float x, y, s;
    juce::Point<float> p (float px, float py) const { return { x + px * s, y + py * s }; }
    void stroke (const juce::Path& path, float w) const { g.strokePath (path, juce::PathStrokeType (w * s, juce::PathStrokeType::curved, juce::PathStrokeType::rounded)); }
    void dot (float px, float py, float r) const { g.fillEllipse (x + (px - r) * s, y + (py - r) * s, 2 * r * s, 2 * r * s); }
    juce::Path quad (std::initializer_list<float> v) const
    {
        // M x y Q cx cy x y [Q cx cy x y ...]
        juce::Path pa; auto it = v.begin();
        float a = *it++, b = *it++; pa.startNewSubPath (p (a, b));
        while (it != v.end()) { float cx = *it++, cy = *it++, ex = *it++, ey = *it++; pa.quadraticTo (p (cx, cy), p (ex, ey)); }
        return pa;
    }
    juce::Path lines (std::initializer_list<float> v) const
    {
        juce::Path pa; auto it = v.begin();
        float a = *it++, b = *it++; pa.startNewSubPath (p (a, b));
        while (it != v.end()) { float ex = *it++, ey = *it++; pa.lineTo (p (ex, ey)); }
        return pa;
    }
};

inline void glyphIson (const Pen& P, float ox) { P.stroke (P.quad ({ ox, -3, ox + 7, -14, ox + 15, -7, ox + 18, -4, ox + 19, 0 }), 3.2f); }
inline void glyphOligon (const Pen& P, float ox)
{
    juce::Path pa; pa.startNewSubPath (P.p (ox, -3)); pa.lineTo (P.p (ox + 26, -8)); pa.lineTo (P.p (ox + 28, -5)); pa.lineTo (P.p (ox + 2, 1)); pa.closeSubPath();
    P.g.fillPath (pa);
}
inline void glyphPetasti (const Pen& P, float ox)
{
    P.stroke (P.quad ({ ox, -14, ox + 1, -1, ox + 12, -2, ox + 22, -3, ox + 24, -14 }), 3.0f);
    P.stroke (P.lines ({ ox + 24, -14, ox + 26, -17 }), 2.2f);
}
inline void glyphKentimata (const Pen& P, float ox) { P.dot (ox + 4, -4, 3.0f); P.dot (ox + 13, -11, 3.0f); }
inline void glyphApostrophos (const Pen& P, float ox)
{
    P.dot (ox + 7, -11, 3.6f);
    P.stroke (P.quad ({ ox + 10, -10, ox + 10, -2, ox + 2, 1 }), 2.6f);
}
inline void glyphElafron (const Pen& P, float ox) { P.stroke (P.quad ({ ox, -4, ox + 10, -17, ox + 21, -6 }), 3.2f); P.dot (ox, -4, 2.6f); }
inline void glyphChamili (const Pen& P, float ox)
{
    P.stroke (P.quad ({ ox, -2, ox + 11, -20, ox + 22, -2 }), 3.2f);
    P.stroke (P.lines ({ ox, -2, ox + 22, -2 }), 1.6f);
}
inline void glyphYpsili (const Pen& P, float ox, float oy) { P.stroke (P.lines ({ ox, oy, ox + 6, oy - 15 }), 3.6f); }

inline void drawQuantity (const Pen& P, const Event& e)
{
    if (e.pause) // leimma (rest)
    {
        P.stroke (P.lines ({ 4, 0, 4, -16 }), 2.4f);
        P.stroke (P.quad ({ 4, -16, 10, -18, 11, -12 }), 2.0f);
        return;
    }
    switch (e.delta)
    {
        case 0: glyphIson (P, 0); break;
        case 1:
            if (e.ascentGlyph == AscPetasti) glyphPetasti (P, 0);
            else if (e.ascentGlyph == AscKentimata) glyphKentimata (P, 0);
            else glyphOligon (P, 0);
            break;
        case 2: glyphOligon (P, 0); P.dot (14, -15, 3.4f); break;
        case 3: glyphOligon (P, 0); P.dot (9, -15, 3.0f); P.dot (19, -18, 3.0f); break;
        case 4: glyphOligon (P, 0); glyphYpsili (P, 21, -8); break;
        case 5: glyphOligon (P, 0); glyphYpsili (P, 11, -6); break;
        case 6: glyphOligon (P, 0); glyphYpsili (P, 1, -4); break;
        case 7: glyphOligon (P, 0); glyphYpsili (P, 21, -8); P.dot (10, -15, 3.2f); break;
        case -1: glyphApostrophos (P, 0); break;
        case -2: glyphElafron (P, 0); break;
        case -3: glyphElafron (P, 0); glyphApostrophos (P, 22); break;
        case -4: glyphChamili (P, 0); break;
        case -5: glyphChamili (P, 0); glyphApostrophos (P, 24); break;
        case -6: glyphChamili (P, 0); glyphElafron (P, 24); break;
        case -7: glyphChamili (P, 0); glyphElafron (P, 24); glyphApostrophos (P, 46); break;
        default: glyphOligon (P, 0); break;
    }
}

inline void drawGorgon (const Pen& P, float ox, float oy) { P.stroke (P.lines ({ ox, oy, ox + 5, oy - 8, ox + 9, oy - 5 }), 2.2f); }

inline void drawTimeAndExpression (const Pen& P, const Event& e, float gw)
{
    float tx = gw - 4;
    if (e.time == Gorgon) drawGorgon (P, tx, -22);
    else if (e.time == Digorgon) { drawGorgon (P, tx - 4, -22); drawGorgon (P, tx + 4, -22); }
    else if (e.time == Argon) { drawGorgon (P, tx, -22); P.dot (tx - 4, -26, 2.0f); }
    if (e.klasma) P.stroke (P.quad ({ tx + (e.time ? 12.f : 0.f), -18, tx + (e.time ? 17.f : 5.f), -24, tx + (e.time ? 22.f : 10.f), -18 }), 2.0f);
    for (int k = 0; k < e.ext; ++k) P.dot (gw * 0.5f - (e.ext - 1) * 3.5f + k * 7.0f, 9, 2.5f);
    switch (e.expr)
    {
        case Vareia:     P.stroke (P.lines ({ -6, 3, -3, -18 }), 2.4f); break;
        case Psifiston:  P.stroke (P.quad ({ gw * 0.5f, 6, gw * 0.5f - 3, 11, gw * 0.5f, 16 }), 2.2f); break;
        case Omalon:     P.stroke (P.quad ({ gw * 0.5f - 9, 12, gw * 0.5f - 6, 7, gw * 0.5f - 3, 12, gw * 0.5f, 17, gw * 0.5f + 3, 12, gw * 0.5f + 6, 7, gw * 0.5f + 9, 12 }), 1.8f); break;
        case Antikenoma: P.stroke (P.quad ({ gw * 0.5f - 6, 8, gw * 0.5f, 17, gw * 0.5f + 6, 8 }), 2.2f); break;
        case Eteron:
        {
            juce::Path pa = P.quad ({ gw * 0.5f, 12, gw * 0.5f + 14, 22, gw * 0.5f + 28, 12 });
            juce::Path dashed; float dl[] = { 3.0f * P.s, 3.0f * P.s };
            juce::PathStrokeType (1.6f * P.s).createDashedStroke (dashed, pa, dl, 2);
            P.g.fillPath (dashed);
            break;
        }
        default: break;
    }
}

inline void drawFthora (const Pen& P, int genus, float ox)
{
    switch (genus)
    {
        case Diatonic:      P.stroke (P.quad ({ ox + 9, -44, ox, -46, ox + 1, -38, ox + 2, -32, ox + 9, -34 }), 2.0f); P.stroke (P.lines ({ ox + 9, -34, ox + 12, -42 }), 2.0f); break;
        case SoftChromatic: P.dot (ox + 5, -44, 3.0f); P.dot (ox + 5, -35, 3.0f); P.stroke (P.lines ({ ox + 5, -44, ox + 5, -35 }), 1.4f); break;
        case HardChromatic: P.stroke (P.lines ({ ox, -46, ox + 10, -34 }), 2.0f); P.stroke (P.lines ({ ox + 10, -46, ox, -34 }), 2.0f); P.dot (ox + 5, -48, 1.8f); break;
        case Enharmonic:    P.g.drawEllipse (P.x + (ox) * P.s, P.y - 45 * P.s, 10 * P.s, 10 * P.s, 1.8f * P.s); P.stroke (P.lines ({ ox - 1, -33, ox + 11, -47 }), 1.6f); break;
        default: break;
    }
}

inline void drawGenusSign (const Pen& P, int genus, float ox, float oy)
{
    switch (genus)
    {
        case Diatonic:      P.stroke (P.quad ({ ox, oy, ox + 5, oy + 6, ox + 10, oy }), 1.8f); break;
        case SoftChromatic: P.stroke (P.quad ({ ox, oy, ox + 3, oy + 5, ox + 5, oy }), 1.6f); P.stroke (P.quad ({ ox + 5, oy, ox + 8, oy + 5, ox + 10, oy }), 1.6f); break;
        case HardChromatic: P.stroke (P.lines ({ ox, oy - 2, ox + 10, oy + 6 }), 1.6f); P.stroke (P.lines ({ ox + 10, oy - 2, ox, oy + 6 }), 1.6f); break;
        case Enharmonic:    P.stroke (P.lines ({ ox, oy + 6, ox + 10, oy - 2 }), 1.6f); P.dot (ox + 2, oy, 1.4f); P.dot (ox + 8, oy + 5, 1.4f); break;
        default: break;
    }
}

inline void drawMartyria (juce::Graphics& g, float x, float base, float s, int deg, int genus, juce::Colour red)
{
    g.setColour (red);
    g.setFont (serifFont (22 * s, true));
    juce::String t = utf8 (martyriaLetter (deg)) + utf8 (octaveMark (deg));
    g.drawText (t, juce::Rectangle<float> (x, base - 26 * s, 22 * s, 24 * s), juce::Justification::centred, false);
    Pen P { g, x + 6 * s, base + 2 * s, s };
    drawGenusSign (P, genus, 0, 0);
}

// ---- layout ---------------------------------------------------------------
struct ScoreLayout
{
    struct Cell { int ev; float x, y, w, gw; };
    std::vector<Cell> cells;
    float headerH = 0, rowH = 0, height = 0, width = 0;
    int rows = 0;
    std::vector<int> rowFirstCell;
};

inline float textW (const juce::Font& f, const juce::String& s) { return juce::GlyphArrangement::getStringWidth (f, s); }

inline ScoreLayout layoutScore (const Composition& c, float width, const ScoreStyle& st)
{
    ScoreLayout L;
    const float s = st.scale, margin = 24 * s;
    L.width = width;
    L.headerH = st.header ? 118 * s : 16 * s;
    L.rowH = (st.showParallage ? 158 : 136) * s;
    auto lf = serifFont (20 * s);
    float x = margin + 40 * s, y = L.headerH;   // first line leaves room for the start martyria
    L.rows = 1; L.rowFirstCell.push_back (0);
    for (int i = 0; i < (int) c.events.size(); ++i)
    {
        auto& e = c.events[(size_t) i];
        const float gw = neumeWidth (e);
        float w = std::max ({ gw * s + 14 * s, textW (lf, e.lyric.trim() == "_" ? juce::String() : e.lyric) + 12 * s, 28 * s });
        if (e.martyria) w += 28 * s;
        if (e.expr == Vareia) w += 6 * s;
        if (x + w > width - margin && x > margin + 1)
        {
            x = margin; y += L.rowH; ++L.rows; L.rowFirstCell.push_back ((int) L.cells.size());
        }
        L.cells.push_back ({ i, x, y, w, gw });
        x += w;
    }
    L.height = y + L.rowH + 20 * s;
    return L;
}

inline int hitTest (const ScoreLayout& L, juce::Point<float> p)
{
    for (auto& cell : L.cells)
        if (p.x >= cell.x && p.x < cell.x + cell.w && p.y >= cell.y && p.y < cell.y + L.rowH) return cell.ev;
    // past the last cell of a row -> nearest in that row
    int best = -1; float bd = 1e9f;
    for (auto& cell : L.cells)
        if (p.y >= cell.y && p.y < cell.y + L.rowH) { float d = std::abs (p.x - (cell.x + cell.w * 0.5f)); if (d < bd) { bd = d; best = cell.ev; } }
    return best;
}

inline juce::String tempoName (double bpm)
{
    if (bpm < 60) return utf8 (u8"Ἀργὸς");
    if (bpm < 100) return utf8 (u8"Μέτριος");
    if (bpm < 140) return utf8 (u8"Ταχύς");
    return utf8 (u8"Ταχύτατος");
}

// ---- painting -------------------------------------------------------------
inline void paintScore (juce::Graphics& g, const Composition& c, const ScoreLayout& L, const ScoreStyle& st,
                        int selFrom, int selTo, int playEv, int firstRow = 0, int lastRow = 1 << 20, float yOffset = 0)
{
    const float s = st.scale;
    const auto& v = c.var();
    auto d = derive (c, true);
    const float margin = 24 * s;

    auto rowVisible = [&] (float y) { int r = (int) std::round ((y - L.headerH) / L.rowH); return r >= firstRow && r <= lastRow; };

    if (st.header && firstRow == 0)
    {
        g.setColour (st.ink);
        g.setFont (serifFont (28 * s));
        g.drawText (c.title, juce::Rectangle<float> (0, 14 * s + yOffset, L.width, 34 * s), juce::Justification::centred, true);
        const auto& md = modes()[(size_t) c.mode];
        juce::String ml = utf8 (md.shortGreek) + "   ";
        g.setFont (serifFont (20 * s));
        auto mlw = textW (serifFont (20 * s), ml);
        juce::String ap = utf8 (v.apechema).replace ("-", "");
        auto apw = textW (serifFont (20 * s, true), ap);
        float cx = (L.width - (mlw + apw + 40 * s)) * 0.5f, cy = 58 * s + yOffset;
        g.drawText (ml, juce::Rectangle<float> (cx, cy, mlw, 26 * s), juce::Justification::centredLeft, false);
        g.setColour (st.red);
        g.setFont (serifFont (20 * s, true));
        g.drawText (ap, juce::Rectangle<float> (cx + mlw, cy, apw, 26 * s), juce::Justification::centredLeft, false);
        drawMartyria (g, cx + mlw + apw + 8 * s, cy + 24 * s, s, v.base, v.genus, st.red);
        g.setColour (st.faint);
        g.setFont (serifFont (14 * s, true));
        g.drawText (utf8 (u8"Χρόνος: ") + tempoName (c.bpm) + "  (" + juce::String ((int) c.bpm) + ")",
                    juce::Rectangle<float> (0, 86 * s + yOffset, L.width, 20 * s), juce::Justification::centred, false);
    }

    // start martyria at the head of the first line
    if (rowVisible (L.headerH) && ! c.events.empty())
        drawMartyria (g, margin, L.headerH - firstRow * L.rowH + 78 * s + yOffset, s, c.startDeg, v.genus, st.red);
    else if (c.events.empty() && firstRow == 0)
    {
        drawMartyria (g, margin, L.headerH + 78 * s + yOffset, s, c.startDeg, v.genus, st.red);
        g.setColour (st.faint);
        g.setFont (serifFont (16 * s, true));
        g.drawText ("Empty score: click a neume in the palette, or press 0-7 (Shift for descending).",
                    juce::Rectangle<float> (margin + 40 * s, L.headerH + 50 * s + yOffset, L.width, 30 * s), juce::Justification::centredLeft, false);
    }

    auto pf = serifFont (13 * s, true), lf = serifFont (20 * s);
    juce::String lastLyric;
    for (size_t k = 0; k < L.cells.size(); ++k)
    {
        auto& cell = L.cells[k];
        if (! rowVisible (cell.y)) continue;
        const float y = cell.y - firstRow * L.rowH + yOffset;
        const int i = cell.ev;
        auto& e = c.events[(size_t) i];
        const bool sel = i >= std::min (selFrom, selTo) && i <= std::max (selFrom, selTo) && selFrom >= 0;
        if (sel) { g.setColour (st.select); g.fillRoundedRectangle (cell.x, y + 6 * s, cell.w, L.rowH - 12 * s, 6 * s); }
        if (i == playEv) { g.setColour (st.cursor); g.fillRoundedRectangle (cell.x, y + 6 * s, cell.w, L.rowH - 12 * s, 6 * s); }

        const float base = y + 78 * s;
        const float gx = cell.x + (e.expr == Vareia ? 6 * s : 0) + 6 * s;
        g.setColour (st.ink);
        Pen P { g, gx, base, s };
        drawQuantity (P, e);
        if (! e.pause) drawTimeAndExpression (P, e, cell.gw);
        else for (int q = 0; q < e.ext; ++q) P.dot (7 - (e.ext - 1) * 3.5f + q * 7.0f, 9, 2.5f);

        g.setColour (st.red);
        if (e.fthora >= 0) drawFthora (P, e.fthora, cell.gw * 0.5f - 5);
        if (e.accidental < 0) P.stroke (P.lines ({ cell.gw * 0.5f - 4, -30, cell.gw * 0.5f, -24, cell.gw * 0.5f + 4, -30 }), 2.0f);
        if (e.accidental > 0) P.stroke (P.lines ({ cell.gw * 0.5f - 4, -24, cell.gw * 0.5f, -30, cell.gw * 0.5f + 4, -24 }), 2.0f);
        if (e.ison != kNoIson)
        {
            g.setFont (serifFont (14 * s, true));
            juce::String it = "(" + utf8 (martyriaLetter (e.ison)) + utf8 (octaveMark (e.ison)) + ")";
            g.drawText (it, juce::Rectangle<float> (cell.x, y + 8 * s, cell.w, 16 * s), juce::Justification::centredLeft, false);
        }
        if (e.martyria) drawMartyria (g, cell.x + cell.w - 28 * s, base, s, d.deg[(size_t) i], d.genus[(size_t) i], st.red);

        // text underlay
        g.setColour (st.ink);
        g.setFont (lf);
        if (! e.pause)
        {
            if (! isContinuation (e.lyric))
            {
                g.drawText (e.lyric, juce::Rectangle<float> (cell.x, y + 104 * s, cell.w, 24 * s), juce::Justification::centredLeft, false);
                lastLyric = e.lyric;
            }
            else if (lastLyric.isNotEmpty()) // melisma line
            {
                g.setColour (st.faint);
                g.drawLine (cell.x + 2 * s, y + 124 * s, cell.x + cell.w - 4 * s, y + 124 * s, 1.0f * s);
            }
        }
        else lastLyric.clear();

        if (st.showParallage && ! e.pause)
        {
            g.setColour (st.faint);
            g.setFont (pf);
            g.drawText (utf8 (degreeGreek (d.deg[(size_t) i])), juce::Rectangle<float> (cell.x, y + 132 * s, cell.w, 18 * s), juce::Justification::centredLeft, false);
        }
    }
}
} // namespace byz
