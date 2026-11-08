# GNARL — Sequencer module (M3/M4)

`src/sequencer/` owns the Pattern, Scheduler and MIDI export. It is pure C++17
with no JUCE dependency, so it is unit-tested standalone (`tests/sequencer_tests.cpp`).

## Beginner-friendly concepts

**Steps.** A pattern is 16 steps — one 4/4 bar chopped into 16th notes. Step
*i* starts at beat `i * 0.25` (a 16th note is a quarter note divided by 4).
Each step is either a rest or a note with velocity, accent, glide and lock
flags.

**Musical scales.** A MIDI note number is a semitone index (A4 = 69; +1 = one
semitone up). A scale is just a list of semitone offsets from a root note.
The generator only picks notes of the form `root + 12*octave + scaleOffset`,
clamped to the configured pitch range, so every generated note is guaranteed
to be in key. Darkpsy defaults to Phrygian Dominant `{0,1,4,5,7,8,10}`.

**MIDI.** A MIDI channel message is 3 bytes: status byte (`0x90` = note-on,
`0x80` = note-off), note number, velocity (note-on with velocity 0 also means
note-off). A Standard MIDI File stores events as delta times in ticks
(PPQ = ticks per quarter note; at 480 PPQ a 16th note = 120 ticks) plus meta
events: tempo (µs per quarter note), time signature, end-of-track.

**Note scheduling.** The scheduler converts beats to samples with
`samplesPerBeat = sampleRate * 60 / bpm` and keeps an anchor
(sample ↔ beat ↔ tempo). Position is integrated piecewise from the anchor, so
tempo changes and seeks stay sample-accurate. For every audio buffer it emits
the note-ons whose onsets fall inside the buffer *and* the note-offs of notes
started in earlier buffers that fall inside it (active-note tracking), which
keeps events correct across buffer boundaries. Stop and transport jumps cut
all sounding notes at the current sample; loop points cut notes at the loop
end. Note-offs are therefore guaranteed by construction — no hanging notes.

## Glide contract (shared with the DSP Engineer)

A step's `glide` flag means **legato**: if the next step is active it is not
re-articulated — the note sustains through the whole glide run and its
note-off moves to the end of the run. The covered step's pitch is kept as the
**portamento target** for the synth (the DSP voice is mono with glide), but
in Standard MIDI File export the run is a single merged note with no pitch
bend. Internal glide and SMF export are intentionally not identical.

## Determinism

Generation uses a self-contained SplitMix64 PRNG (portable, unlike
`std::mt19937` distributions). Each step derives its stream from
`hash(seed, stepIndex)`, so the same (seed, settings, locks) always produces
the identical pattern across reruns and platforms, and locking one step never
changes the others. MIDI file bytes are a pure function of the pattern.

## Live VST3 MIDI output (M4)

The plugin calls `StepScheduler::process(bufferStart, numSamples)` once per
audio block and forwards the returned `ScheduledEvent`s through a
`MidiOutputSink`, using each event's sample offset to place it inside the
block's `juce::MidiBuffer`. Host-side routing in FL Studio is covered by the
manual checklist in `docs/FL_STUDIO_TESTING.md`.
