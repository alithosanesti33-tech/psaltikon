// Offline checks: theory tables, timing, ison analysis, melody generation,
// audio rendering and PDF engraving. Build with -DPSALTIKON_TESTS=ON.
#include <JuceHeader.h>
#include "../Source/Exporters.h"

using namespace byz;

static int failures = 0;
#define CHECK(c) do { if (! (c)) { std::cerr << "FAIL: " #c " (line " << __LINE__ << ")\n"; ++failures; } } while (0)

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const juce::File out (argc > 1 ? argv[1] : "/tmp/psaltikon-test");
    out.createDirectory();

    // every genus spans exactly one 72-moria octave and each mode keeps its base pitch
    for (int g = 0; g < NumGenera; ++g) CHECK (moriaOf (7, g) - moriaOf (0, g) == 72);
    CHECK (moriaOf (1, HardChromatic) == moriaOf (1, Diatonic));
    CHECK (moriaOf (3, Enharmonic) == moriaOf (3, Diatonic));
    CHECK (moriaOf (4, SoftChromatic) == moriaOf (4, Diatonic));
    CHECK (moriaOf (2, Diatonic) - moriaOf (1, Diatonic) == 10);
    CHECK (moriaOf (3, HardChromatic) - moriaOf (2, HardChromatic) == 20);

    auto c = sampleComposition();
    auto d = derive (c, true);
    CHECK (d.total > 10);
    CHECK (d.deg.back() == 1);           // ends on Pa
    auto tl = buildTimeline (c, {});
    CHECK (! tl->ctrls.empty());
    CHECK (! tl->ison.empty());
    for (size_t i = 1; i < tl->ctrls.size(); ++i) CHECK (tl->ctrls[i].beat >= tl->ctrls[i - 1].beat);

    // gorgon halves two notes
    Composition g; g.startDeg = 1;
    insertDegrees (g, 0, { 1, 2, 3 });
    g.events[2].time = Gorgon;
    auto gd = derive (g, false);
    CHECK (std::abs (gd.beats[1] - 0.5) < 1e-9 && std::abs (gd.beats[2] - 0.5) < 1e-9 && std::abs (gd.total - 2.0) < 1e-9);

    // JSON round trip
    Composition r;
    CHECK (fromVar (juce::JSON::parse (juce::JSON::toString (toVar (c))), r));
    CHECK (r.events.size() == c.events.size() && r.events[0].lyric == c.events[0].lyric);

    // composition in every mode ends on the base of the mode
    for (int m = 0; m < 8; ++m)
        for (int v = 0; v < (int) modes()[(size_t) m].variants.size(); ++v)
        {
            auto cc = composeFromText (juce::String (juce::CharPointer_UTF8 (u8"Κύ-ρι-ε ἐ-κέ-κρα-ξα πρὸς σέ, εἰ-σά-κου-σόν μου.")), Neumatic, m, v, 90, 42);
            auto dd = derive (cc, true);
            CHECK (! cc.events.empty());
            CHECK (dd.deg.back() == cc.var().base);
            bool hasIson = false; for (auto& e : cc.events) if (e.ison != kNoIson) hasIson = true;
            CHECK (hasIson);
            for (auto& e : cc.events) CHECK (e.delta >= -7 && e.delta <= 7);
        }

    // composing with parallage syllables
    {
        std::vector<int> pd; int bad = -1;
        CHECK (parseParallage (juce::String (juce::CharPointer_UTF8 (u8"Πα Βου Γα Δι Κε Ζω Νη Πα' Νη Ζω, Pa")), 1, pd, bad));
        CHECK (pd.size() == 11 && pd[0] == 1 && pd[3] == 4 && pd[6] == 7 && pd[7] == 8 && pd[9] == -1 && pd[10] == 1);
        CHECK (! parseParallage ("Pa Xx", 1, pd, bad) && bad == 1);
    }

    // capitals: consonant and vowel of "Κύ" and "ΑΙ"
    CHECK (lyricConsonant (juce::String (juce::CharPointer_UTF8 (u8"Κύ"))) == ConsK);
    { int liq = 0; CHECK (lyricConsonant (juce::String (juce::CharPointer_UTF8 (u8"κρα")), &liq) == ConsK && liq == ConsR); }
    CHECK (lyricConsonant (juce::String (juce::CharPointer_UTF8 (u8"μπα"))) == ConsB);
    CHECK (lyricVowel (juce::String (juce::CharPointer_UTF8 (u8"Κύ")), 0) == 2);

    // audio
    TimelineOptions o; PerformParams p;
    const auto& vp = voiceProfiles()[1];
    p.voice.niHz = vp.niHz; p.voice.formantScale = vp.formantScale; p.voice.brightness = vp.brightness;
    p.voice.vibDepth = vp.vibDepth; p.voice.vibRate = vp.vibRate; p.voice.ring = vp.ring; p.voice.breath = vp.breath; p.voice.nasal = vp.nasal;
    auto wav = out.getChildFile ("sample.wav");
    CHECK (renderWav (c, o, p, 0, wav));
    juce::AudioFormatManager fm; fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> rd (fm.createReaderFor (wav));
    CHECK (rd != nullptr);
    if (rd)
    {
        juce::AudioBuffer<float> b (2, (int) rd->lengthInSamples);
        rd->read (&b, 0, (int) rd->lengthInSamples, 0, true, true);
        const float pk = b.getMagnitude (0, b.getNumSamples());
        const float rms = b.getRMSLevel (0, 0, b.getNumSamples());
        std::cout << "wav seconds " << rd->lengthInSamples / rd->sampleRate << " peak " << pk << " rms " << rms << "\n";
        CHECK (pk > 0.05f && pk <= 1.0f);
        CHECK (rms > 0.005f);
        bool finite = true; for (int s = 0; s < b.getNumSamples(); ++s) if (! std::isfinite (b.getSample (0, s))) finite = false;
        CHECK (finite);
    }
    CHECK (renderWav (c, o, p, 1, out.getChildFile ("melody.wav")));
    // all five voices and ison-only render
    for (int vi = 0; vi < 5; ++vi)
    {
        auto pp = p; const auto& v = voiceProfiles()[(size_t) vi];
        pp.voice.niHz = v.niHz; pp.voice.formantScale = v.formantScale; pp.voice.brightness = v.brightness; pp.voice.nasal = v.nasal;
        CHECK (renderWav (c, o, pp, vi == 4 ? 2 : 0, out.getChildFile ("voice" + juce::String (vi) + ".wav")));
    }
    // melismatic, other mode
    auto m2 = composeFromText (juce::String (juce::CharPointer_UTF8 (u8"Ἅ-γι-ος ὁ Θε-ός.")), Melismatic, 5, 0, 80, 3);
    CHECK (renderWav (m2, o, p, 0, out.getChildFile ("plagal2.wav")));

    auto pages = renderPages (c, true);
    CHECK (! pages.empty());
    CHECK (writePdf (pages, out.getChildFile ("sample.pdf"), c.title));
    juce::PNGImageFormat png; juce::FileOutputStream os (out.getChildFile ("page1.png"));
    os.setPosition (0); os.truncate(); png.writeImageToStream (pages[0], os);
    auto longComp = composeFromText (juce::String (juce::CharPointer_UTF8 (u8"Κύ-ρι-ε ἐ-κέ-κρα-ξα πρὸς σέ, εἰ-σά-κου-σόν μου· εἰ-σά-κου-σόν μου, Κύ-ρι-ε. Κύ-ρι-ε ἐ-κέ-κρα-ξα πρὸς σέ, εἰ-σά-κου-σόν μου· πρόσ-χες τῇ φω-νῇ τῆς δε-ή-σε-ώς μου, ἐν τῷ κε-κρα-γέ-ναι με πρὸς σέ. Εἰ-σά-κου-σόν μου, Κύ-ρι-ε.")), Melismatic, 0, 1, 90, 9);
    auto lp = renderPages (longComp, true);
    std::cout << "long score pages " << lp.size() << "\n";
    CHECK (writePdf (lp, out.getChildFile ("long.pdf"), "Long"));
    for (size_t i = 0; i < lp.size(); ++i) { juce::FileOutputStream o2 (out.getChildFile ("long" + juce::String ((int) i + 1) + ".png")); o2.setPosition (0); o2.truncate(); png.writeImageToStream (lp[i], o2); }

    std::cout << (failures == 0 ? "ALL CHECKS PASSED" : "FAILURES: " + std::to_string (failures)) << "\n";
    return failures == 0 ? 0 : 1;
}
