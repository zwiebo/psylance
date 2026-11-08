# GNARL — Known issues

Updated per milestone. "Automated" = covered by CI tests; "manual" = must be
checked in FL Studio via `docs/FL_STUDIO_TESTING.md`.

## Current milestone (M0–M2 build/QA hardening)

- [ ] **FL Studio host behavior not yet verified.** CI proves the artifact is
  a well-formed VST3 (structure, dependencies, Steinberg validator) and the
  DSP/sequencer pass unit tests — it does **not** prove the plugin loads or
  sounds right in FL Studio. Run the manual checklist before claiming
  host-tested. (manual)
- [ ] **Project save/reload of plugin state** is implemented via APVTS
  `copyState`/`replaceState` but only verified manually in the host.
  (manual)
- [ ] **MIDI export** produces format-0 SMF verified by unit tests; importing
  the `.mid` back into FL Studio is only verified manually. (manual)
- [ ] **Live VST3 MIDI output** is not implemented yet (`producesMidi()` is
  `false`); M4 will add it. Use MIDI file export until then.
- [ ] **Mono voice only** — no polyphony; by design for the darkpsy lead, but
  rapid overlapping notes in the piano roll will cut each other.
- [ ] Plugin UI is functional but plain (no custom graphics) — out of scope
  for MVP.
- [ ] The Linux CI job cannot build the full plugin (no Windows toolchain);
  plugin compilation is covered by the Windows job only.
