# FL Studio manual test checklist (host-specific validation)

Automated CI (see `docs/VALIDATION.md`) proves the artifact is a well-formed,
spec-conformant VST3 and that the DSP/sequencer logic is correct. **It does
not prove the plugin works in FL Studio.** Do not mark a milestone
"FL Studio-tested" until every applicable box below is checked on a real
Windows machine with FL Studio installed.

Tested by: ____________  Date: ________  FL Studio version: ________
Artifact: `GNARL-vst3-windows-x64` from CI run #________

## Setup
- [ ] Install the VC++ Redistributable if missing (see `docs/INSTALL.md`)
- [ ] Copy the whole `GNARL.vst3` directory to `C:\Program Files\Common Files\VST3\`
- [ ] Rescan plugins in FL Studio (Options → Plugin Manager → Refresh plugin list)
- [ ] Verify plugin metadata: GNARL appears as a **VST3 instrument** from
      vendor "zwiebo" (check the plugin manager's info panel)

## MIDI note-on / note-off
- [ ] Add GNARL to a channel, open the piano roll, draw notes → audible FM tone
- [ ] Note-off releases the sound (no drone after key release)
- [ ] MIDI keyboard input works (note-on/note-off, velocity affects loudness)

## Silence on idle
- [ ] With transport stopped and no notes playing, the channel is silent
      (no hum, noise floor, or residual tail)

## No stuck notes
- [ ] Delete a playing note from the piano roll → sound stops
- [ ] Press Stop during playback → all sound stops
- [ ] Rapid start/stop of transport 10× → no stuck notes
- [ ] Jump the playhead while playing → no stuck notes or crash

## Audio sample safety
- [ ] Project sample rate 44.1 kHz → clean, no clicks/glitches
- [ ] Project sample rate 48 kHz → clean
- [ ] Project sample rate 96 kHz → clean
- [ ] Buffer length 64 → no glitches
- [ ] Buffer length 256 → no glitches
- [ ] Buffer length 1024 → no glitches
- [ ] Output never clips harshly at full velocity + high drive (bounded waveshaper)

## Host tempo changes
- [ ] Pattern/step timing follows the DAW tempo (change project BPM 120 → 150
      while playing: timing tightens at the next step, no crash)
      — applies from M3 when internal pattern playback exists

## Transport stop/start
- [ ] Start → notes play; Stop → silence; Start again → notes play again
- [ ] No stuck notes across repeated transport cycles

## Project save/reload
- [ ] Change several knobs, save the FL Studio project (.flp), close, reopen
      → all settings persist exactly
- [ ] Load the project on a fresh channel → same sound

## MIDI export validity (M4)
- [ ] Export a pattern to `.mid`, then drag the `.mid` back into FL Studio's
      piano roll → notes land on the expected steps at the right pitches
- [ ] The `.mid` opens in another tool (e.g. Windows Media Player or an
      online SMF checker) without errors

## Plugin UI / presets
- [ ] Plugin UI opens (F9), all 11 sliders move the sound
- [ ] Preset combo switches presets; **Mutate Sound** audibly changes the sound
- [ ] Closing and reopening the UI keeps current settings

## Sign-off
- [ ] All applicable items pass → milestone may be called "FL Studio-tested"
- [ ] Any failure: record exact repro steps in `docs/KNOWN_ISSUES.md` and do
      **not** claim host-tested
