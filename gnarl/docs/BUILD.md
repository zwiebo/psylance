# GNARL — Reproducible build instructions

Every milestone build is produced the same way, locally and in CI, so any
artifact can be reproduced bit-for-bit from the same sources and pins.

## Pinned inputs

| Input | Pin | Where |
|---|---|---|
| JUCE | git tag `8.0.6` | `FetchContent` in `CMakeLists.txt`; CI env `JUCE_TAG` |
| VST3 SDK (validator only) | git tag `v3.8.1_build_84` | CI env `VST3SDK_TAG` |
| CMake | >= 3.22 | `cmake_minimum_required` |
| Compiler | MSVC (VS 2022) on Windows; any C++17 compiler for tests | CI `windows-latest` / `ubuntu-latest` |
| C++ standard | C++17 | `CMAKE_CXX_STANDARD` |

CI caches the pinned JUCE checkout (`actions/cache` on the
`FETCHCONTENT_BASE_DIR`) and the validator build, keyed by the pin, so
rebuilds are fast and never drift from the pin.

## Windows — full VST3 plugin (local)

Prerequisites: Windows 10/11 x64, Visual Studio 2022 (C++ workload),
CMake >= 3.22, git.

```
cd gnarl
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build --output-on-failure -C Release
```

The plugin bundle is `build/GNARL_artefacts/Release/GNARL.vst3`
(search for `*.vst3` under `build/` if the JUCE version moves it).

## Any platform — unit tests only (no JUCE download)

```
cd gnarl
cmake -B build-tests -DGNARL_BUILD_PLUGIN=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-tests -j
ctest --test-dir build-tests --output-on-failure
```

## CI (GitHub Actions)

`.github/workflows/windows-build.yml` runs three jobs:

1. **Unit tests (Linux)** — tests-only build, no JUCE fetch. Fast feedback.
2. **Windows VST3 build** — configures with the cached pinned JUCE, builds
   Release, runs `ctest` on Windows, verifies the bundle structure/metadata
   (`tools/verify_vst3_bundle.py`) and runtime dependencies
   (`tools/check_dll_dependencies.ps1`), uploads artifact
   `GNARL-vst3-windows-x64`.
3. **VST3 conformance** — builds Steinberg's official `validator` from the
   pinned VST3 SDK and runs it against the uploaded artifact.

## Verifying a downloaded artifact locally (Windows)

```
python tools/verify_vst3_bundle.py path\to\GNARL.vst3 --name GNARL --manufacturer zwiebo
pwsh tools/check_dll_dependencies.ps1 -DllPath path\to\GNARL.vst3\Contents\x86_64-win\GNARL.dll
```

## Artifact layout

```
GNARL.vst3/                       (the uploadable artifact is this whole directory)
  Contents/
    x86_64-win/
      GNARL.dll                   (PE32+ x86-64, exports GetPluginFactory/InitModule/ExitModule)
```

Install the whole `GNARL.vst3` directory — never the inner DLL alone
(see `docs/INSTALL.md`).
