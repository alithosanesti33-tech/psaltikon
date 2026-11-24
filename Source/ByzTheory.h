// Psaltikon — Byzantine music theory core.
// Interval system: the 72-moria octave of the 1881 Patriarchal Music Committee
// (diatonic 12-10-8 | 12-12-10-8, soft chromatic 8-14-8, hard chromatic 6-20-4,
// enharmonic 12-12-6). Other schools (Chrysanthos' 68 moria, Karas) differ.
// This header has no JUCE dependency so the theory can be tested on its own.
#pragma once
#include <array>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <random>

namespace byz
{
enum Genus { Diatonic = 0, SoftChromatic, HardChromatic, Enharmonic, NumGenera };

inline const char* genusName (int g)
{
    static const char* n[] = { "Diatonic", "Soft chromatic", "Hard chromatic", "Enharmonic" };
    return n[std::clamp (g, 0, 3)];
}
inline const char* genusGreek (int g)
{
    static const char* n[] = { u8"Διατονικόν", u8"Μαλακόν χρωματικόν", u8"Σκληρόν χρωματικόν", u8"Ἐναρμόνιον" };
    return n[std::clamp (g, 0, 3)];
}

// Position of each degree (Ni..Zo) in moria above Ni. Each table is anchored so the
// mode that uses it keeps its base note at the diatonic pitch (Pa for hard chromatic,
// Ga for enharmonic, Di for soft chromatic).
inline const std::array<int, 7>& genusTable (int g)
{
    static const std::array<std::array<int, 7>, 4> t {{
        {{ 0, 12, 22, 30, 42, 54, 64 }},   // diatonic      12 10  8 12 12 10  8
        {{ 0,  8, 22, 30, 42, 50, 64 }},   // soft chrom.    8 14  8 12  8 14  8
        {{ 8, 12, 18, 38, 42, 54, 60 }},   // hard chrom.    4  6 20  4 12  6 20
        {{ 0, 12, 18, 30, 42, 54, 60 }},   // enharmonic    12  6 12 12 12  6 12
    }};
    return t[(size_t) std::clamp (g, 0, 3)];
}

inline int mod7 (int d) { return ((d % 7) + 7) % 7; }
inline int oct7 (int d) { return (int) std::floor (d / 7.0); }
inline int moriaOf (int deg, int genus, int alt = 0) { return genusTable (genus)[(size_t) mod7 (deg)] + 72 * oct7 (deg) + alt; }
inline double hzOf (double moria, double niHz) { return niHz * std::pow (2.0, moria / 72.0); }
inline double centsOfMoria (double m) { return m * 1200.0 / 72.0; }

// Note names. The parallage syllables (Νη Πα Βου…) are what is sung; the theoretical
// names are the letters of the Chrysanthine system (α–η, Pa = α … Ni = η) from which the
// syllables were formed, and the martyria letters are what the score writes.
inline const char* noteGreek (int deg)   { static const char* n[] = { u8"Νη", u8"Πα", u8"Βου", u8"Γα", u8"Δι", u8"Κε", u8"Ζω" }; return n[mod7 (deg)]; }
inline const char* noteLatin (int deg)   { static const char* n[] = { "Ni", "Pa", "Vou", "Ga", "Di", "Ke", "Zo" }; return n[mod7 (deg)]; }
inline const char* noteLetter (int deg)  { static const char* n[] = { u8"η", u8"α", u8"β", u8"γ", u8"δ", u8"ε", u8"ζ" }; return n[mod7 (deg)]; }
inline const char* martyriaLetter (int deg) { static const char* n[] = { u8"ν", u8"π", u8"β", u8"γ", u8"δ", u8"κ", u8"ζ" }; return n[mod7 (deg)]; }
inline std::string octaveMark (int deg)
{
    int o = oct7 (deg);
    if (o >= 2) return u8"″";
    if (o == 1) return u8"′";
    if (o == -1) return ",";
    if (o <= -2) return ",,";
    return "";
}
inline std::string degreeLatin (int deg) { return std::string (noteLatin (deg)) + octaveMark (deg); }
inline std::string degreeGreek (int deg) { return std::string (noteGreek (deg)) + octaveMark (deg); }

// ---- phonetics used by the voice -------------------------------------------
// Consonants of Greek (and Latin-script) lyrics, in their Modern Greek ecclesiastical
// pronunciation: β = v, γ = gh, δ = dh, θ = th, χ = kh.
enum Consonant { ConsNone = 0, ConsP, ConsT, ConsK, ConsB, ConsD, ConsG, ConsV, ConsDh, ConsGh, ConsF, ConsTh,
                 ConsS, ConsZ, ConsX, ConsKs, ConsPs, ConsM, ConsN, ConsL, ConsR, NumConsonants };

enum ConsonantKind { KindStop = 0, KindVoicedStop, KindFricative, KindVoicedFricative, KindNasal, KindLateral, KindTap, KindAffricate };

struct ConsonantSpec
{
    int kind;
    float noiseLo, noiseHi;   // frication / burst band (Hz)
    float noiseGain;
    float ms;                 // total duration
    float f1, f2, f3;         // formant locus during the consonant (0 = keep vowel)
};

inline const ConsonantSpec& consonantSpec (int c)
{
    static const ConsonantSpec t[NumConsonants] = {
        { KindStop,            0,    0,    0.0f,  0,   0,    0,    0 },    // none
        { KindStop,            500,  1600, 0.55f, 70,  300,  900,  2200 }, // p
        { KindStop,            2800, 6500, 0.65f, 70,  300,  1700, 2700 }, // t
        { KindStop,            1400, 3200, 0.75f, 80,  300,  1600, 2400 }, // k
        { KindVoicedStop,      500,  1600, 0.30f, 60,  250,  900,  2200 }, // b
        { KindVoicedStop,      2800, 6500, 0.35f, 60,  250,  1700, 2700 }, // d
        { KindVoicedStop,      1400, 3200, 0.40f, 65,  250,  1600, 2400 }, // g
        { KindVoicedFricative, 1200, 6000, 0.14f, 60,  300,  1100, 2300 }, // v (β)
        { KindVoicedFricative, 1600, 7000, 0.12f, 55,  300,  1500, 2600 }, // dh (δ)
        { KindVoicedFricative, 1000, 3200, 0.20f, 55,  300,  1500, 2400 }, // gh (γ)
        { KindFricative,       1200, 7500, 0.30f, 80,  0,    0,    0 },    // f (φ)
        { KindFricative,       1600, 8000, 0.25f, 80,  0,    0,    0 },    // th (θ)
        { KindFricative,       4500, 9500, 0.60f, 90,  0,    0,    0 },    // s (σ)
        { KindVoicedFricative, 4000, 9000, 0.35f, 75,  300,  1600, 2600 }, // z (ζ)
        { KindFricative,       1200, 3600, 0.45f, 80,  0,    0,    0 },    // kh (χ)
        { KindAffricate,       4500, 9500, 0.60f, 110, 0,    0,    0 },    // ks (ξ)
        { KindAffricate,       4500, 9500, 0.60f, 110, 0,    0,    0 },    // ps (ψ)
        { KindNasal,           0,    0,    0.0f,  70,  250,  1100, 2200 }, // m
        { KindNasal,           0,    0,    0.0f,  65,  250,  1500, 2500 }, // n
        { KindLateral,         0,    0,    0.0f,  50,  350,  1000, 2700 }, // l
        { KindTap,             0,    0,    0.0f,  35,  0,    0,    0 },    // r (tapped)
    };
    return t[std::clamp (c, 0, (int) NumConsonants - 1)];
}

// Sung vowel / consonant of each parallage syllable: Νη Πα Βου Γα Δι Κε Ζω.
inline char parallageVowel (int deg) { static const char v[] = { 'i', 'a', 'u', 'a', 'i', 'e', 'o' }; return v[mod7 (deg)]; }
inline int  parallageConsonant (int deg) { static const int c[] = { ConsN, ConsP, ConsV, ConsGh, ConsDh, ConsK, ConsZ }; return c[mod7 (deg)]; }

struct Formula
{
    const char* function;
    const char* name;
    std::vector<int> degs;
};

struct Variant
{
    const char* name;
    int genus;
    int base;
    std::vector<int> dominants;
    std::vector<int> isonNotes;
    const char* apechema;            // syllables separated by '-'
    std::vector<int> apechemaMel;    // one degree per syllable (simplified intonation)
    std::vector<Formula> formulas;
};

struct Mode
{
    const char* greek;
    const char* shortGreek;
    const char* english;
    const char* character;
    std::vector<Variant> variants;
};

static constexpr const char* kFunctions[] = { "Opening", "Ascent", "Descent", "Medial cadence", "Final cadence", "Melismatic" };
static constexpr int kNumFunctions = 6;

// The formula library: characteristic movements compiled for practice and composition.
// They are idiomatic shapes for each mode, not transcriptions of particular hymns.
inline const std::vector<Mode>& modes()
{
    using F = std::vector<Formula>;
    static const F first {
        { "Opening", "Rise from Pa to Di", { 1, 2, 3, 4, 4 } },
        { "Opening", "Turning on Pa", { 1, 1, 2, 3, 2, 1 } },
        { "Ascent", "Upper tetrachord, turning at Zo", { 3, 4, 5, 6, 5, 4 } },
        { "Descent", "Through Ni back to Pa", { 4, 3, 2, 1, 0, 1 } },
        { "Medial cadence", "Medial cadence on Di", { 2, 3, 4, 3, 4 } },
        { "Medial cadence", "Medial cadence on Ga", { 1, 2, 3, 2, 3 } },
        { "Final cadence", "Final cadence on Pa", { 3, 2, 3, 2, 1, 1 } },
        { "Melismatic", "Melisma around Pa", { 1, 2, 1, 2, 3, 2, 1, 0, 1 } },
    };
    static const F second {
        { "Opening", "Di – Ke – Zo turn", { 4, 5, 6, 5, 4 } },
        { "Opening", "Rise from Vou", { 2, 3, 4, 4 } },
        { "Ascent", "To upper Ni", { 4, 5, 6, 7, 6, 5 } },
        { "Descent", "Zo down to Vou", { 6, 5, 4, 3, 2 } },
        { "Medial cadence", "Medial cadence on Vou", { 4, 3, 2, 2 } },
        { "Medial cadence", "Rest on Ke", { 4, 5, 5 } },
        { "Final cadence", "Final cadence on Di", { 6, 5, 4, 3, 4, 4 } },
        { "Melismatic", "Melisma around Di", { 4, 5, 4, 3, 4, 5, 6, 5, 4 } },
    };
    static const F third {
        { "Opening", "Ga – Ke arch", { 3, 4, 5, 4, 3 } },
        { "Opening", "Rise from Pa", { 1, 2, 3, 3 } },
        { "Ascent", "To upper Ni", { 3, 4, 5, 6, 7, 6, 5 } },
        { "Descent", "Ke down to Pa", { 5, 4, 3, 2, 1 } },
        { "Medial cadence", "Medial cadence on Pa", { 3, 2, 1, 1 } },
        { "Medial cadence", "Medial cadence on Ke", { 4, 5, 5 } },
        { "Final cadence", "Final cadence on Ga", { 5, 4, 3, 2, 3, 3 } },
        { "Melismatic", "Melisma around Ga", { 3, 4, 3, 2, 3, 4, 5, 4, 3 } },
    };
    static const F legetos {
        { "Opening", "Vou – Di arch", { 2, 3, 4, 3, 2 } },
        { "Opening", "Rise from Pa", { 1, 2, 3, 2 } },
        { "Ascent", "Up to Ke", { 2, 3, 4, 5, 4 } },
        { "Descent", "Di down to Pa", { 4, 3, 2, 1, 2 } },
        { "Medial cadence", "Medial cadence on Di", { 3, 4, 4 } },
        { "Medial cadence", "Medial cadence on Pa", { 2, 1, 1 } },
        { "Final cadence", "Final cadence on Vou", { 4, 3, 2, 1, 2, 2 } },
        { "Melismatic", "Melisma around Vou", { 2, 3, 2, 1, 2, 3, 4, 3, 2 } },
    };
    static const F agia {
        { "Opening", "Di – Zo arch", { 4, 5, 6, 5, 4 } },
        { "Opening", "Rise from Vou", { 2, 3, 4, 4 } },
        { "Ascent", "To upper Ni", { 4, 5, 6, 7, 6 } },
        { "Descent", "Zo down to Vou", { 6, 5, 4, 3, 2 } },
        { "Medial cadence", "Medial cadence on Vou", { 4, 3, 2, 2 } },
        { "Medial cadence", "Rest on Di", { 5, 4, 4 } },
        { "Final cadence", "Final cadence on Di", { 6, 5, 4, 3, 4, 4 } },
        { "Melismatic", "Melisma around Di", { 4, 5, 6, 5, 4, 3, 4, 5, 4 } },
    };
    static const F plagal1 {
        { "Opening", "Pa – Ga arch", { 1, 2, 3, 2, 1 } },
        { "Opening", "Rise from low Ke", { -2, -1, 0, 1, 1 } },
        { "Ascent", "Up to Ke", { 1, 2, 3, 4, 5, 4 } },
        { "Descent", "Through low Zo to Pa", { 3, 2, 1, 0, -1, 0, 1 } },
        { "Medial cadence", "Medial cadence on Ga", { 1, 2, 3, 3 } },
        { "Medial cadence", "Medial cadence on Di", { 3, 4, 4 } },
        { "Final cadence", "Final cadence on Pa", { 2, 3, 2, 1, 0, 1, 1 } },
        { "Melismatic", "Melisma around Pa", { 1, 0, 1, 2, 1, 0, -1, 0, 1 } },
    };
    static const F plagal2 {
        { "Opening", "Pa to Di", { 1, 2, 3, 4, 3 } },
        { "Opening", "Descent from Di", { 4, 3, 2, 1 } },
        { "Ascent", "Through Ke to Zo", { 1, 2, 3, 4, 5, 6, 5, 4 } },
        { "Descent", "Di down to Ni", { 4, 3, 2, 1, 0, 1 } },
        { "Medial cadence", "Medial cadence on Di", { 2, 3, 4, 4 } },
        { "Medial cadence", "Medial cadence on Ga", { 1, 2, 3, 3 } },
        { "Final cadence", "Final cadence on Pa", { 3, 2, 1, 2, 1, 1 } },
        { "Melismatic", "Augmented-second melisma", { 1, 2, 3, 2, 1, 2, 3, 4, 3, 2, 1 } },
    };
    static const F varysDia {
        { "Opening", "Low Zo to Vou", { -1, 0, 1, 2, 1 } },
        { "Opening", "Descent to low Zo", { 2, 1, 0, -1 } },
        { "Ascent", "Up to Ga", { -1, 0, 1, 2, 3, 2 } },
        { "Descent", "Down to low Ke", { 2, 1, 0, -1, -2, -1 } },
        { "Medial cadence", "Medial cadence on Vou", { 0, 1, 2, 2 } },
        { "Medial cadence", "Medial cadence on Ga", { 2, 3, 3 } },
        { "Final cadence", "Final cadence on low Zo", { 1, 0, -1, 0, -1, -1 } },
        { "Melismatic", "Melisma around low Zo", { -1, 0, 1, 0, -1, -2, -1, 0, -1 } },
    };
    static const F varysEnh {
        { "Opening", "Ga down to Pa and back", { 3, 2, 1, 2, 3 } },
        { "Opening", "Ga to Ke", { 3, 4, 5, 4 } },
        { "Ascent", "Up to Zo", { 3, 4, 5, 6, 5 } },
        { "Descent", "Ke down to Pa", { 5, 4, 3, 2, 1, 2 } },
        { "Medial cadence", "Medial cadence on Pa", { 3, 2, 1, 1 } },
        { "Medial cadence", "Medial cadence on Ke", { 4, 5, 5 } },
        { "Final cadence", "Final cadence on Ga", { 4, 3, 2, 3, 3 } },
        { "Melismatic", "Melisma around Ga", { 3, 2, 1, 2, 3, 4, 3, 2, 3 } },
    };
    static const F plagal4 {
        { "Opening", "Ni – Vou arch", { 0, 1, 2, 1, 0 } },
        { "Opening", "Rise to Di", { 0, 1, 2, 3, 4 } },
        { "Ascent", "Up to upper Ni", { 2, 3, 4, 5, 6, 7, 6 } },
        { "Descent", "Di down to Ni", { 4, 3, 2, 1, 0 } },
        { "Medial cadence", "Medial cadence on Vou", { 0, 1, 2, 2 } },
        { "Medial cadence", "Medial cadence on Di", { 3, 4, 4 } },
        { "Final cadence", "Final cadence on Ni", { 2, 1, 0, 1, 0, 0 } },
        { "Melismatic", "Melisma around Ni", { 0, 1, 2, 1, 0, -1, 0, 1, 0 } },
    };

    static const std::vector<Mode> m {
        { u8"Ἦχος Πρῶτος", u8"Ἦχος α′", "First Mode",
          "Diatonic, base Pa. Melodies move between Pa and Di; in sticheraric and papadic melody Ga is a further point of rest. "
          "Zo is drawn down toward Ke when the melody turns below it.",
          { { "Heirmologic", Diatonic, 1, { 1, 4 }, { 1, 4 }, u8"Ἀ-να-νές", { 1, 2, 1 }, first },
            { "Sticheraric", Diatonic, 1, { 1, 3, 4 }, { 1, 3, 4 }, u8"Ἀ-να-νές", { 1, 2, 1 }, first },
            { "Papadic", Diatonic, 1, { 1, 3, 4, 5 }, { 1, 3, 4, -2 }, u8"Ἀ-να-νές", { 1, 2, 1 }, first } } },
        { u8"Ἦχος Δεύτερος", u8"Ἦχος β′", "Second Mode",
          "Soft chromatic genus: tetrachords of 8–14–8 moria joined by a 12-moria tone. Base Di; Vou is the chief medial point. "
          "Many compositions move between soft chromatic and diatonic passages.",
          { { "Heirmologic", SoftChromatic, 4, { 4, 2 }, { 4, 2 }, u8"Νε-χέ-α-νες", { 4, 5, 4, 4 }, second },
            { "Sticheraric", SoftChromatic, 4, { 4, 2, 6 }, { 4, 2 }, u8"Νε-χέ-α-νες", { 4, 5, 4, 4 }, second } } },
        { u8"Ἦχος Τρίτος", u8"Ἦχος γ′", "Third Mode",
          "Enharmonic genus (12–12–6) on Ga: Vou and Zo are sung low, a small 6-moria step below Ga and Ni. "
          "A bright, firm character; cadences on Ga with medial rests on Pa and Ke.",
          { { "Heirmologic / Sticheraric", Enharmonic, 3, { 3, 1, 5 }, { 3, 1 }, u8"Ἀ-α-νές", { 3, 4, 3 }, third } } },
        { u8"Ἦχος Τέταρτος", u8"Ἦχος δ′", "Fourth Mode",
          "Diatonic, in two main forms: Legetos with its base on Vou (heirmologic) and Agia with its base on Di "
          "(sticheraric and papadic).",
          { { "Legetos (on Vou)", Diatonic, 2, { 2, 4, 1 }, { 2, 4, 1 }, u8"Ἅ-γι-α", { 2, 3, 2 }, legetos },
            { "Agia (on Di)", Diatonic, 4, { 4, 2, 6 }, { 4, 2 }, u8"Ἅ-γι-α", { 4, 5, 4 }, agia } } },
        { u8"Ἦχος Πλάγιος τοῦ Πρώτου", u8"Ἦχος πλ. α′", "Plagal First",
          "Diatonic, base Pa in a lower range than First Mode, with low Ke as a lower support. Ga and Di are medial points.",
          { { "Heirmologic / Sticheraric", Diatonic, 1, { 1, 3, 4 }, { 1, 3, 4, -2 }, u8"Ἀ-α-νε-α-νές", { 1, 2, 3, 2, 1 }, plagal1 } } },
        { u8"Ἦχος Πλάγιος τοῦ Δευτέρου", u8"Ἦχος πλ. β′", "Plagal Second",
          "Hard chromatic genus (6–20–4): the wide 20-moria step Vou–Ga gives the mode its colour. Base Pa; Di is the main medial point.",
          { { "Heirmologic / Sticheraric", HardChromatic, 1, { 1, 4 }, { 1, 4, 0 }, u8"Νε-α-νές", { 1, 2, 1 }, plagal2 } } },
        { u8"Ἦχος Βαρύς", u8"Ἦχος βαρύς", "Varys (Grave Mode)",
          "Two forms: diatonic with its base on low Zo, and enharmonic with its base on Ga, sharing Third Mode's scale "
          "with heavier, lower cadences.",
          { { "Diatonic (on low Zo)", Diatonic, -1, { -1, 2, 3 }, { -1, 2, 3 }, u8"Ἀ-α-α-νές", { -1, 0, 1, -1 }, varysDia },
            { "Enharmonic (on Ga)", Enharmonic, 3, { 3, 1, 5 }, { 3, 1 }, u8"Ἀ-α-α-νές", { 3, 4, 2, 3 }, varysEnh } } },
        { u8"Ἦχος Πλάγιος τοῦ Τετάρτου", u8"Ἦχος πλ. δ′", "Plagal Fourth",
          "Diatonic, base Ni. Broad and stable; Vou and Di are the medial points.",
          { { "Heirmologic", Diatonic, 0, { 0, 2, 4 }, { 0, 2, 4 }, u8"Νε-α-γι-έ", { 0, 1, 2, 0 }, plagal4 },
            { "Sticheraric / Papadic", Diatonic, 0, { 0, 2, 4, 1 }, { 0, 2, 4, 1 }, u8"Νε-α-γι-έ", { 0, 1, 2, 0 }, plagal4 } } },
    };
    return m;
}

inline const Variant& variantOf (int mode, int variant)
{
    const auto& ms = modes();
    const auto& md = ms[(size_t) std::clamp (mode, 0, (int) ms.size() - 1)];
    return md.variants[(size_t) std::clamp (variant, 0, (int) md.variants.size() - 1)];
}

inline bool contains (const std::vector<int>& v, int x) { return std::find (v.begin(), v.end(), x) != v.end(); }

// Melodic attraction (ἕλξις): in the diatonic genus Zo is drawn down toward Ke
// (sung with a 4-moria ὕφεσις) when the melody turns back below it.
inline int attraction (int prevDeg, int deg, int nextDeg, bool hasNext, int genus)
{
    if (genus == Diatonic && mod7 (deg) == 6 && hasNext && nextDeg < deg && prevDeg <= deg)
        return -4;
    return 0;
}

// ---- Exercises -----------------------------------------------------------
enum ExerciseKind { ExScale = 0, ExSteps, ExLeaps, ExCadences };

inline std::vector<int> makeExercise (const Variant& v, int kind, int length, std::mt19937& rng)
{
    std::vector<int> out;
    const int b = v.base;
    auto rnd = [&] (int lo, int hi) { return std::uniform_int_distribution<int> (lo, hi) (rng); };
    if (kind == ExScale)
    {
        for (int d = b; d <= b + 7; ++d) out.push_back (d);
        for (int d = b + 6; d >= b; --d) out.push_back (d);
        return out;
    }
    if (kind == ExCadences)
    {
        std::vector<const Formula*> cad;
        for (auto& f : v.formulas)
            if (std::string (f.function).find ("cadence") != std::string::npos) cad.push_back (&f);
        while ((int) out.size() < length && ! cad.empty())
        {
            auto* f = cad[(size_t) rnd (0, (int) cad.size() - 1)];
            out.insert (out.end(), f->degs.begin(), f->degs.end());
        }
        return out;
    }
    int d = b;
    out.push_back (d);
    const int lo = b - 2, hi = b + 6;
    while ((int) out.size() < length - 1)
    {
        int step;
        if (kind == ExSteps) step = rnd (0, 1) ? 1 : -1;
        else { int mag = rnd (2, 4); step = rnd (0, 1) ? mag : -mag; }
        if (d + step < lo || d + step > hi) step = -step;
        d = std::clamp (d + step, lo, hi);
        out.push_back (d);
    }
    // close on the base by step
    while (d != b) { d += (b > d ? 1 : -1); out.push_back (d); }
    return out;
}

} // namespace byz
