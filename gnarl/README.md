# GNARL

Windows VST3 synthesizer plugin for FL Studio, specialized for darkpsy /
forest psytrance: a two-operator FM engine plus a DAW-tempo-synced 16-step
pattern generator.

## Quick start

- **Build (Windows):** `docs/BUILD.md`
- **Install in FL Studio:** `docs/INSTALL.md`
- **Validation (automated + manual):** `docs/VALIDATION.md`, `docs/FL_STUDIO_TESTING.md`
- **Licensing / redistributed dependencies:** `docs/LICENSING.md`
- **Known issues:** `docs/KNOWN_ISSUES.md`
- **Roadmap:** `docs/ROADMAP.md`

CI (`.github/workflows/windows-build.yml`) builds the VST3 on a Windows
runner, runs unit tests, verifies the artifact (structure, runtime
dependencies, Steinberg VST3 validator) and uploads the
`GNARL-vst3-windows-x64` artifact.
