# Psaltikon

A Byzantine chant workspace as an **Audio Unit, VST3 and standalone app**. It combines a neumatic notation editor, the eight modes with their microtonal scales, an ison generator, parallage training, melodic formulas and composition tools, and a chanting voice. The theory follows the Constantinopolitan / Greek Orthodox tradition.

## Getting it

**From GitHub:** push to `main` (or open *Actions › Build Psaltikon (AU + VST3) › Run workflow*), then download the artifacts:

- `Psaltikon-macOS`: `Psaltikon-AU.zip`, `Psaltikon-VST3-macOS.zip`, `Psaltikon-App-macOS.zip`, plus a sample render (WAV) and score (PDF) produced by the tests.
- `Psaltikon-Windows`: `Psaltikon-VST3-Windows.zip`, `Psaltikon-App-Windows.zip`.

Install on a Mac by unzipping `Psaltikon.component` into `~/Library/Audio/Plug-Ins/Components/` and `Psaltikon.vst3` into `~/Library/Audio/Plug-Ins/VST3/`. Then restart the DAW. In Logic it appears under *Instrument › AU Instruments › Psaltikon*.

**Local build on a Mac:** `./build.sh` (needs Xcode, CMake and git). It builds all three formats, installs them, and runs `auval`.

## What's in it

### The eight modes (Ἦχος tab)
- All eight modes, plus their forms where the tradition has more than one: First (heirmologic / sticheraric / papadic), Second (heirmologic / sticheraric), Fourth (Legetos on Vou, Agia on Di), Varys (diatonic on low Zo, enharmonic on Ga), Plagal Fourth (heirmologic / sticheraric-papadic).
- Intervals use the 72-moria octave of the 1881 Patriarchal Music Committee:
  - diatonic: 12-10-8-12-12-10-8
  - soft chromatic: 8-14-8
  - hard chromatic: 6-20-4
  - enharmonic: 12-12-6
- A scale ladder drawn to scale in moria. It marks the base, the dominants and the ison notes, gives intervals and cents, and you can click any rung to hear the note.
- For each mode: base note, dominant notes, ison notes, apechema (intonation), character notes, and openings and cadences you can play or insert.
- Melodic attraction: Zo is drawn down toward Ke when the melody turns below it. You can switch this off.

### Composition editor (Σύνθεσις tab)
- Quantity neumes from −7 to +7 steps: ison, oligon, petasti, kentimata, kentima, ypsili, apostrophos, elafron, chamili, and their combinations.
- Time signs: gorgon, digorgon, argon, apli, dipli, tripli, klasma, and the leimma (rest).
- Expression signs (hypostases): vareia, psifiston, omalon, antikenoma, eteron. The voice performs these as accents, oscillation, approach from below, or a bound glide.
- Fthoras that change genus, ὕφεσις and δίεσις, ison indications, and martyriai (the start martyria is drawn automatically).
- Text underlay with melisma lines, and an optional parallage line under every note.
- Automatic spacing and line wrapping, undo and redo, and keyboard entry:
  - `0`–`7` ascend, `Shift`+`0`–`7` descend
  - `G` gorgon, `D` digorgon, `A` apli, `K` klasma, `M` martyria, `P` rest
  - `Space` plays, `Enter` edits the syllable
- Save and open (`.psaltikon`), export a PDF of A4 pages, and export WAV (melody + ison, melody only, or ison only).

### Melos (Μέλος tab)
- A library of formulas for each mode, organised by function: openings, ascents, descents, medial cadences, final cadences, melismatic turns. Each can be played or inserted.
- Set a text to a melody in the selected mode, in a syllabic, neumatic or melismatic style. It uses medial cadences at commas and the final cadence at the end, and generates the ison.
- Develop a selected passage without leaving the mode: step up or down, ornament, invert, retrograde, make melismatic, simplify, or end with the final cadence.

### Parallage (Παραλλαγή tab)
- The parallage syllables (Νη Πα Βου Γα Δι Κε Ζω) shown beside the theoretical names (the Chrysanthine letters α–η), with moria positions. You can click any degree to hear it sung on its syllable.
- Ascending and descending modal sequences.
- Generated exercises (scale, stepwise, leaps, cadential formulas) that you can play or send to the score with their syllables as text.
- Interval recognition quiz.
- Microtonal comparison of any degree across genera (for example, diatonic Vou against hard-chromatic Vou), and the three diatonic tones of 12, 10 and 8 moria.

### Voice & Ison (Ἴσον tab)
- Five voices: deep resonant bass, warm baritone, clear tenor, high-register psaltis, and a restrained ecclesiastical style. Controls for register, resonance, intensity, vibrato depth and speed, articulation, breath, and church reverb.
- Ison modes:
  - **Follow score:** the ison indications in the score. **Generate ison** places them the way chanters move the ison with the melody's region and return to the base at cadences.
  - **Manual drone:** any note you choose.
  - **Live:** MIDI keys below C2 set the ison.
- 1–4 ison voices, ison octave, vowel (a, e, i, o, u or a closed hum), and separate ison and melody volumes.
- **MIDI:** white keys are the degrees of the current mode (middle C = Ni), in its microtonal tuning. Black keys raise the lower note by a diesis.
- **Host sync:** with *Follow host transport* on, the score plays from the DAW's position when you press play in the DAW.

## About the voice

The voice is a formant-based vocal model written for this plugin. It has:
- a band-limited glottal source with spectral tilt, jitter and shimmer
- five vowel formants plus the singer's formant and a nasal resonance
- consonant onsets taken from the text (plosive bursts, fricative noise, nasal dips)
- breaths at rests, delayed vibrato, phrase shaping, and pitch glides measured in moria

It is far more vocal than an oscillator, but it is **not yet** the sound of a recorded chanter. The architecture is built for a hybrid engine: the `Performer` / `ChantVoice` interface takes the same timeline controls (pitch in moria, vowel, articulation, level), so a sample-based or neural singing back end can be dropped in behind it.

## Notes on the theory
- Formulas in the library are idiomatic shapes compiled for practice and composition. They are not transcriptions of particular hymns.
- Neume shapes are drawn as vector paths approximating the printed Chrysanthine forms. Placement of the composite ascending signs is simplified.
- Interval values differ between schools (Chrysanthos' 68 moria, Karas); this build uses the Committee's 72.

## Layout
```
Source/ByzTheory.h       genera, modes, formulas, note names, exercises (no JUCE dependency)
Source/Composition.h     score model, rhythm, timeline, auto-ison, melody generation, JSON
Source/VocalEngine.h     glottal tables, formant voice, ison choir
Source/Performer.h       real-time and offline performance with reverb
Source/ScoreRenderer.h   neume engraving and layout
Source/Exporters.h       WAV render, PDF writer
Source/PluginProcessor.* parameters, MIDI, host sync, state
Source/PluginEditor.*    the five workspaces
Tests/RenderTest.cpp     offline checks (cmake -DPSALTIKON_TESTS=ON)
```
