# GNARL — Learning Log

Audience: project owner, first-time plugin developer.

## Entry 1 — What a VST3 plugin actually is (M0/M1)

1. **What we built**: GNARL, a VST3 plugin — a small program that FL Studio
   loads into its mixer channel rack. FL Studio sends it MIDI notes and an
   audio stream; the plugin fills the audio stream with synthesized sound.
2. **How it works**: the DAW calls `processBlock()` on the plugin roughly
   every few milliseconds with a buffer of samples plus incoming MIDI events.
   The plugin writes its output into that buffer. A separate `AudioProcessorEditor`
   draws the knobs and sliders.
3. **Why this way**: VST3 is the standard plugin format FL Studio supports;
   JUCE wraps the hundreds of lines of VST3 boilerplate so we only write
   DSP and UI code. CMake drives the build so the same project builds locally
   and in GitHub Actions.
4. **How to test in FL Studio**: copy `GNARL.vst3` to
   `C:\Program Files\Common Files\VST3\`, restart FL Studio, add GNARL to a
   channel, open the piano roll and play notes.
5. **One important concept**: the *audio thread* must never do slow things
   (memory allocation, disk I/O, locks). All knob values reach the audio
   thread as plain atomic numbers, so reading them is instant and safe.
6. **What comes next**: the pattern generator (M3) and MIDI export (M4).

## Entry 2 — FM synthesis in one paragraph

FM (frequency modulation) makes a *carrier* oscillator's pitch wobble using a
second *modulator* oscillator. The amount of wobble is the *modulation index*;
more index = more harmonics = gnarlier. A short index envelope is the secret
of the classic darkpsy "bubble/squelch" — the brightness punches at note
start and decays. Two operators are enough for that whole family of sounds.
