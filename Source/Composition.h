// Psaltikon — composition model, timing, timeline building, ison analysis and melody generation.
#pragma once
#include <JuceHeader.h>
#include "ByzTheory.h"
#include <climits>
#include <memory>

namespace byz
{
static constexpr int kNoIson = INT_MIN;

enum TimeSign { TimeNone = 0, Gorgon, Digorgon, Argon };
enum Expression { ExprNone = 0, Vareia, Psifiston, Omalon, Antikenoma, Eteron };
enum AscentGlyph { AscOligon = 0, AscPetasti, AscKentimata };

struct Event
{
    bool pause = false;
    int delta = 0;          // quantity: steps from the previous note (-7..+7)
    int ascentGlyph = 0;    // oligon / petasti / kentimata for +1
    int time = TimeNone;
    int ext = 0;            // 1 apli, 2 dipli, 3 tripli (beats added)
    bool klasma = false;    // +1 beat
    int expr = ExprNone;
    int fthora = -1;        // genus change from this note on
    int accidental = 0;     // -1 ὕφεσις, +1 δίεσις (±4 moria on this note)
    int ison = kNoIson;     // ison indication: ison moves to this degree here
    bool martyria = false;  // print a martyria after this note
    juce::String lyric;     // syllable; empty or "_" continues the previous syllable
};

struct Composition
{
    juce::String title { juce::CharPointer_UTF8 (u8"Κύριε ἐλέησον") };
    int mode = 0, variant = 0;
    double bpm = 90.0;
    int startDeg = 1;
    std::vector<Event> events;

    const Variant& var() const { return variantOf (mode, variant); }
};

// ---- per-event derived values ----------------------------------------------
struct Derived
{
    std::vector<int> deg;       // degree of each event (pauses keep the last degree)
    std::vector<int> genus;     // genus in force
    std::vector<int> moria;     // final pitch in moria above Ni (attractions + accidentals)
    std::vector<double> beats;  // duration in beats
    std::vector<double> start;  // start beat
    double total = 0;
};

inline Derived derive (const Composition& c, bool attractions)
{
    Derived d;
    const int n = (int) c.events.size();
    d.deg.resize ((size_t) n); d.genus.resize ((size_t) n); d.moria.resize ((size_t) n);
    d.beats.assign ((size_t) n, 1.0); d.start.resize ((size_t) n);
    int cur = c.startDeg, g = c.var().genus;
    for (int i = 0; i < n; ++i)
    {
        auto& e = c.events[(size_t) i];
        if (! e.pause) cur += e.delta;
        if (e.fthora >= 0) g = e.fthora;
        d.deg[(size_t) i] = cur;
        d.genus[(size_t) i] = g;
    }
    // attraction needs neighbours (notes only)
    std::vector<int> noteIdx;
    for (int i = 0; i < n; ++i) if (! c.events[(size_t) i].pause) noteIdx.push_back (i);
    for (size_t k = 0; k < noteIdx.size(); ++k)
    {
        int i = noteIdx[k];
        auto& e = c.events[(size_t) i];
        int prev = k > 0 ? d.deg[(size_t) noteIdx[k - 1]] : d.deg[(size_t) i];
        bool hasNext = k + 1 < noteIdx.size();
        int next = hasNext ? d.deg[(size_t) noteIdx[k + 1]] : d.deg[(size_t) i];
        int alt = e.accidental * 4;
        if (attractions && e.accidental == 0) alt += attraction (prev, d.deg[(size_t) i], next, hasNext, d.genus[(size_t) i]);
        d.moria[(size_t) i] = moriaOf (d.deg[(size_t) i], d.genus[(size_t) i], alt);
    }
    for (int i = 0; i < n; ++i) if (c.events[(size_t) i].pause) d.moria[(size_t) i] = moriaOf (d.deg[(size_t) i], d.genus[(size_t) i]);

    // rhythm: gorgon, digorgon and argon divide beats; apli/dipli/tripli and klasma add them
    for (int i = 0; i < n; ++i)
    {
        auto& e = c.events[(size_t) i];
        if (e.pause) continue;
        if (e.time == Gorgon && i > 0) { d.beats[(size_t) i - 1] = 0.5; d.beats[(size_t) i] = 0.5; }
        else if (e.time == Digorgon && i > 0)
        {
            d.beats[(size_t) i - 1] = 1.0 / 3; d.beats[(size_t) i] = 1.0 / 3;
            if (i + 1 < n) d.beats[(size_t) i + 1] = 1.0 / 3;
        }
        else if (e.time == Argon && i > 0) { d.beats[(size_t) i - 1] = 0.5; d.beats[(size_t) i] = 1.5; }
    }
    for (int i = 0; i < n; ++i)
    {
        auto& e = c.events[(size_t) i];
        d.beats[(size_t) i] += e.ext + (e.klasma ? 1 : 0);
    }
    double t = 0;
    for (int i = 0; i < n; ++i) { d.start[(size_t) i] = t; t += d.beats[(size_t) i]; }
    d.total = t;
    return d;
}

// ---- lyric phonetics -------------------------------------------------------
// Reduce Greek (monotonic or polytonic) and Latin letters to a base lowercase letter.
inline juce::juce_wchar baseLetter (juce::juce_wchar c)
{
    // explicit Greek case folding: towlower() depends on the C locale and may leave Greek capitals alone
    if (c >= 0x0391 && c <= 0x03A9 && c != 0x03A2) c += 0x20;
    else if (c == 0x0386) c = 0x03AC; else if (c == 0x0388) c = 0x03AD; else if (c == 0x0389) c = 0x03AE;
    else if (c == 0x038A) c = 0x03AF; else if (c == 0x038C) c = 0x03CC; else if (c == 0x038E) c = 0x03CD; else if (c == 0x038F) c = 0x03CE;
    else if (c < 128) c = juce::CharacterFunctions::toLowerCase (c);
    if (c >= 0x1F00 && c <= 0x1FFF)
    {
        struct R { int lo, hi; juce::juce_wchar b; };
        static const R r[] = {
            { 0x1F00, 0x1F0F, 0x03B1 }, { 0x1F10, 0x1F1F, 0x03B5 }, { 0x1F20, 0x1F2F, 0x03B7 }, { 0x1F30, 0x1F3F, 0x03B9 },
            { 0x1F40, 0x1F4F, 0x03BF }, { 0x1F50, 0x1F5F, 0x03C5 }, { 0x1F60, 0x1F6F, 0x03C9 }, { 0x1F70, 0x1F71, 0x03B1 },
            { 0x1F72, 0x1F73, 0x03B5 }, { 0x1F74, 0x1F75, 0x03B7 }, { 0x1F76, 0x1F77, 0x03B9 }, { 0x1F78, 0x1F79, 0x03BF },
            { 0x1F7A, 0x1F7B, 0x03C5 }, { 0x1F7C, 0x1F7D, 0x03C9 }, { 0x1F80, 0x1F8F, 0x03B1 }, { 0x1F90, 0x1F9F, 0x03B7 },
            { 0x1FA0, 0x1FAF, 0x03C9 }, { 0x1FB0, 0x1FBC, 0x03B1 }, { 0x1FC2, 0x1FCC, 0x03B7 }, { 0x1FD0, 0x1FDB, 0x03B9 },
            { 0x1FE4, 0x1FE5, 0x03C1 }, { 0x1FE0, 0x1FEB, 0x03C5 }, { 0x1FF2, 0x1FFC, 0x03C9 } };
        for (auto& x : r) if ((int) c >= x.lo && (int) c <= x.hi) return x.b;
        return c;
    }
    switch ((int) c)
    {
        case 0x03AC: return 0x03B1; case 0x03AD: return 0x03B5; case 0x03AE: return 0x03B7;
        case 0x03AF: case 0x03CA: case 0x0390: return 0x03B9;
        case 0x03CC: return 0x03BF; case 0x03CD: case 0x03CB: case 0x03B0: return 0x03C5;
        case 0x03CE: return 0x03C9; case 0x03C2: return 0x03C3;
        default: return c;
    }
}

inline bool isVowelLetter (juce::juce_wchar b)
{
    return juce::String (juce::CharPointer_UTF8 (u8"αεηιουωaeiouy")).containsChar (b);
}

// vowel: 0 a, 1 e, 2 i, 3 o, 4 u
inline int vowelIndex (char v) { switch (v) { case 'a': return 0; case 'e': return 1; case 'i': return 2; case 'o': return 3; case 'u': return 4; default: return 0; } }

inline int lyricVowel (const juce::String& syl, int fallback)
{
    juce::Array<juce::juce_wchar> b;
    for (auto p = syl.getCharPointer(); ! p.isEmpty(); ++p) b.add (baseLetter (*p));
    for (int i = 0; i < b.size(); ++i)
    {
        auto c = b[i];
        if (! isVowelLetter (c)) continue;
        auto n = i + 1 < b.size() ? b[i + 1] : 0;
        if ((c == 0x03BF && n == 0x03C5) || (c == 'o' && n == 'u')) return 4;
        if ((c == 0x03B1 && n == 0x03B9) || (c == 'a' && n == 'e')) return 1;
        if ((c == 0x03B5 || c == 0x03BF || c == 0x03C5) && n == 0x03B9) return 2;
        switch ((int) c)
        {
            case 0x03B1: case 'a': return 0;
            case 0x03B5: case 'e': return 1;
            case 0x03B7: case 0x03B9: case 0x03C5: case 'i': case 'y': return 2;
            case 0x03BF: case 0x03C9: case 'o': return 3;
            case 'u': return 4;
            default: break;
        }
    }
    return fallback;
}

// 0 none, 1 plosive, 2 fricative, 3 nasal/liquid
inline int lyricConsonant (const juce::String& syl)
{
    if (syl.isEmpty()) return 0;
    auto c = baseLetter (syl[0]);
    if (isVowelLetter (c)) return 0;
    const juce::String plos (juce::CharPointer_UTF8 (u8"πτκpbtdkgcq"));
    const juce::String fric (juce::CharPointer_UTF8 (u8"βγδφθχσζξψfvszhxj"));
    if (plos.containsChar (c)) return 1;
    if (fric.containsChar (c)) return 2;
    return 3;
}

inline bool isContinuation (const juce::String& s) { auto t = s.trim(); return t.isEmpty() || t == "_"; }

// ---- timeline: what the audio engine plays ---------------------------------
enum CtrlKind { CPitch, CVowel, CArtic, CAccent, CLevel, CVibrato, CBreath };

struct Ctrl
{
    double beat;
    int kind;
    float a = 0, b = 0;
    int event = -1;
};

struct Timeline
{
    std::vector<Ctrl> ctrls;             // melody controls, sorted by beat
    std::vector<std::pair<double, int>> ison; // (beat, moria)
    std::vector<double> eventStart;      // per score event (for the cursor)
    double total = 0;
    double bpm = 90;
    bool hasMelody = true;
};

struct TimelineOptions
{
    bool attractions = true;
    bool singParallage = false;
    float articulation = 0.5f;   // 0 legato .. 1 detached
};

// Builds the performance from the score: pitch glides, vowels, consonant onsets,
// ornaments from the expression signs, phrase shaping and breaths at the pauses.
inline std::shared_ptr<Timeline> buildTimeline (const Composition& c, const TimelineOptions& o)
{
    auto tl = std::make_shared<Timeline>();
    tl->bpm = c.bpm;
    const double spb = 60.0 / c.bpm;
    auto d = derive (c, o.attractions);
    const int n = (int) c.events.size();
    tl->eventStart = d.start;
    tl->total = d.total;
    const float baseGlide = 0.035f + 0.05f * (1.0f - o.articulation);
    int vowel = 0;
    bool sounding = false;
    for (int i = 0; i < n; ++i)
    {
        auto& e = c.events[(size_t) i];
        const double b0 = d.start[(size_t) i], db = d.beats[(size_t) i], secs = db * spb;
        if (e.pause)
        {
            if (sounding) tl->ctrls.push_back ({ b0, CLevel, 0.0f, 0.12f, i });
            sounding = false;
            if (secs > 0.6) tl->ctrls.push_back ({ b0 + db - 0.45 / spb, CBreath, 1.0f, 0, i });
            continue;
        }
        const int m = d.moria[(size_t) i];
        bool newSyl; int cons;
        if (o.singParallage) { newSyl = true; vowel = vowelIndex (parallageVowel (d.deg[(size_t) i])); cons = parallageConsonant (d.deg[(size_t) i]); }
        else
        {
            newSyl = ! isContinuation (e.lyric);
            if (newSyl) { vowel = lyricVowel (e.lyric, vowel); cons = lyricConsonant (e.lyric); }
            else cons = 0;
        }
        const bool prevEteron = i > 0 && ! c.events[(size_t) i - 1].pause && c.events[(size_t) i - 1].expr == Eteron;
        float glide = prevEteron ? 0.22f : (newSyl ? baseGlide * 0.8f : baseGlide);
        if (! sounding) glide = 0.0f;

        if (e.expr == Antikenoma)
        {
            tl->ctrls.push_back ({ b0, CPitch, (float) (m - 10), glide * 0.5f, i });
            tl->ctrls.push_back ({ b0 + std::min (0.12 / spb, db * 0.3), CPitch, (float) m, 0.03f, i });
        }
        else tl->ctrls.push_back ({ b0, CPitch, (float) m, glide, i });

        if (newSyl || ! sounding) tl->ctrls.push_back ({ b0, CVowel, (float) vowel, 0, i });
        if (newSyl && cons > 0) tl->ctrls.push_back ({ b0, CArtic, (float) cons, o.articulation, i });
        if (! sounding) tl->ctrls.push_back ({ b0, CLevel, 1.0f, 0.06f, i });
        else if (newSyl) tl->ctrls.push_back ({ b0, CLevel, 1.0f, 0.08f, i });
        sounding = true;

        if (e.expr == Psifiston || e.expr == Vareia) tl->ctrls.push_back ({ b0, CAccent, 1.35f, 0.12f, i });
        if (e.delta > 0 && e.ascentGlyph == AscPetasti) tl->ctrls.push_back ({ b0, CAccent, 1.18f, 0.25f, i });
        if (e.expr == Omalon)
        {
            const double up = b0 + db * 0.35, back = up + std::min (0.14 / spb, db * 0.2);
            const int um = moriaOf (d.deg[(size_t) i] + 1, d.genus[(size_t) i]);
            tl->ctrls.push_back ({ up, CPitch, (float) um, 0.025f, i });
            tl->ctrls.push_back ({ back, CPitch, (float) m, 0.035f, i });
        }
        tl->ctrls.push_back ({ b0, CVibrato, secs > 0.55 ? 1.0f : 0.25f, (float) secs, i });

        // phrase shaping: soften toward a pause or the end
        int nx = i + 1;
        const bool phraseEnd = nx >= n || c.events[(size_t) nx].pause;
        if (phraseEnd)
        {
            tl->ctrls.push_back ({ b0 + db * 0.45, CLevel, 0.72f, (float) (secs * 0.5), i });
            if (nx >= n) { tl->ctrls.push_back ({ b0 + db, CLevel, 0.0f, 0.4f, i }); sounding = false; }
        }
    }
    std::stable_sort (tl->ctrls.begin(), tl->ctrls.end(), [] (const Ctrl& a, const Ctrl& b) { return a.beat < b.beat; });

    // ison line from the ison indications; base note if there are none
    const auto& v = c.var();
    int g = v.genus;
    bool any = false;
    for (int i = 0; i < n; ++i)
    {
        auto& e = c.events[(size_t) i];
        if (e.fthora >= 0) g = e.fthora;
        if (e.ison != kNoIson)
        {
            if (! any && i > 0) tl->ison.push_back ({ 0.0, moriaOf (v.base, v.genus) });
            tl->ison.push_back ({ d.start[(size_t) i], moriaOf (e.ison, g) });
            any = true;
        }
    }
    if (! any) tl->ison.push_back ({ 0.0, moriaOf (v.base, v.genus) });
    return tl;
}

// A quick sequence of degrees for auditions (scales, formulas, exercises, quiz).
inline std::shared_ptr<Timeline> sequenceTimeline (const Variant& v, const std::vector<int>& degs, double bpm,
                                                   bool parallage, const std::vector<juce::String>& syllables = {},
                                                   int genusOverride = -1, int isonDeg = kNoIson, double lastBeats = 2.0)
{
    Composition c;
    c.bpm = bpm;
    // locate the variant to keep genus/base
    for (int mi = 0; mi < (int) modes().size(); ++mi)
        for (int vi = 0; vi < (int) modes()[(size_t) mi].variants.size(); ++vi)
            if (&modes()[(size_t) mi].variants[(size_t) vi] == &v) { c.mode = mi; c.variant = vi; }
    if (degs.empty()) return std::make_shared<Timeline>();
    c.startDeg = degs[0];
    for (size_t k = 0; k < degs.size(); ++k)
    {
        Event e;
        e.delta = k == 0 ? 0 : std::clamp (degs[k] - degs[k - 1], -14, 14);
        if (k == 0 && genusOverride >= 0) e.fthora = genusOverride;
        if (k < syllables.size()) e.lyric = syllables[k];
        if (k + 1 == degs.size()) e.ext = (int) std::max (0.0, lastBeats - 1.0);
        if (k == 0 && isonDeg != kNoIson) e.ison = isonDeg;
        c.events.push_back (e);
    }
    TimelineOptions o; o.attractions = false; o.singParallage = parallage;
    return buildTimeline (c, o);
}

// ---- automatic ison -------------------------------------------------------
// The ison follows the region the melody occupies: it sits on the highest permitted
// ison note at or below the melody's floor over the next few notes, changes only when
// the new region lasts, and returns to the base for the final cadence.
inline void autoIson (Composition& c)
{
    const auto& v = c.var();
    auto d = derive (c, false);
    std::vector<int> notes;
    for (int i = 0; i < (int) c.events.size(); ++i) { c.events[(size_t) i].ison = kNoIson; if (! c.events[(size_t) i].pause) notes.push_back (i); }
    if (notes.empty()) return;
    std::vector<int> allowed = v.isonNotes;
    std::sort (allowed.begin(), allowed.end());
    std::vector<int> cand (notes.size());
    for (size_t k = 0; k < notes.size(); ++k)
    {
        int lo = INT_MAX;
        for (size_t j = k; j < std::min (notes.size(), k + 4); ++j) lo = std::min (lo, d.deg[(size_t) notes[j]]);
        int best = v.base;
        bool found = false;
        for (int a : allowed) if (a <= lo) { best = a; found = true; }
        if (! found) best = allowed.front();
        // prefer the base when it is close to the chosen note
        if (best != v.base && v.base <= lo && lo - v.base <= 2) best = v.base;
        cand[k] = best;
    }
    for (size_t k = notes.size() >= 3 ? notes.size() - 3 : 0; k < notes.size(); ++k) cand[k] = v.base;
    // smooth: a change must last at least 3 notes
    int cur = cand[0];
    c.events[(size_t) notes[0]].ison = cur;
    for (size_t k = 1; k < notes.size(); ++k)
    {
        if (cand[k] == cur) continue;
        size_t run = 0;
        while (k + run < notes.size() && cand[k + run] == cand[k]) ++run;
        if (run >= 3 || k + run == notes.size())
        {
            cur = cand[k];
            c.events[(size_t) notes[k]].ison = cur;
        }
    }
}

// ---- editing helpers ------------------------------------------------------
inline int degreeBefore (const Composition& c, int index)
{
    int cur = c.startDeg;
    for (int i = 0; i < index && i < (int) c.events.size(); ++i) if (! c.events[(size_t) i].pause) cur += c.events[(size_t) i].delta;
    return cur;
}

// Insert absolute degrees at a position, converting them to neume quantities.
inline void insertDegrees (Composition& c, int pos, const std::vector<int>& degs, const std::vector<juce::String>& lyrics = {})
{
    pos = std::clamp (pos, 0, (int) c.events.size());
    if (degs.empty()) return;
    int afterDeg = pos < (int) c.events.size() ? degreeBefore (c, pos + 1) : INT_MIN;
    bool fixFollowing = pos < (int) c.events.size() && ! c.events[(size_t) pos].pause;
    int prev = degreeBefore (c, pos);
    if (c.events.empty()) { c.startDeg = degs[0]; prev = degs[0]; }
    std::vector<Event> add;
    for (size_t k = 0; k < degs.size(); ++k)
    {
        Event e;
        e.delta = std::clamp (degs[k] - prev, -7, 7);
        prev += e.delta;
        if (k < lyrics.size()) e.lyric = lyrics[k];
        add.push_back (e);
    }
    c.events.insert (c.events.begin() + pos, add.begin(), add.end());
    if (fixFollowing) // keep the following note at its pitch
    {
        auto& nx = c.events[(size_t) pos + add.size()];
        nx.delta = std::clamp (afterDeg - prev, -7, 7);
    }
}

// Shift a range by k steps without moving the notes after it.
inline void shiftRange (Composition& c, int from, int to, int k)
{
    int first = -1, after = -1;
    for (int i = from; i <= to && i < (int) c.events.size(); ++i) if (! c.events[(size_t) i].pause) { first = i; break; }
    for (int i = to + 1; i < (int) c.events.size(); ++i) if (! c.events[(size_t) i].pause) { after = i; break; }
    if (first < 0) return;
    if (first == 0 && from == 0) c.startDeg += k; else c.events[(size_t) first].delta = std::clamp (c.events[(size_t) first].delta + k, -7, 7);
    if (after >= 0) c.events[(size_t) after].delta = std::clamp (c.events[(size_t) after].delta - k, -7, 7);
}

inline std::vector<int> rangeDegrees (const Composition& c, int from, int to)
{
    auto d = derive (c, false);
    std::vector<int> out;
    for (int i = from; i <= to && i < (int) c.events.size(); ++i) if (! c.events[(size_t) i].pause) out.push_back (d.deg[(size_t) i]);
    return out;
}

// Replace the notes of a range with new degrees, keeping lyrics where possible.
inline void replaceRange (Composition& c, int from, int to, const std::vector<int>& degs, bool keepLyrics = true)
{
    std::vector<juce::String> ly;
    if (keepLyrics) for (int i = from; i <= to && i < (int) c.events.size(); ++i) if (! c.events[(size_t) i].pause) ly.push_back (c.events[(size_t) i].lyric);
    std::vector<juce::String> l2 (degs.size());
    // spread existing syllables over the new notes, first note of each group carries it
    if (! ly.empty())
    {
        for (size_t k = 0; k < ly.size(); ++k)
        {
            size_t at = (size_t) std::llround ((double) k * (double) degs.size() / (double) ly.size());
            if (at < l2.size() && ! isContinuation (ly[k])) l2[at] = ly[k];
        }
    }
    int afterIdx = to + 1;
    int afterDeg = afterIdx < (int) c.events.size() ? degreeBefore (c, afterIdx + 1) : INT_MIN;
    c.events.erase (c.events.begin() + from, c.events.begin() + std::min ((int) c.events.size(), to + 1));
    int prev = degreeBefore (c, from);
    if (from == 0 && ! degs.empty()) { c.startDeg = degs[0]; prev = degs[0]; }
    std::vector<Event> add;
    for (size_t k = 0; k < degs.size(); ++k)
    {
        Event e; e.delta = std::clamp (degs[k] - prev, -7, 7); prev += e.delta; e.lyric = l2[k]; add.push_back (e);
    }
    c.events.insert (c.events.begin() + from, add.begin(), add.end());
    int ai = from + (int) add.size();
    if (afterDeg != INT_MIN && ai < (int) c.events.size() && ! c.events[(size_t) ai].pause)
        c.events[(size_t) ai].delta = std::clamp (afterDeg - prev, -7, 7);
}

// ---- melody generation from text -----------------------------------------
enum Style { Syllabic = 0, Neumatic, Melismatic };

inline std::vector<int> connect (int from, int to)
{
    std::vector<int> p;
    int d = from;
    while (std::abs (to - d) > 2) { d += to > d ? 1 : -1; p.push_back (d); }
    return p;
}

inline Composition composeFromText (const juce::String& text, int style, int mode, int variant, double bpm, unsigned seed)
{
    Composition c;
    c.mode = mode; c.variant = variant; c.bpm = bpm;
    const auto& v = c.var();
    std::mt19937 rng (seed);
    auto rnd = [&] (int lo, int hi) { return std::uniform_int_distribution<int> (lo, hi) (rng); };
    auto pick = [&] (const char* fn) -> const Formula*
    {
        std::vector<const Formula*> f;
        for (auto& x : v.formulas) if (juce::String (x.function) == fn) f.push_back (&x);
        return f.empty() ? nullptr : f[(size_t) rnd (0, (int) f.size() - 1)];
    };

    // phrases split at punctuation; syllables split at spaces and hyphens
    juce::StringArray phrases;
    {
        juce::String cur;
        for (auto p = text.getCharPointer(); ! p.isEmpty(); ++p)
        {
            auto ch = *p;
            if (ch == '\n') ch = ' ';
            cur += juce::String::charToString (ch);
            if (ch == ',' || ch == '.' || ch == ';' || ch == ':' || ch == 0x0387 || ch == '!' || ch == '?')
            { if (cur.trim().isNotEmpty()) phrases.add (cur.trim()); cur.clear(); }
        }
        if (cur.trim().isNotEmpty()) phrases.add (cur.trim());
    }
    if (phrases.isEmpty()) phrases.add (juce::String (juce::CharPointer_UTF8 (u8"Κύ-ρι-ε ἐ-λέ-η-σον")));

    int last = v.base;
    c.startDeg = v.base;
    std::vector<int> allDegs;
    std::vector<Event> evs;
    for (int pi = 0; pi < phrases.size(); ++pi)
    {
        juce::StringArray words; words.addTokens (phrases[pi], " ", ""); words.removeEmptyStrings();
        std::vector<juce::String> syl;
        std::vector<bool> wordEnd;
        for (auto& w : words)
        {
            juce::StringArray s; s.addTokens (w, "-", ""); s.removeEmptyStrings();
            for (int k = 0; k < s.size(); ++k) { syl.push_back (s[k]); wordEnd.push_back (k == s.size() - 1); }
        }
        if (syl.empty()) continue;
        std::vector<int> per (syl.size());
        for (size_t k = 0; k < syl.size(); ++k)
        {
            if (style == Syllabic) per[k] = 1;
            else if (style == Neumatic) { int r = rnd (0, 9); per[k] = r < 4 ? 1 : (r < 8 ? 2 : 3); }
            else { int r = rnd (0, 9); per[k] = wordEnd[k] ? rnd (3, 7) : (r < 3 ? 1 : rnd (2, 4)); }
        }
        const bool finalPhrase = pi == phrases.size() - 1;
        if (style != Syllabic) per.back() = std::max (per.back(), finalPhrase ? 3 : 2);
        int N = 0; for (int x : per) N += x;

        auto* cad = pick (finalPhrase ? "Final cadence" : "Medial cadence");
        std::vector<int> cadence = cad ? cad->degs : std::vector<int> { v.base };
        if ((int) cadence.size() > N) cadence.erase (cadence.begin(), cadence.end() - N);
        std::vector<int> body;
        const Formula* first = pi == 0 ? pick ("Opening") : nullptr;
        if (first) body = first->degs;
        while ((int) body.size() < N - (int) cadence.size())
        {
            const char* fns[] = { "Ascent", "Descent", "Melismatic", "Opening" };
            auto* f = pick (fns[rnd (0, style == Melismatic ? 3 : 2)]);
            if (! f) break;
            int from = body.empty() ? last : body.back();
            auto bridge = connect (from, f->degs.front());
            body.insert (body.end(), bridge.begin(), bridge.end());
            body.insert (body.end(), f->degs.begin(), f->degs.end());
        }
        body.resize ((size_t) std::max (0, N - (int) cadence.size()));
        // smooth the join into the cadence
        {
            int target = cadence.front();
            for (int k = (int) body.size() - 1; k >= 0; --k)
            {
                if (std::abs (body[(size_t) k] - target) <= 2) break;
                body[(size_t) k] = target + (body[(size_t) k] > target ? 2 : -2);
                target = body[(size_t) k];
            }
        }
        std::vector<int> line = body;
        line.insert (line.end(), cadence.begin(), cadence.end());

        size_t at = 0;
        for (size_t k = 0; k < syl.size(); ++k)
        {
            for (int j = 0; j < per[k] && at < line.size(); ++j, ++at)
            {
                Event e;
                e.lyric = j == 0 ? syl[k] : juce::String();
                if (style == Melismatic && j > 0 && j % 2 == 1 && j + 1 < per[k]) e.time = Gorgon;
                allDegs.push_back (line[at]);
                evs.push_back (e);
            }
        }
        if (! evs.empty()) evs.back().ext = finalPhrase ? 2 : 1;
        last = line.back();
        if (! finalPhrase) { Event p; p.pause = true; evs.push_back (p); allDegs.push_back (last); }
    }
    // deltas
    if (! allDegs.empty()) c.startDeg = allDegs[0];
    int prev = c.startDeg;
    for (size_t k = 0; k < evs.size(); ++k)
    {
        if (! evs[k].pause) { evs[k].delta = std::clamp (allDegs[k] - prev, -7, 7); prev += evs[k].delta; }
    }
    c.events = evs;
    if (! c.events.empty()) c.events.back().martyria = true;
    autoIson (c);
    return c;
}

// ---- composing with parallage syllables ------------------------------------
// "Πα Βου Γα Δι" or "Pa Vou Ga Di": each syllable goes to the nearest octave of the
// previous note; ' (or ′) forces the upper octave, ',' the lower one (of the base octave).
// Returns false and the 0-based token index of the first unknown syllable.
inline bool parseParallage (const juce::String& text, int prevDeg, std::vector<int>& degs, int& badToken)
{
    degs.clear(); badToken = -1;
    auto toks = juce::StringArray::fromTokens (text.replace ("-", " ").replace ("\n", " "), " \t\n", "");
    toks.removeEmptyStrings();
    int prev = prevDeg;
    for (int t = 0; t < toks.size(); ++t)
    {
        juce::String raw = toks[t];
        int forced = 0;
        while (raw.endsWith ("'") || raw.endsWith (juce::String (juce::CharPointer_UTF8 (u8"′")))) { ++forced; raw = raw.dropLastCharacters (1); }
        while (raw.endsWith (",")) { --forced; raw = raw.dropLastCharacters (1); }
        juce::String base;
        for (auto p = raw.getCharPointer(); ! p.isEmpty(); ++p) base += juce::String::charToString (baseLetter (*p));
        static const char* lat[] = { "ni", "pa", "vou", "ga", "di", "ke", "zo" };
        static const char* gr[] = { u8"νη", u8"πα", u8"βου", u8"γα", u8"δι", u8"κε", u8"ζω" };
        int idx = -1;
        for (int k = 0; k < 7; ++k)
            if (base == lat[k] || base == juce::String (juce::CharPointer_UTF8 (gr[k])) || (k == 2 && (base == "vu" || base == "bou")))
                idx = k;
        if (idx < 0) { badToken = t; return false; }
        int d;
        if (forced != 0) d = idx + 7 * forced;
        else
        {
            d = idx + 7 * (int) std::floor ((prev - idx) / 7.0);
            if (std::abs (d + 7 - prev) < std::abs (d - prev)) d += 7;
        }
        if (forced == 0 && ! degs.empty() && std::abs (d - degs.back()) > 7) d += d > degs.back() ? -7 : 7;
        degs.push_back (d);
        prev = d;
    }
    return ! degs.empty();
}

// ---- persistence ----------------------------------------------------------
inline juce::var toVar (const Composition& c)
{
    auto* o = new juce::DynamicObject();
    o->setProperty ("format", "psaltikon-1");
    o->setProperty ("title", c.title);
    o->setProperty ("mode", c.mode);
    o->setProperty ("variant", c.variant);
    o->setProperty ("bpm", c.bpm);
    o->setProperty ("start", c.startDeg);
    juce::Array<juce::var> ev;
    for (auto& e : c.events)
    {
        auto* x = new juce::DynamicObject();
        if (e.pause) x->setProperty ("p", true);
        x->setProperty ("d", e.delta);
        if (e.ascentGlyph) x->setProperty ("g", e.ascentGlyph);
        if (e.time) x->setProperty ("t", e.time);
        if (e.ext) x->setProperty ("x", e.ext);
        if (e.klasma) x->setProperty ("k", true);
        if (e.expr) x->setProperty ("e", e.expr);
        if (e.fthora >= 0) x->setProperty ("f", e.fthora);
        if (e.accidental) x->setProperty ("a", e.accidental);
        if (e.ison != kNoIson) x->setProperty ("i", e.ison);
        if (e.martyria) x->setProperty ("m", true);
        if (e.lyric.isNotEmpty()) x->setProperty ("l", e.lyric);
        ev.add (juce::var (x));
    }
    o->setProperty ("events", ev);
    return juce::var (o);
}

inline bool fromVar (const juce::var& v, Composition& c)
{
    if (! v.isObject()) return false;
    Composition r;
    r.title = v.getProperty ("title", r.title).toString();
    r.mode = std::clamp ((int) v.getProperty ("mode", 0), 0, 7);
    r.variant = std::clamp ((int) v.getProperty ("variant", 0), 0, (int) modes()[(size_t) r.mode].variants.size() - 1);
    r.bpm = std::clamp ((double) v.getProperty ("bpm", 90.0), 20.0, 240.0);
    r.startDeg = std::clamp ((int) v.getProperty ("start", 1), -14, 21);
    if (auto* a = v.getProperty ("events", {}).getArray())
        for (auto& x : *a)
        {
            Event e;
            e.pause = (bool) x.getProperty ("p", false);
            e.delta = std::clamp ((int) x.getProperty ("d", 0), -7, 7);
            e.ascentGlyph = std::clamp ((int) x.getProperty ("g", 0), 0, 2);
            e.time = std::clamp ((int) x.getProperty ("t", 0), 0, 3);
            e.ext = std::clamp ((int) x.getProperty ("x", 0), 0, 3);
            e.klasma = (bool) x.getProperty ("k", false);
            e.expr = std::clamp ((int) x.getProperty ("e", 0), 0, 5);
            e.fthora = std::clamp ((int) x.getProperty ("f", -1), -1, 3);
            e.accidental = std::clamp ((int) x.getProperty ("a", 0), -1, 1);
            e.ison = x.hasProperty ("i") ? (int) x.getProperty ("i", 0) : kNoIson;
            e.martyria = (bool) x.getProperty ("m", false);
            e.lyric = x.getProperty ("l", "").toString();
            r.events.push_back (e);
        }
    c = r;
    return true;
}

// The sample composition (melody composed for this demo; liturgical text).
inline Composition sampleComposition()
{
    Composition c;
    c.mode = 0; c.variant = 0; c.bpm = 88; c.startDeg = 1;
    const char* s1[] = { u8"Κύ", "", u8"ρι", "", u8"ε", u8"ἐ", u8"λέ", "", u8"η", u8"σον" };
    std::vector<int> d1 { 1, 2, 3, 3, 2, 1, 2, 3, 4, 4 };
    const char* s2[] = { u8"Κύ", u8"ρι", "", u8"ε", u8"ἐ", u8"λέ", u8"η", u8"σον", "" };
    std::vector<int> d2 { 4, 3, 4, 3, 2, 3, 2, 1, 1 };
    std::vector<juce::String> l1, l2;
    for (auto* s : s1) l1.push_back (juce::String (juce::CharPointer_UTF8 (s)));
    for (auto* s : s2) l2.push_back (juce::String (juce::CharPointer_UTF8 (s)));
    insertDegrees (c, 0, d1, l1);
    c.events[3].time = Gorgon;
    c.events.back().ext = 1;
    Event p; p.pause = true; c.events.push_back (p);
    insertDegrees (c, (int) c.events.size(), d2, l2);
    c.events[c.events.size() - 3].expr = Omalon;
    c.events[c.events.size() - 2].time = Gorgon;
    c.events.back().ext = 2;
    c.events.back().martyria = true;
    autoIson (c);
    return c;
}
} // namespace byz
