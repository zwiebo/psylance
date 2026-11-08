# Installing the first GNARL build in FL Studio

## Get the artifact

Download the `GNARL-vst3-windows-x64` artifact from the latest successful
run of the **Build GNARL VST3 (Windows)** GitHub Action on this branch,
or build locally on Windows:

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

The plugin bundle is `build/GNARL_artefacts/Release/GNARL.vst3`
(path may vary slightly by JUCE version; search for `*.vst3` under `build/`).

## Install

1. Copy the whole `GNARL.vst3` bundle (it is a directory) to
   `C:\Program Files\Common Files\VST3\`
   (create the folder if needed).
2. Restart FL Studio (or use Options → Plugin Manager → "Rescan" /
   "Refresh plugin list" and enable "Rescan previously scanned plugins").
3. Add GNARL: Channel Rack → right-click empty slot → it appears under
   `Installed → Generators` (VST3 synths appear as instruments).

## First sound (M1)

1. Click the GNARL channel to open its wrapper/piano roll.
2. Open the piano roll (F7 or the channel's piano-roll button) and draw a
   few notes, or connect a MIDI keyboard.
3. Press Play in FL Studio. You should hear a two-operator FM tone.
4. Open the plugin UI (F9 / the green plugin button) and try the presets
   and the **Mutate Sound** button.

## State saving

Knob positions are stored in the FL Studio project automatically (VST3
parameter state). Save the project and reopen it — settings should persist.
