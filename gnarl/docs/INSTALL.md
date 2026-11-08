# Installing GNARL in FL Studio

## Prerequisites

- Windows 10/11 x64
- FL Studio (any edition that supports VST3)
- [Visual C++ Redistributable x64](https://aka.ms/vs/17/release/vc_redist.x64.exe)
  — required because the plugin is built with MSVC. CI verifies the plugin
  imports system DLLs only (`tools/check_dll_dependencies.ps1`).

## Get the artifact

Download the `GNARL-vst3-windows-x64` artifact from the latest successful
run of the **Build GNARL VST3 (Windows)** GitHub Action on this branch
(artifacts are kept for 30 days), or build locally on Windows — see
`docs/BUILD.md` for the reproducible build:

```
cd gnarl
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

The plugin bundle is `build/GNARL_artefacts/Release/GNARL.vst3`
(search for `*.vst3` under `build/` if the JUCE version moves it).

## Verify the artifact (optional, Windows)

```
python tools/verify_vst3_bundle.py path\to\GNARL.vst3
pwsh tools/check_dll_dependencies.ps1 -DllPath path\to\GNARL.vst3\Contents\x86_64-win\GNARL.dll
```

## Install

1. Copy the **whole `GNARL.vst3` bundle** (it is a directory, not a single
   file) to `C:\Program Files\Common Files\VST3\` (create the folder if
   needed). Never copy the inner DLL out of the bundle.
2. Restart FL Studio, or use Options → Plugin Manager → "Refresh plugin
   list" with "Rescan previously scanned plugins" enabled.
3. Add GNARL: Channel Rack → right-click an empty slot → it appears under
   `Installed → Generators` (VST3 synths appear as instruments).

## First sound

1. Click the GNARL channel to open its wrapper.
2. Open the piano roll (F7) and draw a few notes, or connect a MIDI keyboard.
3. Press Play. You should hear a two-operator FM tone.
4. Open the plugin UI (F9) and try the presets and the **Mutate Sound** button.

## State saving

Knob positions are stored in the FL Studio project automatically (VST3
parameter state). Save the project and reopen it — settings persist.

## After installing

Run the manual host checklist in `docs/FL_STUDIO_TESTING.md` before calling a
build "FL Studio-tested". See `docs/KNOWN_ISSUES.md` for what is not yet
verified.
