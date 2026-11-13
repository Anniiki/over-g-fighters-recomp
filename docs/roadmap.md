# Roadmap

## Milestones

| Stage | Work | Exit condition | Status |
| --- | --- | --- | --- |
| 0. Disc and project definition | Identify the disc and executable, survey the workstation, compare existing ports and define a first playable milestone. | Record the hashes, repeat the disc inventory with a second reader and document scope and acceptance tests. | Inventory and planning complete |
| 1. Toolchain and private input | Resolve Clang, activate the Visual Studio x64 environment, install the pinned SDK, extract the disc locally and verify the XEX. | The tool versions and SDK checksum are recorded. The extracted XEX matches the inventory. A deliberately incorrect input produces a useful error. | Next |
| 2. Translation and native build | Initialize the host project, generate the guest C++, resolve analysis errors and link a Windows x64 executable. | From a second checkout, reproduce the build and log entry into the guest at `0x8235EBA8`. Record any unresolved imports or indirect calls. | Pending |
| 3. Boot, video and menu | Trace file opens, first GPU submission, movie playback and input. | Reach a responsive menu on cold start. Pass both the full introduction and skip tests. Document any temporary diagnostic bypass separately. | Pending |
| 4. First playable mission | Load a mission from a new profile, verify cockpit, HUD, controller axes, audio and completion, then persist progress. | Another tester repeats the named mission and reloads its progress after restarting the application. | Pending |
| 5. Campaign and fidelity | Play the offline campaign, test the disc languages, compare flight timing and record GPU, audio and save regressions. | Complete the campaign route and the [acceptance matrix](specification.md#acceptance-tests). Record the comparison platform and measured performance. | Pending |
| 6. Public release candidate | Package project-owned sources, provide reproducible local setup, credit dependencies and test the published instructions. | On a clean Windows system, a tester builds from the release and their own disc, completes the smoke mission and resumes the save. The public archive passes its content review. | Pending |

Publish the planning repository separately from the first playable release. Roadmap stages describe evidence. Dates will be estimated after the first native build exposes the remaining translation and runtime work.

## First engineering session

Use the checkout as the current directory and a sibling named `private` for local inputs. The following PowerShell workflow is for stages 1 and 2. Only the disc listing and inventory have been tested with this title. The paths below are examples, and `REXGLUE_PREFIX` should identify the extracted Windows SDK package on the development machine.

1. Open an x64 developer shell. Check that `clang`, `clang++`, `cmake`, `ninja`, `xdvdfs` and `rexglue` resolve to the intended installations. The Windows SDK package is `rexglue-sdk-0.10.1-win-amd64.zip`. Verify its SHA-256 against `26dbcd19e6976a7ad349917bffc58c1a0f21ad43d71a963d16b623c8e70d1f4d`. [SDK release](https://github.com/rexglue/rexglue-sdk/releases/tag/v0.10.1)
2. Set `$iso` to the player's image path and use the following commands. Compare both hashes with the [inventory](inventory.md) before proceeding. An already extracted copy can start at the XEX hash check. [XDVDFS usage](https://github.com/antangelo/xdvdfs/blob/1cc850bf1b3487fad7ec7c9eed01d83e8fc75ba4/README.md#L163-L180)

   ```powershell
   $game = '../private/game'
   Get-FileHash -LiteralPath $iso -Algorithm SHA256
   xdvdfs tree "$iso"
   New-Item -ItemType Directory -Force ../private | Out-Null
   xdvdfs unpack "$iso" "$game"
   Get-FileHash -LiteralPath "$game/default.xex" -Algorithm SHA256
   ```

3. Run `init` once to create the host scaffold. Inspect its manifest and keep its game paths relative. On later checkouts with a tracked manifest, start at `codegen`. It recreates the generated CMake include required by the first configure. [Init options](https://github.com/rexglue/rexglue-sdk/blob/bd833a2ab69d305f99ddf57418fc38ffec8af93d/src/rexglue/commands/init_command.cpp#L562-L580) and [codegen command](https://github.com/rexglue/rexglue-sdk/blob/bd833a2ab69d305f99ddf57418fc38ffec8af93d/src/rexglue/commands/codegen_command.cpp#L185-L217)

   ```powershell
   rexglue init --project-name overg --project-root . --xex-path "$game/default.xex" --game-root "$game"
   rexglue codegen overg_manifest.toml
   cmake --preset win-amd64-relwithdebinfo -DREXSDK_VERSION=0.10.1 "-DCMAKE_PREFIX_PATH=$env:REXGLUE_PREFIX"
   cmake --build --preset win-amd64-relwithdebinfo
   ```

4. Launch the resulting `overg.exe` with `--game_data_root` pointing at the absolute path of the extracted game directory. Capture the guest entry address, first file open and first failing subsystem. For a failed boot, save the earliest reproducible error alongside the XEX hash, SDK commit and build configuration before changing code. [Runtime startup](https://github.com/rexglue/rexglue-sdk/blob/bd833a2ab69d305f99ddf57418fc38ffec8af93d/src/ui/rex_app.cpp#L201-L281)

## Early decisions driven by tests

- If code generation misses an indirect branch target, verify the target against the guest instruction stream and add the smallest reproducible manifest hint. A successful link alone cannot prove that a mission's indirect calls resolve.
- If the opening movie blocks startup, trace file access, decoder selection and the signal that hands control back to the menu. The SDK's bundled FFmpeg codec list contains audio decoders. The title's WMV3 video path needs a runtime test. [SDK codec list](https://github.com/rexglue/rexglue-sdk/blob/bd833a2ab69d305f99ddf57418fc38ffec8af93d/thirdparty/ffmpeg-overlay/codec_list.c)
- If a mission loads with missing audio, identify the requested wave bank and cue before changing the mixer. Bank version 34 alone does not identify the codec used by every entry.
- If graphics or flight timing differ, reproduce at the original resolution and settings before adding PC enhancements. Use the measurements to decide whether a host-side adaptation is needed.
