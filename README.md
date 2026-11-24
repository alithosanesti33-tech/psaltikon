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

The chanting voice is a physical (source–filter) model written for this plugin; it needs no samples and no network.

- **Glottal source.** Glottal-flow pulses for three voice qualities (breathy, modal and pressed), stored as band-limited tables. Vocal effort crossfades between them, so the voice brightens as it gets louder and higher. A high-shelf models the firm glottal closure of trained singers. Pitch and amplitude vary from cycle to cycle (jitter and shimmer).
- **Vocal tract.** Five cascaded formant resonators plus higher-pole correction. F3–F5 are pulled together into the singer's formant, and the bright "twang" of the psaltic voice adds to that cluster. Nasal pole–zero coupling is used for m and n.
- **Consonants with real articulation.** Stops have a closure, a burst and aspiration. Fricatives (β δ γ ζ voiced; φ θ σ χ voiceless) use band-limited frication, and ξ and ψ are handled as affricates. Nasals, λ and the tapped ρ are modelled too, all with formant transitions into the vowel. Onset clusters such as κρα and πλη are handled. Consonants start **before** the beat, as singers sing them, and words are read in Greek ecclesiastical pronunciation.
- **Pitch.** Glides through a damped second-order system, with a slight overshoot on leaps. Syllables start with a scoop. On top of that come slow drift and a vibrato that is delayed and varies in rate, all on the 72-moria grid.
- **Phrasing and ornament.** Breaths at rests, swells on long notes, softening into cadences, a slow settle into cadences approached from above, gentle lower-neighbour inflections on long held notes, and a falling release at the end. The *Ornamentation* control sets how much of this you hear.
- **Ison.** Up to four chanters, slightly detuned and spread in stereo. Each breathes at a different moment, so the ison never stops.
- **Church acoustic.** Predelay, early reflections from stone, and an 8-line feedback-delay network whose high frequencies decay faster than its lows. The RT60 runs from about 2.4 to 4.8 s.

This is as far as a self-contained synthesis engine goes. It sounds sung, with real consonants and a resonant church tone, but a recorded chanter still has detail no model reproduces. The engine is built so a sample-based or neural singing back end can replace `ChantVoice` without touching the rest: it is driven by the same timeline controls (pitch in moria, vowel, consonant, level).

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
