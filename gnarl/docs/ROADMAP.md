# GNARL — Roadmap

Every milestone ships: a downloadable Windows artifact
(`GNARL-vst3-windows-x64` from the **Build GNARL VST3 (Windows)** action,
see `.github/workflows/windows-build.yml`), reproducible build instructions
(`docs/BUILD.md`), automated validation (`docs/VALIDATION.md`), and a current
list of remaining known issues (`docs/KNOWN_ISSUES.md`).

| Milestone | Goal | Status | Artifact | Known issues |
|---|---|---|---|---|
| M0 | Minimal VST3 compiles on Windows and loads in FL Studio | In progress | `GNARL-vst3-windows-x64` | `docs/KNOWN_ISSUES.md` |
| M1 | Plugin responds to MIDI notes, produces a basic FM tone | In progress | `GNARL-vst3-windows-x64` | `docs/KNOWN_ISSUES.md` |
| M2 | Darkpsy engine: envelopes, filter, distortion, glide, presets, mutate | In progress | `GNARL-vst3-windows-x64` | `docs/KNOWN_ISSUES.md` |
| M3 | Tempo-synced 16-step pattern generator with internal playback | Planned | `GNARL-vst3-windows-x64` | `docs/KNOWN_ISSUES.md` |
| M4 | MIDI file export + live VST3 MIDI output (tested in FL Studio) | Planned | `GNARL-vst3-windows-x64` | `docs/KNOWN_ISSUES.md` |
| M5 | Stable MVP: state save, tests, manual checklist | Planned | `GNARL-vst3-windows-x64` | `docs/KNOWN_ISSUES.md` |

Manual host validation checklist: `docs/FL_STUDIO_TESTING.md`.
Installation: `docs/INSTALL.md`. Licensing: `docs/LICENSING.md`.
