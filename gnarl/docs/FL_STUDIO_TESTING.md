# FL Studio manual test checklist

Automated CI builds the `.vst3` and runs DSP unit tests, but host behavior
must be verified manually in FL Studio on Windows. Do not mark a milestone
"verified in host" until these pass.

## M0/M1 — Load and first sound
- [ ] `GNARL.vst3` copied to `C:\Program Files\Common Files\VST3\`
- [ ] FL Studio plugin manager finds GNARL after rescan
- [ ] GNARL loads on a channel without crashing FL Studio
- [ ] Piano-roll notes produce audible FM tone
- [ ] No stuck notes when deleting notes / stopping transport
- [ ] Plugin UI opens and all sliders move the sound
- [ ] Preset combo switches presets; Mutate changes the sound

## State
- [ ] Change knobs, save project, close, reopen → settings persist

## Robustness (spot checks)
- [ ] Play at 44.1 kHz, 48 kHz and 96 kHz project sample rates
- [ ] Set buffer size to 64, 256 and 1024 (F10 → Audio → Buffer length) — no clicks/glitches
- [ ] Start/stop transport repeatedly — no stuck notes
- [ ] Jump the playhead while playing — no stuck notes or crashes

## M4 (later) — MIDI output
- [ ] Live VST3 MIDI out routed to a second instrument (test with two GNARL
      instances or any VSTi): notes arrive at the receiver
- [ ] If FL Studio does not route it, record the exact wrapper/Patcher setup
      and keep MIDI-file export as the reliable workflow
