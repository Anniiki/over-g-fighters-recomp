# First native boot on Windows

On November 13, 2026, two locally compiled Windows x64 executables entered translated game code at `0x8235EBA8`. Both stopped on the same unregistered guest address, `0x824A95A8`, before reaching the menu.

## Reproduce the build

1. Start with the USA/Europe Xbox 360 disc identified in the [inventory](inventory.md). Put the checkout and a directory named `private` next to one another. From the checkout, set `$iso` to the full path of your local disc. Compare its SHA-256 to the inventory. With XDVDFS v0.8.3 available, extract the image locally. An existing, complete extraction can be placed directly in `../private/game`. [XDVDFS release](https://github.com/antangelo/xdvdfs/releases/tag/v0.8.3)

   ```powershell
   Get-FileHash -LiteralPath $iso -Algorithm SHA256
   New-Item -ItemType Directory -Force ../private | Out-Null
   xdvdfs unpack "$iso" ../private/game
   ```

2. Install the Windows x64 C++ build tools, Windows SDK, CMake and Ninja. The tested versions were Visual Studio 2022 Build Tools 17.14.45, Windows SDK 10.0.26100.0, CMake 3.31.6 and Ninja 1.12.1. The Visual Studio CMake component supplies both CMake and Ninja on the reference PC.
3. Install LLVM 20.1.8 for Windows x64. Download the ReXGlue v0.10.1 Windows AMD64 ZIP, verify its SHA-256 and extract the directory containing `bin`, `include`, `lib` and `share`. The table gives the tested downloads. [LLVM release](https://github.com/llvm/llvm-project/releases/tag/llvmorg-20.1.8) and [ReXGlue release](https://github.com/rexglue/rexglue-sdk/releases/tag/v0.10.1)

   | Download | SHA-256 |
   | --- | --- |
   | `LLVM-20.1.8-win64.exe` | `3197846a2b19063687dd56e93e34cd941e3548d907f23a6131571321bdf9fe7b` |
   | `rexglue-sdk-0.10.1-win-amd64.zip` | `26dbcd19e6976a7ad349917bffc58c1a0f21ad43d71a963d16b623c8e70d1f4d` |

4. Place LLVM in `../private/tools/llvm` and the SDK package root in `../private/tools/rexglue/win-amd64`, or pass `-LlvmBin` and `-SdkRoot` to the script. In PowerShell, run the following command from the checkout.

   ```powershell
   ./scripts/build.ps1
   ```

   The script checks the XEX size and SHA-256, selects the Visual Studio x64 environment, runs code generation and links the host with the matching SDK. The default output contains `overg.exe`, `rexruntimerd.dll`, `rexgpu-xenosrd.dll` and `TracyClientrd.dll`. Keep these files together.

5. Run a boot test with a new private profile. The current executable exits during startup. Trace logging below captures the first successful archive opens. Use a fresh output directory for a comparison run.

   ```powershell
   $game = (Resolve-Path ../private/game).Path
   $proof = (New-Item -ItemType Directory -Force ../private/boot-test).FullName
   $profile = (New-Item -ItemType Directory -Force "$proof/profile").FullName
   $exe = './out/build/win-amd64-relwithdebinfo/overg.exe'
   & $exe "--game_data_root=$game" "--user_data_root=$profile" "--log_file=$proof/boot.log" --log_level=trace --log_noisy=true --log_flush_interval=1 | Out-Null
   Select-String -LiteralPath "$proof/boot.log" -Pattern 'entered recompiled','NtCreateFile','unregistered function'
   ```

## Observed results

| Check | Evidence |
| --- | --- |
| Independent inputs | The full ISO and extracted XEX matched the recorded SHA-256 fingerprints. All 81 extracted files matched their source extents. Synthetic XEX files with the wrong size or the right size and wrong digest were rejected before code generation. |
| First translation | The SDK initially rejected a direct branch from `0x824B4FCC` to `0x824AA1F0`. Giving the analyzer that destination in the [title configuration](../config/entry.toml) allowed validation to finish and codegen to write 227 files. |
| Independent build | A second clean Git worktree at `492e478` had no generated directory or build cache. The public build script regenerated the translation, configured Clang 20.1.8 and linked a PE x64 executable. Both worktrees used the same verified private input and toolchain. |
| Guest entry | Each runtime registered 12,048 translated functions, loaded the Xenos plugin and reached the [entry marker](../src/main.cpp). Codegen inserts the marker directly after the instruction at `0x8235EBA8`. |
| First file access | In the second run, guest `NtCreateFile` requests for `game:\test.IDX` and `game:\test.000` both returned success. D3D12 initialized on the reference Radeon RX 6950 XT and the guest registered an audio render client. |
| Repeatable stop | Each executable exited on its own with Windows status `0xC0000409`. The last runtime diagnostic reported an invalid or unregistered function at guest address `0x824A95A8`. The menu, movie, audio playback and gameplay remain unverified. |

The two local executable hashes differ. The reproducibility check covers regeneration, compilation and the observed runtime behavior. The failing address lies in an executable guest page, directly after a terminal branch at `0x824A95A4` and 32 bytes before the next registered function. The next debugging step is to identify the caller and inspect that interval before adding a translation hint.
