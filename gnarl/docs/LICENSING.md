# GNARL — Licensing obligations and redistributed dependencies

## GNARL's own license route

GNARL is developed against JUCE under the **GPLv3 route** (see
`docs/DECISIONS.md`). Consequences:

- If GNARL (the combined work) is ever **distributed** — including the
  downloadable CI artifacts — it must be distributed under GPLv3, with the
  corresponding source code available and the JUCE license notice included.
- Personal, non-distributed use has no obligations.
- Selling GNARL closed-source would require a JUCE commercial license
  instead — a business decision, not a code one.

## Dependencies and their licenses

| Component | License | Role | Redistributed in the artifact? |
|---|---|---|---|
| JUCE 8.0.6 (pinned git tag) | GPLv3 (or commercial) | Plugin framework, statically linked into `GNARL.dll` | Yes — as part of the binary; triggers the GPLv3 distribution obligations above. Include the [JUCE license notice](https://github.com/juce-framework/JUCE/blob/master/LICENSE.md) with any distribution. |
| VST3 SDK `v3.8.1_build_84` (pinned git tag) | MIT | Build-time only: compiled into Steinberg's `validator` in CI to conformance-test the artifact | No — the validator never ships inside `GNARL.vst3`. MIT requires retaining the copyright/license notice in the validator's own distribution only. |
| Visual C++ runtime (`vcruntime140.dll`, `msvcp140.dll`, `ucrtbase.dll`) | [Microsoft Visual Studio Redistributable license](https://visualstudio.microsoft.com/license-terms/mlt687465/) | Runtime dependency of the MSVC-built DLL | Not bundled — documented as an end-user prerequisite (install the [VC++ Redistributable](https://aka.ms/vs/17/release/vc_redist.x64.exe)). CI verifies the artifact imports system DLLs only (`tools/check_dll_dependencies.ps1`). |
| Steinberg VST trademark | [VST Usage Guidelines](https://www.steinberg.net/vst3sdk) | "VST" is a trademark of Steinberg Media Technologies GmbH | Using the VST3 format is fine; displaying the VST logo requires following Steinberg's usage guidelines. |

## Practical checklist before distributing a build

- [ ] Ship the GPLv3 license text (or a clear offer of source) with any
      distributed GNARL binary.
- [ ] Include the JUCE license notice.
- [ ] Do not bundle the VST3 SDK or the validator in the artifact.
- [ ] Document the VC++ Redistributable prerequisite in the release notes
      (`docs/INSTALL.md`).
- [ ] If the VST logo is shown anywhere, follow Steinberg's VST Usage
      Guidelines.
