# Project specification

## Supported configuration

The initial target is Windows 11 x64, an Xbox-compatible controller and the USA/Europe retail disc identified in the [inventory](inventory.md). Start with the SDK's D3D12 backend and original offline game logic. Record the game's actual render resolution, simulation cadence, controller settings and difficulty on the reference system before setting numerical fidelity targets.

The first release target covers the offline campaign, local profiles and saves. Additional disc revisions, other operating systems, online multiplayer, achievements integration, HOTAS support and higher frame rates are future scope. The four catalog languages are test cases for the supported disc. Repository text, diagnostics, documentation and release notes are in English.

## Implementation boundaries

1. **Build input.** Accept the player's local disc image for extraction, or an already extracted game directory. Validate the XEX size and SHA-256 before translation. A different hash must produce a diagnostic identifying the unsupported revision. Record the full ISO hash when a raw image is supplied.
2. **Translation.** Use ReXGlue v0.10.1, commit `bd833a2ab69d305f99ddf57418fc38ffec8af93d`. Pin both the CLI and the runtime to that release. Maintain a TOML manifest for `default.xex`, with version-specific functions and jump-table hints added when supported by evidence. Regenerate the C++ locally. [SDK release](https://github.com/rexglue/rexglue-sdk/releases/tag/v0.10.1) and [manifest template](https://github.com/rexglue/rexglue-sdk/blob/bd833a2ab69d305f99ddf57418fc38ffec8af93d/resources/templates/init/manifest_toml.inja)
3. **Runtime.** Use the SDK application, VFS, kernel services, graphics, input and audio as the starting implementation. Point `--game_data_root` at the extracted directory containing `default.xex`. Put profile and save writes in a separate user directory. Preserve the original disc layout, including the case of filenames. [Startup and VFS mapping](https://github.com/rexglue/rexglue-sdk/blob/bd833a2ab69d305f99ddf57418fc38ffec8af93d/src/ui/rex_app.cpp#L201-L281)
4. **Title-specific fixes.** For a reproducible failure, record the scene, XEX hash, guest address, observed behavior and expected result. Use manifest hints for missing code targets. Put host API adaptations in handwritten C++. Use a narrow SDK patch only when the failing behavior can be traced into the pinned SDK. Repeat the triggering scene after each fix.
5. **Reproducibility.** Build C++23 with Clang 18 or newer, CMake 3.25 or newer and Ninja in an x64 Windows developer environment. Record the working compiler and linker versions in the first successful build. A second checkout using the same inputs must reach the same execution checkpoint. [SDK build settings](https://github.com/rexglue/rexglue-sdk/blob/bd833a2ab69d305f99ddf57418fc38ffec8af93d/CMakeLists.txt) and [Windows presets](https://github.com/rexglue/rexglue-sdk/blob/bd833a2ab69d305f99ddf57418fc38ffec8af93d/resources/templates/init/cmake_presets.inja)

## Acceptance tests

| Area | Test | Passing evidence |
| --- | --- | --- |
| Boot and menu | Cold-start twice from a fresh profile. Let the introduction finish on one run and skip it on the other. | Each path reaches a responsive title menu. The full playback run has visible video and audible, synchronized sound. |
| Controls | Use the same controller configuration and aircraft on PC and reference hardware. Exercise pitch, roll, yaw, throttle, camera, pause and menu navigation. | Correct axis direction, usable neutral position, and no persistent input after pause or disconnect and reconnect. |
| Mission and timing | Record one named mission, aircraft, difficulty and repeatable input sequence. Capture the mission clock and comparable flight and radio events. | The same events occur in order. Compare elapsed game time with the observed variation across repeated reference runs. Record frame times and any drift. |
| Rendering | Compare the menu, cockpit instruments, outside view, horizon and HUD at matched mission checkpoints and the observed reference resolution. | Instruments and text are legible. Terrain, aircraft and HUD remain visible in motion. Repeat at 1440p and track scaling defects separately. |
| Audio and loading | Observe archive reads during mission loading and listen to engine loops, dialogue, music and effects through a pause and mission transition. | Requested assets load. Applicable cues begin, loop and end at their reference events. No repeatable dropout or stall. |
| Save and progress | Complete the mission, return to the menu, exit the process, relaunch and load the same profile. | Mission completion and unlocked progress survive the restart. The next available mission loads. |
| Languages and campaign | Check the UI, subtitles and voice selection in each language exposed on the reference disc. Play every required offline campaign mission through the ending. | A coverage table records build, language, mission, result and remaining defects. The first full campaign release requires a complete passing route. |

## Evidence and distribution

Use an Xbox 360 running the reference edition for fidelity measurements when available. Record disc revision, scene, controller settings and capture conditions. A pinned Xenia Canary run can supply a provisional comparison. Label measurements by source and keep console parity open until a console comparison is available.

Public CI can validate manifests, host helpers, input rejection and packaging with synthetic fixtures. Full-disc builds and playthroughs run locally. Publish original sources, instructions, version pins and verified fingerprints. Keep extracted assets, embedded title icons, generated game C++, local executables, dumps and player saves outside public artifacts. Review the tracked file list and each release archive. Choose a license for original project code before inviting contributions, and retain notices required by included dependencies. [ReXGlue license](https://github.com/rexglue/rexglue-sdk/blob/bd833a2ab69d305f99ddf57418fc38ffec8af93d/LICENSE)
