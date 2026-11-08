# GNARL — Technical Decisions

## Pinned dependencies

| Dependency | Version | How pinned |
|---|---|---|
| JUCE | 8.0.6 (git tag) | `FetchContent` with `GIT_TAG 8.0.6` in `gnarl/CMakeLists.txt`; CI caches it under `FETCHCONTENT_BASE_DIR` keyed by the tag |
| VST3 SDK (validator only, never shipped in the artifact) | v3.8.1_build_84 (git tag, MIT) | CI env `VST3SDK_TAG` in `.github/workflows/windows-build.yml` |
| CMake | >= 3.22 | `cmake_minimum_required` |
| Compiler | MSVC (VS 2022) on Windows; any C++17 compiler for DSP/sequencer tests | CI: `windows-latest` / `ubuntu-latest` |

JUCE 8.0.6 requires C++17, matching the project constraint. See
`docs/BUILD.md` for reproducible build instructions and `docs/LICENSING.md`
for license obligations of each pin.

## JUCE licensing obligations

JUCE is dual-licensed: GPLv3 (free) or commercial. This project uses the
GPLv3 route. Consequences:

- If GNARL (the combined work) is ever distributed, it must be distributed
  under GPLv3 with source code available and the JUCE license notice included.
- Personal, non-distributed use has no obligations.
- If the owner later wants to sell GNARL closed-source, a JUCE commercial
  license is required instead. This is a business decision, not a code one.

Full dependency/license table: `docs/LICENSING.md`.

## VST3 validation decision

Spec conformance is checked in CI with Steinberg's official command-line
`validator`, built from the pinned MIT-licensed VST3 SDK
(`steinbergmedia/vst3sdk` @ `v3.8.1_build_84`), plus a fast structural check
(`tools/verify_vst3_bundle.py`) and a runtime-dependency check
(`tools/check_dll_dependencies.ps1`). Details and the explicit
"CI green ≠ FL Studio-tested" rule: `docs/VALIDATION.md`.

## Architecture decisions

- **DSP is pure C++17, no JUCE types.** This lets DSP unit tests compile and
  run anywhere (including this Linux sandbox and Windows CI) without GUI
  dependencies, and keeps the audio hot path free of framework overhead.
  The tests-only build (`-DGNARL_BUILD_PLUGIN=OFF`) skips the JUCE fetch
  entirely, so the Linux CI job stays fast.
- **APVTS as the single authoritative parameter store.** GUI, host automation,
  presets and mutation all go through `AudioProcessorValueTreeState`; the
  audio thread reads atomic snapshots once per buffer.
- **Mono voice with portamento** for the lead character; darkpsy leads are
  monophonic. No hanging notes: every note-off and every transport stop
  triggers a release, and note-offs are guaranteed by construction.
- **Sine oscillators with phase accumulation** — sampling `sin()` is exact
  for a sine, so there is no aliasing to correct for.
- **TPT state-variable filter** (Zavalishin-style) — unconditionally stable,
  cheap, multimode with low-pass priority.
- **Deterministic pattern generation**: pure function of (seed, settings).
- **Sample-accurate step scheduler**: tempo changes apply at the next step
  boundary; `stop()` always emits a note-off for the sounding note.
- **Verification strategy**: DSP/sequencer correctness and VST3 conformance
  are proven by automated CI (unit tests + Steinberg validator). FL Studio
  host behavior (loading, live MIDI routing, project save/reload) cannot be
  automated in this environment and is covered by a documented manual
  checklist (`docs/FL_STUDIO_TESTING.md`). No host-specific behavior is
  claimed as verified until tested by the owner.
