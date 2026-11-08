# GNARL — Technical Decisions

## Pinned dependencies

| Dependency | Version | How pinned |
|---|---|---|
| JUCE | 8.0.6 (git tag) | `FetchContent` with `GIT_TAG 8.0.6` in root `CMakeLists.txt` |
| CMake | >= 3.22 | `cmake_minimum_required` |
| Compiler | MSVC (VS 2022) on Windows; any C++17 compiler for DSP tests | CI: `windows-latest` |

JUCE 8.0.6 requires C++17, matching the project constraint.

## JUCE licensing obligations

JUCE is dual-licensed: GPLv3 (free) or commercial. This project uses the
GPLv3 route. Consequences:

- If GNARL (the combined work) is ever distributed, it must be distributed
  under GPLv3 with source code available and the JUCE license notice included.
- Personal, non-distributed use has no obligations.
- If the owner later wants to sell GNARL closed-source, a JUCE commercial
  license is required instead. This is a business decision, not a code one.

## Architecture decisions

- **DSP is pure C++17, no JUCE types.** This lets DSP unit tests compile and
  run anywhere (including this Linux sandbox and Windows CI) without GUI
  dependencies, and keeps the audio hot path free of framework overhead.
- **APVTS as the single authoritative parameter store.** GUI, host automation,
  presets and mutation all go through `AudioProcessorValueTreeState`; the
  audio thread reads atomic snapshots once per buffer.
- **Mono voice with portamento** for the lead character; darkpsy leads are
  monophonic. No hanging notes: every note-off and every transport stop
  triggers a release, and note-offs are guaranteed by construction.
- **PolyBLEP oscillators** to suppress aliasing at low CPU cost; a naive
  `sin(phase)` aliases badly at high notes.
- **TPT state-variable filter** (Zavalishin-style) — unconditionally stable,
  cheap, multimode with low-pass priority.
- **Deterministic pattern generation**: pure function of (seed, settings).
- **Verification strategy**: DSP correctness is proven by automated tests on
  Linux and Windows CI. FL Studio host behavior (loading, live MIDI routing)
  cannot be automated in this environment and is covered by a documented
  manual checklist (`docs/FL_STUDIO_TEST_CHECKLIST.md`). No host-specific
  behavior is claimed as verified until tested by the owner.
