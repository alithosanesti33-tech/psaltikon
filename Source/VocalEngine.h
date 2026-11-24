// Psaltikon — vocal engine.
//
// A source-filter model of the singing voice:
//  * Glottal source: a Rosenberg/LF-style glottal flow pulse with a smoothed return phase,
//    differentiated (lip radiation) and stored as band-limited tables for three voice
//    qualities (breathy, modal, pressed). Vocal effort crossfades between them, so the voice
//    brightens naturally as it gets louder and higher.
//  * Cycle-to-cycle jitter and shimmer, slow pitch drift, vibrato with wandering rate,
//    and pitch movement through a damped second-order system (natural portamento with a
//    small overshoot on leaps), plus the scoop singers use at a syllable onset.
//  * Vocal tract: five Klatt cascade resonators (so formant amplitudes follow from the
//    resonances themselves, as in a real tract), a nasal pole-zero pair for the nasal
//    colour of the psaltic voice, and the clustering of F3–F5 that forms the singer's formant.
//  * Aspiration noise gated by the glottal open phase.
//  * Consonants with their own articulation: closures, bursts and aspiration for stops,
//    band-limited frication for fricatives (voiced or not), nasal murmur, laterals and taps,
//    with formant transitions into the vowel.
#pragma once
#include <JuceHeader.h>
#include "ByzTheory.h"

namespace byz
{
struct VoiceProfile
{
    const char* name;
    float niHz;          // pitch of Ni for this voice
    float formantScale;  // vocal tract length (smaller = longer tract, darker)
    float brightness;    // base vocal effort 0..1
    float vibDepth;      // cents
    float vibRate;       // Hz
    float ring;          // singer's formant
    float breath;        // aspiration
    float nasal;         // nasal colour
};

inline const std::array<VoiceProfile, 5>& voiceProfiles()
{
    static const std::array<VoiceProfile, 5> p {{
        { "Deep resonant bass",          98.0f, 0.93f, 0.40f, 12.0f, 5.0f, 0.40f, 0.040f, 0.25f },
        { "Warm baritone",              110.0f, 0.99f, 0.48f, 14.0f, 5.2f, 0.50f, 0.035f, 0.30f },
        { "Clear tenor",                130.8f, 1.05f, 0.56f, 15.0f, 5.5f, 0.60f, 0.030f, 0.30f },
        { "High-register psaltis",      146.8f, 1.08f, 0.66f,  9.0f, 5.9f, 0.75f, 0.028f, 0.55f },
        { "Restrained ecclesiastical",  116.5f, 0.99f, 0.40f,  5.0f, 4.8f, 0.35f, 0.030f, 0.30f },
    }};
    return p;
}

// Male vowel formants (Hz) and bandwidths for a e i o u and a closed hum.
struct VowelSpec { float f[5]; float bw[5]; };
inline const VowelSpec& vowelSpec (int v)
{
    static const VowelSpec t[6] = {
        { { 700, 1180, 2600, 3300, 3750 }, { 110, 75, 140, 220, 220 } },   // a
        { { 460, 1750, 2550, 3350, 3750 }, {  70, 95, 140, 220, 220 } },   // e
        { { 300, 2150, 2950, 3400, 3800 }, {  55, 95, 150, 200, 220 } },   // i
        { { 460,  860, 2500, 3300, 3750 }, {  75, 80, 120, 220, 220 } },   // o
        { { 330,  740, 2400, 3300, 3750 }, {  65, 80, 110, 220, 220 } },   // u
        { { 260, 1100, 2300, 3300, 3750 }, {  90, 200, 300, 400, 400 } },  // m (hum)
    };
    return t[std::clamp (v, 0, 5)];
}

// ---------------------------------------------------------------------------------------
// Band-limited glottal pulse tables: 3 qualities x 10 octave levels.
class GlottalTables
{
public:
    static constexpr int kSize = 2048, kLevels = 10, kQualities = 3;

    void build (double sr)
    {
        if (std::abs (sr - builtFor) < 1.0) return;
        builtFor = sr;
        struct Q { double oq, am, ta; };
        static const Q qs[kQualities] = { { 0.72, 0.62, 0.012 }, { 0.60, 0.70, 0.004 }, { 0.50, 0.80, 0.0015 } };
        constexpr int hiN = 8192, order = 13;
        juce::dsp::FFT fwd (order), inv (11);
        for (int q = 0; q < kQualities; ++q)
        {
            // glottal flow over one period, then a cyclic one-pole for the return phase
            std::vector<double> g (hiN);
            const double Te = qs[q].oq, Tp = Te * qs[q].am;
            for (int n = 0; n < hiN; ++n)
            {
                const double t = (double) n / hiN;
                g[(size_t) n] = t < Tp ? 0.5 * (1.0 - std::cos (juce::MathConstants<double>::pi * t / Tp))
                              : t < Te ? std::cos (juce::MathConstants<double>::halfPi * (t - Tp) / (Te - Tp)) : 0.0;
            }
            const double a = std::exp (-1.0 / (qs[q].ta * hiN));
            double y = 0;
            for (int pass = 0; pass < 3; ++pass)
                for (int n = 0; n < hiN; ++n) { y = a * y + (1 - a) * g[(size_t) n]; if (pass == 2) g[(size_t) n] = y; }
            std::vector<float> buf ((size_t) hiN * 2, 0.0f);
            for (int n = 0; n < hiN; ++n) buf[(size_t) n] = (float) (g[(size_t) n] - g[(size_t) ((n + hiN - 1) % hiN)]);   // derivative
            fwd.performRealOnlyForwardTransform (buf.data(), true);   // interleaved complex bins 0..hiN/2

            float norm = 0;
            for (int l = 0; l < kLevels; ++l)
            {
                const double maxF = 40.0 * std::pow (2.0, l);
                const int H = std::max (1, std::min (kSize / 2 - 1, (int) (0.46 * sr / maxF)));
                std::vector<float> spec ((size_t) kSize * 2, 0.0f);
                for (int h = 1; h <= H; ++h)
                {
                    // gentle taper of the top harmonics avoids Gibbs ringing
                    const float w = h > H * 0.8 ? 0.5f + 0.5f * std::cos (juce::MathConstants<float>::pi * (float) (h - H * 0.8) / (float) (H * 0.2 + 1)) : 1.0f;
                    spec[(size_t) h * 2] = buf[(size_t) h * 2] * w;
                    spec[(size_t) h * 2 + 1] = buf[(size_t) h * 2 + 1] * w;
                }
                inv.performRealOnlyInverseTransform (spec.data());
                auto& t = tables[(size_t) q][(size_t) l];
                t.assign (kSize + 1, 0.0f);
                for (int j = 0; j < kSize; ++j) t[(size_t) j] = spec[(size_t) j];
                if (l == 0) { for (int j = 0; j < kSize; ++j) norm = std::max (norm, std::abs (t[(size_t) j])); }
                for (int j = 0; j < kSize; ++j) t[(size_t) j] /= (norm > 0 ? norm : 1.0f);
                t[kSize] = t[0];
            }
        }
    }

    // quality 0..2 (fractional), returns sample
    inline float read (double phase, double freq, float quality) const
    {
        int l = (int) std::ceil (std::log2 (std::max (freq, 40.0) / 40.0));
        l = std::clamp (l, 0, kLevels - 1);
        const double p = phase * kSize;
        const int i = (int) p;
        const float f = (float) (p - i);
        const int q0 = std::clamp ((int) quality, 0, kQualities - 2);
        const float qf = std::clamp (quality - (float) q0, 0.0f, 1.0f);
        const auto& a = tables[(size_t) q0][(size_t) l];
        const auto& b = tables[(size_t) q0 + 1][(size_t) l];
        const float va = a[(size_t) i] + f * (a[(size_t) i + 1] - a[(size_t) i]);
        const float vb = b[(size_t) i] + f * (b[(size_t) i + 1] - b[(size_t) i]);
        return va + qf * (vb - va);
    }
private:
    double builtFor = 0;
    std::array<std::array<std::vector<float>, kLevels>, kQualities> tables;
};

// Klatt resonator / antiresonator
struct Reson
{
    double a = 1, b = 0, c = 0, y1 = 0, y2 = 0;
    void set (double f, double bw, double sr)
    {
        const double T = 1.0 / sr;
        c = -std::exp (-2.0 * juce::MathConstants<double>::pi * bw * T);
        b = 2.0 * std::exp (-juce::MathConstants<double>::pi * bw * T) * std::cos (2.0 * juce::MathConstants<double>::pi * f * T);
        a = 1.0 - b - c;
    }
    inline double process (double x) { const double y = a * x + b * y1 + c * y2; y2 = y1; y1 = y; return y; }
    void reset() { y1 = y2 = 0; }
};
struct AntiReson
{
    double a = 1, b = 0, c = 0, x1 = 0, x2 = 0;
    void set (double f, double bw, double sr)
    {
        Reson r; r.set (f, bw, sr);
        a = 1.0 / r.a; b = -r.b / r.a; c = -r.c / r.a;
    }
    inline double process (double x) { const double y = a * x + b * x1 + c * x2; x2 = x1; x1 = x; return y; }
    void reset() { x1 = x2 = 0; }
};

struct SVF  // TPT state-variable filter, normalised band-pass (for frication noise)
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
    float niHz = 110, formantScale = 1, brightness = 0.48f, vibDepth = 14, vibRate = 5.2f;
    float ring = 0.5f, breath = 0.035f, nasal = 0.3f, resonance = 0.5f, intensity = 0.6f;
    float detuneCents = 0;
};

class ChantVoice
{
public:
    void prepare (double sampleRate, const GlottalTables* t, uint32_t seed)
    {
        sr = sampleRate; tables = t; rng.seed (seed);
        for (auto& r : tract) r.reset();
        nasPole.reset(); nasZero.reset(); fric.reset(); dc1 = dc2 = 0; lpNas = 0;
        presence.reset(); presence.set (5200.0f, 0.75f, (float) sampleRate);
        shelfCoef = (float) (1.0 - std::exp (-juce::MathConstants<double>::twoPi * 700.0 / sampleRate)); srcLP = 0;
        f6.reset(); f6.set (4900.0, 350.0, sampleRate);
        const auto& v = vowelSpec (0);
        for (int k = 0; k < 5; ++k) { curF[k] = v.f[k]; curBW[k] = v.bw[k]; }
        vibRateCur = prm.vibRate;
        updateTract();
    }
    void setParams (const VoiceParams& p) { prm = p; }

    // pitch in moria above Ni; glideSec 0 = jump; scoop = start that many moria below
    void setPitch (float moria, float glideSec, float scoop = 0)
    {
        tgtM = moria;
        if (glideSec <= 0.0005f || ! active)
        {
            curM = moria; velM = 0;
            if (scoop > 0) scoopM = -scoop;
            omega = 60.0;
            return;
        }
        omega = 4.2 / std::max (0.012, (double) glideSec);  // ~settling time
        if (scoop > 0) scoopM = std::min (scoopM, (double) -scoop);
    }
    void setVowel (int v, float morphSec = 0.06f)
    {
        vowel = std::clamp (v, 0, 5);
        morph = 1.0 - std::exp (-8.0 / (std::max (0.004, (double) morphSec) * sr));
    }
    void setLevel (float target, float timeSec)
    {
        tgtLvl = target;
        lvlCoef = timeSec <= 0.001f ? 1.0 : 1.0 - std::exp (-1.0 / (timeSec * sr / 3.0));
        if (target > 0) active = true;
    }
    void articulate (int consonant, float strength, int liquid = ConsNone)
    {
        if (consonant <= ConsNone || consonant >= NumConsonants) return;
        cons = consonant; consLiquid = liquid;
        const auto& cs = consonantSpec (cons);
        consLen = (int) (sr * cs.ms * 0.001 * (0.8 + 0.4 * strength));
        liqLen = liquid != ConsNone ? (int) (sr * 0.03) : 0;
        consPos = 0;
        consStrength = 0.6f + 0.6f * strength;
        if (cs.noiseHi > 0)
        {
            const float fc = std::sqrt (cs.noiseLo * cs.noiseHi);
            fric.set (fc, fc / (cs.noiseHi - cs.noiseLo), (float) sr);
        }
        active = true;
    }
    void accent (float amount, float decaySec) { acc = amount; accCoef = 1.0 - std::exp (-1.0 / (std::max (0.02, (double) decaySec) * sr)); }
    void vibrato (float scale, float noteSec)
    {
        vibTarget = scale;
        vibDelay = (int) (sr * std::min (0.32, noteSec * 0.35));
        vibCur = std::min (vibCur, 0.25f * scale);
    }
    void breathIn() { inhale = 1.0f; active = true; }
    void silence() { tgtLvl = 0; lvl = 0; active = false; acc = 1; inhale = 0; cons = ConsNone; scoopM = 0; velM = 0; }
    bool isSounding() const { return active; }

    void render (float* out, int n)
    {
        if (! active) return;
        const double dt = 1.0 / sr;
        for (int i = 0; i < n; ++i)
        {
            // ---- slow controls (every 8 samples)
            if (--ctrlCount <= 0)
            {
                ctrlCount = 8;
                updateConsonantShape();
                for (int k = 0; k < 5; ++k)
                {
                    float tf = tgtF (k), tb = tgtBW (k);
                    if (locus > 0 && cLocus[k] > 0) { tf += (cLocus[k] * (float) prm.formantScale - tf) * locus; tb *= 1.0f + locus; }
                    curF[k] += (float) ((tf - curF[k]) * morph * 8.0);
                    curBW[k] += (float) ((tb - curBW[k]) * morph * 8.0);
                    curF[k] = std::clamp (curF[k], 150.0f, (float) (0.45 * sr));
                }
                updateTract();
                if (--driftCount <= 0) { driftCount = (int) (sr / 8 / 2.5); driftTgt = uni (rng) * 5.0f; rateTgt = prm.vibRate * (1.0f + uni (rng) * 0.07f); }
                drift += (driftTgt - drift) * 0.004f;
                vibRateCur += (rateTgt - vibRateCur) * 0.01f;
                if (vibDelay > 0) vibDelay -= 8; else vibCur += (vibTarget - vibCur) * 0.006f;
                // vocal effort: louder and higher singing is more pressed and brighter
                const float effTgt = std::clamp (prm.brightness * (0.55f + 0.7f * prm.intensity) + 0.3f * (float) (lvl * acc - 0.8)
                                                 + 0.18f * std::clamp ((float) (curM - 30.0) / 72.0f, -0.5f, 1.0f), 0.0f, 1.0f);
                effort += (effTgt - effort) * 0.01f;
            }

            // ---- pitch: damped second-order glide, scoop, drift, vibrato, jitter
            const double accM = omega * omega * (tgtM - curM) - 2.0 * 0.62 * omega * velM;
            velM += accM * dt; curM += velM * dt;
            scoopM *= scoopDecay;
            vibPh += vibRateCur * dt; if (vibPh >= 1) vibPh -= 1;
            const double vibC = std::sin (juce::MathConstants<double>::twoPi * vibPh) * prm.vibDepth * vibCur;
            const double f = prm.niHz * std::pow (2.0, (curM + scoopM) / 72.0 + (vibC + drift + prm.detuneCents) / 1200.0) * (1.0 + cycJit);
            phase += f * dt;
            if (phase >= 1.0)
            {
                phase -= 1.0;
                cycJit = gauss (rng) * 0.0022 * (1.0 + 2.0 * prm.breath);
                cycShim = 1.0f + (float) gauss (rng) * 0.03f;
            }

            // ---- amplitude
            lvl += (tgtLvl - lvl) * lvlCoef;
            acc += (1.0 - acc) * accCoef;
            const float voiceGain = (float) (lvl * acc) * cycShim * consVoicing;
            const float quality = 2.0f * effort;
            float src = tables->read (phase, f, quality) * voiceGain;
            // firm glottal closure of trained singers: a high shelf on the source that rises with effort
            srcLP += (src - srcLP) * shelfCoef;
            src += (src - srcLP) * (shelfBase + 1.6f * effort);

            // aspiration noise, stronger in the open phase of the glottal cycle
            const float noise = uni (rng);
            const float open = phase < 0.6 ? 1.0f : 0.35f;
            float tractIn = src + noise * (float) lvl * open * (prm.breath * (1.3f - effort) + consAsp);

            // ---- vocal tract (cascade) with nasal coupling
            double y = tractIn;
            if (nasalExtra > 0.01f || nasalActive) { y = nasZero.process (nasPole.process (y)); nasalActive = nasalExtra > 0.01f; }
            for (auto& r : tract) y = r.process (y);
            // higher-pole correction: the tract keeps resonating above F5 (F6 and the
            // diffuse energy that gives a voice its air and edge)
            y += f6.process (tractIn) * 0.55 + presence.bp (src) * (0.10f + 0.14f * effort);
            // nasal murmur: low-pass the output during m/n
            lpNas += (y - lpNas) * 0.12;
            y = y + (lpNas - y) * nasalBlend;

            // ---- frication and bursts bypass the tract
            if (consNoise > 0.0001f) y += fric.bp (noise) * consNoise * 1.5f;
            if (inhale > 0.001f) { y += fric.bp (noise) * inhale * 0.05f; inhale *= 0.99988f; }

            // radiation already in the source; remove DC
            const double o1 = y - dc1 + 0.995 * dc2; dc1 = y; dc2 = o1;
            out[i] += (float) (o1 * 0.2);

            if (tgtLvl <= 0 && lvl < 1.0e-4 && inhale < 0.001f && cons == ConsNone) { active = false; lvl = 0; }
        }
    }

private:
    float tgtF (int k) const
    {
        const auto& v = vowelSpec (vowel);
        float f = v.f[k] * prm.formantScale;
        // singer's formant: F3–F5 drawn together near 2.6–3 kHz
        // the bright "twang" of the psaltic voice adds to the same cluster
        const float cl = std::min (1.0f, prm.ring * (0.5f + 0.5f * prm.resonance) + 0.35f * prm.nasal);
        const float centre = 2750.0f * prm.formantScale;
        if (k == 2) f += (centre - 200.0f - f) * cl * 0.5f;
        if (k == 3) f += (centre + 150.0f - f) * cl * 0.7f;
        if (k == 4) f += (centre + 600.0f - f) * cl * 0.6f;
        return f;
    }
    float tgtBW (int k) const
    {
        const auto& v = vowelSpec (vowel);
        float bw = v.bw[k] / (0.7f + 0.8f * prm.resonance);
        if (k >= 2) bw *= 1.0f - 0.35f * prm.ring - 0.15f * prm.nasal;
        return std::max (35.0f, bw);
    }

    void updateTract()
    {
        for (int k = 0; k < 5; ++k) tract[(size_t) k].set (curF[k], curBW[k], sr);
        // nasal pole-zero: cancelled when the zero sits on the pole; nasality moves it up
        // nasal murmur of m / n: pole near 270 Hz, zero moving up with the closure
        const double nz = 270.0 + 400.0 * nasalExtra;
        nasPole.set (270.0, 160.0, sr);
        nasZero.set (nz, 160.0 + 80.0 * nasalExtra, sr);
    }

    // shapes voicing, noise, formant locus and nasality over the course of a consonant
    void updateConsonantShape()
    {
        consVoicing = 1.0f; consNoise = 0; consAsp = 0; locus = 0; nasalBlend = 0; nasalExtra = 0;
        if (cons == ConsNone) return;
        const auto& cs = consonantSpec (cons);
        const float t = (float) consPos / (float) std::max (1, consLen);
        const float rel = std::clamp ((t - 0.75f) / 0.25f, 0.0f, 1.0f);   // release into the vowel
        auto ramp = [] (float x) { return std::clamp (x, 0.0f, 1.0f); };
        for (int k = 0; k < 3; ++k) cLocus[k] = k == 0 ? cs.f1 : (k == 1 ? cs.f2 : cs.f3);
        cLocus[3] = cLocus[4] = 0;
        const float s = consStrength;
        switch (cs.kind)
        {
            case KindStop:
                if (t < 0.55f) consVoicing = 1.0f - ramp (t / 0.1f);                  // closure: silence
                else if (t < 0.65f) { consVoicing = 0; consNoise = cs.noiseGain * s * (1.0f - (t - 0.55f) / 0.1f); } // burst
                else { consVoicing = ramp ((t - 0.65f) / 0.3f); consAsp = 0.5f * s * (1.0f - ramp ((t - 0.65f) / 0.3f)); }
                locus = 1.0f - rel;
                break;
            case KindVoicedStop:
                if (t < 0.6f) { consVoicing = 0.18f; nasalBlend = 0.8f; }
                else if (t < 0.7f) { consVoicing = 0.6f; consNoise = cs.noiseGain * s * 0.6f; }
                else consVoicing = 0.6f + 0.4f * rel;
                locus = 1.0f - rel;
                break;
            case KindFricative:
                consVoicing = 1.0f - ramp (t / 0.12f) * (1.0f - rel);
                consNoise = cs.noiseGain * s * ramp (t / 0.1f) * (1.0f - rel);
                break;
            case KindVoicedFricative:
                consVoicing = 0.45f + 0.55f * rel;
                consNoise = cs.noiseGain * s * (1.0f - rel);
                locus = 1.0f - rel;
                break;
            case KindAffricate:
                if (t < 0.35f) consVoicing = 1.0f - ramp (t / 0.08f);
                else { consVoicing = rel; consNoise = cs.noiseGain * s * (t < 0.42f ? 1.4f : 1.0f) * (1.0f - rel); }
                break;
            case KindNasal:
                consVoicing = 0.55f + 0.45f * rel;
                nasalBlend = 0.85f * (1.0f - rel);
                nasalExtra = 2.5f * (1.0f - rel);
                locus = 1.0f - rel;
                break;
            case KindLateral:
                consVoicing = 0.75f + 0.25f * rel;
                locus = 1.0f - rel;
                break;
            case KindTap:
            {
                const float m = std::abs (t - 0.5f) * 2.0f;
                consVoicing = 0.3f + 0.7f * m;
                cLocus[2] = 1800; locus = 1.0f - m;
                break;
            }
            default: break;
        }
        consPos += 8;
        if (consPos >= consLen)
        {
            if (liqLen > 0)
            {
                // continue with the liquid of the onset (κρ-, πλ-)
                cons = consLiquid; consLiquid = ConsNone;
                consLen = liqLen; liqLen = 0; consPos = 0;
            }
            else cons = ConsNone;
        }
    }

    double sr = 48000;
    const GlottalTables* tables = nullptr;
    VoiceParams prm;
    std::mt19937 rng { 1 };
    std::uniform_real_distribution<float> uni { -1.0f, 1.0f };
    std::normal_distribution<double> gauss { 0.0, 1.0 };
    std::array<Reson, 5> tract;
    Reson nasPole; AntiReson nasZero;
    SVF fric, presence;
    Reson f6;
    float curF[5] {}, curBW[5] {}, cLocus[5] {};
    int vowel = 0, ctrlCount = 0, driftCount = 0, vibDelay = 0;
    double curM = 0, tgtM = 0, velM = 0, omega = 60, scoopM = 0, scoopDecay = 0.9997;
    double lvl = 0, tgtLvl = 0, lvlCoef = 0.001, acc = 1, accCoef = 0.001, morph = 0.02;
    double phase = 0, cycJit = 0, dc1 = 0, dc2 = 0, lpNas = 0;
    float srcLP = 0, shelfCoef = 0.09f, shelfBase = 1.2f;
    float cycShim = 1, effort = 0.5f, inhale = 0, drift = 0, driftTgt = 0;
    float vibPh = 0, vibCur = 0, vibTarget = 0, vibRateCur = 5.2f, rateTgt = 5.2f;
    int cons = ConsNone, consLiquid = ConsNone, consPos = 0, consLen = 1, liqLen = 0;
    float consStrength = 1, consVoicing = 1, consNoise = 0, consAsp = 0, locus = 0, nasalBlend = 0, nasalExtra = 0;
    bool active = false, nasalActive = false;
};

// Several chanters holding the ison: slightly detuned, spread, each breathing at
// different moments so the ison itself never stops.
class IsonChoir
{
public:
    static constexpr int kMax = 4;
    void prepare (double sampleRate, const GlottalTables* t)
    {
        sr = sampleRate;
        for (int i = 0; i < kMax; ++i)
        {
            voices[(size_t) i].prepare (sampleRate, t, 1234u + (uint32_t) i * 77u);
            breathAt[(size_t) i] = (int) (sr * (5.0 + 2.7 * i));
        }
    }
    void setParams (const VoiceParams& base, int count, int vowel)
    {
        n = std::clamp (count, 1, kMax);
        static const float det[] = { 0.0f, 6.5f, -7.5f, 11.0f };
        static const float fs[] = { 1.0f, 0.97f, 1.03f, 0.99f };
        for (int i = 0; i < kMax; ++i)
        {
            auto p = base;
            p.detuneCents = det[i];
            p.formantScale *= fs[i];
            p.vibDepth *= 0.2f;
            p.vibRate *= 0.8f + 0.1f * (float) i;
            p.ring *= 0.55f;
            p.brightness *= 0.8f;
            voices[(size_t) i].setParams (p);
            if (vowel != curVowel) voices[(size_t) i].setVowel (vowel, 0.25f);
        }
        curVowel = vowel;
    }
    void start (float moria)
    {
        target = moria;
        holding = true;
        for (int i = 0; i < kMax; ++i)
        {
            auto& v = voices[(size_t) i];
            if (! v.isSounding()) { v.setPitch (moria, 0); v.setVowel (curVowel, 0.001f); }
            else v.setPitch (moria, 0.3f);
            v.setLevel (i < n ? 0.9f : 0.0f, 0.3f);
            v.vibrato (1.0f, 10.0f);
        }
    }
    void stop (float t = 0.4f) { holding = false; for (auto& v : voices) v.setLevel (0, t); }
    void silence() { holding = false; for (auto& v : voices) v.silence(); }
    bool sounding() const { for (auto& v : voices) if (v.isSounding()) return true; return false; }

    void render (float* L, float* R, int count, float gain)
    {
        static const float pan[] = { 0.0f, -0.45f, 0.45f, -0.15f };
        // staggered breathing
        if (holding && n >= 2)
            for (int i = 0; i < n; ++i)
            {
                auto& b = breathAt[(size_t) i];
                b -= count;
                auto& v = voices[(size_t) i];
                if (b <= 0 && b > -count) { v.setLevel (0.0f, 0.12f); v.breathIn(); }
                if (b <= -(int) (sr * 0.55))
                {
                    v.setPitch (target, 0.0f, 1.5f);
                    v.setLevel (0.9f, 0.18f);
                    b = (int) (sr * (7.0 + 5.0 * dist (rng)));
                }
            }
        for (int i = 0; i < kMax; ++i)
        {
            auto& v = voices[(size_t) i];
            if (! v.isSounding()) continue;
            if (count > (int) tmp.size()) continue;
            std::fill (tmp.begin(), tmp.begin() + count, 0.0f);
            v.render (tmp.data(), count);
            const float g = gain / std::sqrt ((float) n);
            const float l = g * std::cos ((pan[i] + 1) * juce::MathConstants<float>::pi / 4), r = g * std::sin ((pan[i] + 1) * juce::MathConstants<float>::pi / 4);
            for (int s = 0; s < count; ++s) { L[s] += tmp[(size_t) s] * l; R[s] += tmp[(size_t) s] * r; }
        }
    }
    void reserve (int maxBlock) { tmp.assign ((size_t) maxBlock, 0.0f); }
private:
    std::array<ChantVoice, kMax> voices;
    std::array<int, kMax> breathAt {};
    std::vector<float> tmp;
    std::mt19937 rng { 99 };
    std::uniform_real_distribution<float> dist { 0.0f, 1.0f };
    double sr = 48000;
    float target = 0;
    bool holding = false;
    int n = 2, curVowel = 3;
};

// ---------------------------------------------------------------------------------------
// Church acoustics: predelay, early reflections from stone surfaces, and an 8-line
// feedback delay network with frequency-dependent decay (high frequencies die sooner).
class ChurchReverb
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        static const float ms[kLines] = { 59.3f, 67.1f, 73.9f, 83.3f, 91.7f, 101.9f, 109.3f, 121.1f };
        for (int i = 0; i < kLines; ++i)
        {
            len[(size_t) i] = (int) (ms[i] * 0.001 * sr) | 1;
            line[(size_t) i].assign ((size_t) len[(size_t) i], 0.0f);
            pos[(size_t) i] = 0; damp[(size_t) i] = 0;
        }
        pre.assign ((size_t) (0.12 * sr), 0.0f); prePos = 0;
        setRT (3.2f);
    }
    void setRT (float rt60)
    {
        if (std::abs (rt60 - rt) < 0.01f) return;
        rt = rt60;
        for (int i = 0; i < kLines; ++i)
            gain[(size_t) i] = (float) std::pow (10.0, -3.0 * len[(size_t) i] / (rt60 * sr));
    }
    void reset() { for (auto& l : line) std::fill (l.begin(), l.end(), 0.0f); std::fill (pre.begin(), pre.end(), 0.0f); inLP = 0; }

    void process (float* L, float* R, int n, float wet, float dry)
    {
        static const float erMs[6] = { 11.0f, 19.0f, 27.0f, 37.0f, 49.0f, 63.0f };
        static const float erG[6] = { 0.42f, 0.36f, 0.31f, 0.27f, 0.22f, 0.18f };
        const int preLen = (int) pre.size();
        const int preD = (int) (0.022 * sr);
        for (int s = 0; s < n; ++s)
        {
            const float in = 0.5f * (L[s] + R[s]);
            inLP += (in - inLP) * 0.35f;            // air absorption
            pre[(size_t) prePos] = inLP;
            auto tap = [&] (int d) { int p = prePos - d; while (p < 0) p += preLen; return pre[(size_t) p]; };
            const float x = tap (preD);
            float erL = 0, erR = 0;
            for (int k = 0; k < 6; ++k)
            {
                const float v = tap (std::min (preLen - 1, (int) (erMs[k] * 0.001 * sr))) * erG[k];
                if (k % 2) erR += v; else erL += v;
            }
            prePos = (prePos + 1) % preLen;

            float o[kLines], sum = 0;
            for (int i = 0; i < kLines; ++i) { o[i] = line[(size_t) i][(size_t) pos[(size_t) i]]; sum += o[i]; }
            const float h = sum * (2.0f / kLines);   // Householder mixing
            float outL = 0, outR = 0;
            for (int i = 0; i < kLines; ++i)
            {
                float v = (o[i] - h) * gain[(size_t) i];
                damp[(size_t) i] += (v - damp[(size_t) i]) * 0.42f;   // high frequencies decay faster
                v = damp[(size_t) i];
                line[(size_t) i][(size_t) pos[(size_t) i]] = v + x * ((i & 1) ? -0.35f : 0.35f);
                if (++pos[(size_t) i] >= len[(size_t) i]) pos[(size_t) i] = 0;
                if (i < 4) outL += o[i] * ((i & 1) ? -1.0f : 1.0f); else outR += o[i] * ((i & 1) ? 1.0f : -1.0f);
            }
            L[s] = L[s] * dry + (outL * 0.45f + erL * 0.6f) * wet;
            R[s] = R[s] * dry + (outR * 0.45f + erR * 0.6f) * wet;
        }
    }
private:
    static constexpr int kLines = 8;
    double sr = 48000;
    std::array<std::vector<float>, kLines> line;
    std::array<int, kLines> len {}, pos {};
    std::array<float, kLines> gain {}, damp {};
    std::vector<float> pre;
    int prePos = 0;
    float rt = 0, inLP = 0;
};
} // namespace byz
