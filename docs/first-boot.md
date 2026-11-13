# First native boot on Windows

On November 13, 2026, two independent Windows x64 builds entered translated game code at `0x8235EBA8`. Their first failure at `0x824A95A8` provided the starting point for the subsequent [startup tests](#startup-tests).

## Reproduce the build

1. From the checkout, prepare the USA/Europe disc identified in the [inventory](inventory.md). Set `$iso` to the path of your local ISO and supply the Windows XDVDFS 0.8.3 executable. If `xdvdfs.exe` is on PATH, omit `-XdvdfsPath`. [XDVDFS release](https://github.com/antangelo/xdvdfs/releases/tag/v0.8.3)

   ```powershell
   .\scripts\prepare-game.ps1 -IsoPath $iso -XdvdfsPath 'C:\Tools\xdvdfs.exe'
   ```

   For an existing extraction at `../private/game`, run `.\scripts\prepare-game.ps1` without arguments. Verification works without the ISO or XDVDFS. Use `-GameRoot` to prepare or verify a different directory outside the checkout. The build script reads `../private/game`.

   The script reports the verified ISO SHA-256 and checks the size and SHA-256 of all 81 disc files, including the audio banks, movie, archives and executable. It preserves extra local files and refuses to replace an incomplete or altered extraction. For a new extraction it verifies a temporary directory on the destination volume before renaming it into place. A forced process exit can leave an unpublished `.overg-unpack-*` directory in the destination's parent. A repeat run uses a new temporary name.

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

5. Run a boot test with a new private profile. Trace logging captures the first successful archive opens. Use a fresh output directory for a comparison run.

   ```powershell
   $game = (Resolve-Path ../private/game).Path
   $proof = (New-Item -ItemType Directory -Force ../private/boot-test).FullName
   $profile = (New-Item -ItemType Directory -Force "$proof/profile").FullName
   $exe = './out/build/win-amd64-relwithdebinfo/overg.exe'
   & $exe "--game_data_root=$game" "--user_data_root=$profile" "--log_file=$proof/boot.log" --log_level=trace --log_noisy=true --log_flush_interval=1 | Out-Null
   Select-String -LiteralPath "$proof/boot.log" -Pattern 'entered recompiled','NtCreateFile','unregistered function'
   ```

## Check the SDK without a disc

The Windows SDK check runs on a hosted Windows Server 2022 machine. It verifies the official LLVM and ReXGlue downloads, compiles and links the project's language import, then checks that the SDK setting reaches the guest register for all four supported console languages. The first run passed for English, German, French and Spanish. Full game builds and campaign tests require the player's files. [Hosted test log](https://github.com/Anniiki/over-g-fighters-recomp/actions/runs/43402438995/job/131808360508)

To run the same compiler and runtime check locally, use PowerShell 7.4 or newer with the tools from steps 2 and 3.

```powershell
.\tests\windows-sdk\check.ps1 -SdkRoot ../private/tools/rexglue/win-amd64 -LlvmBin ../private/tools/llvm/bin -BuildDir ./out/sdk-check
```

## Initial build at 492e478

| Check | Evidence |
| --- | --- |
| Independent inputs | The full ISO and extracted XEX matched the recorded SHA-256 fingerprints. All 81 extracted files matched their source extents. Synthetic XEX files with the wrong size or the right size and wrong digest were rejected before code generation. |
| First translation | The SDK initially rejected a direct branch from `0x824B4FCC` to `0x824AA1F0`. Giving the analyzer that destination in the [title configuration](../config/entry.toml) allowed validation to finish and codegen to write 227 files. |
| Independent build | A second clean Git worktree at `492e478` had no generated directory or build cache. The public build script regenerated the translation, configured Clang 20.1.8 and linked a PE x64 executable. Both worktrees used the same verified private input and toolchain. |
| Guest entry | Each runtime registered 12,048 translated functions, loaded the Xenos plugin and reached the [entry marker](../src/main.cpp). Codegen inserts the marker directly after the instruction at `0x8235EBA8`. |
| First file access | In the second run, guest `NtCreateFile` requests for `game:\test.IDX` and `game:\test.000` both returned success. D3D12 initialized on the reference Radeon RX 6950 XT and the guest registered an audio render client. |
| Repeatable stop | Each executable exited on its own with Windows status `0xC0000409`. The last runtime diagnostic reported an invalid or unregistered function at guest address `0x824A95A8`. The menu, movie, audio playback and gameplay remain unverified. |

The two local executable hashes differ. The reproducibility check covers regeneration, compilation and the observed runtime behavior.

## Startup tests

These tests use the USA/Europe executable identified by SHA-256 in the [inventory](inventory.md). Each row describes an observed failure or a test of a change in the [title configuration](../config/entry.toml).

| Site | Observation |
| --- | --- |
| Indirect calls at `0x824B7690`, `0x823210E4` and `0x8248EC08` | Separate runs stopped at three unregistered callbacks. The guest call sites, function pointers and instruction boundaries established the entries at `0x824A95A8`, `0x8231B990` and `0x82488BD0`. Registering each target let the next run pass its failing call. |
| Image decoder | The decoder at `0x82472840` saves a continuation. The error handler at `0x82470910` restores it when the decoder rejects input. Configuring the guest setjmp and longjmp pair let the title resume image loading. A cold start then displayed the copyright, Space Imaging, Taito and Ubisoft screens. |
| Transition after Ubisoft | The callback at `0x824B52D8` adjusts the object pointer and branches to `0x824B5150`. Registering the missing two-instruction callbacks from the same table let a fresh run pass the previous stop and create another D3D12 graphics pipeline. |
| Audio mixing | The Windows audio worker raised a floating-point inexact-result exception in translated function `0x824C2CB8`. ReXGlue initializes the floating-point context for regular guest threads but omits it on the host thread used for this callback. Initializing that context at `0x824B5EF0` let the movie play. A window capture matched the local disc video at 1.5, 3.5, 5.5 and 7.5 seconds. A separate four-second capture of the game process during the movie contained audio. |

### Full and skipped opening

On a fresh profile the movie played through its final transition, and the title screen displayed its Start prompt about 149 seconds after launch. Window captures matched the disc video at multiple positions, including 88 seconds. Game-process audio matched the disc soundtrack in 32 two-second windows over 124 seconds, with normalized correlations from 0.972 to 0.992. The audio offset stayed constant within the 0.167 ms analysis resolution. The capture did not record the audio and video against a shared clock, so it cannot establish their absolute synchronization.

On another fresh profile the XInput backend received Start from a virtual Xbox 360 controller about 49 seconds after launch. The title screen appeared by 51 seconds. Both runs subsequently entered the automatic demonstration and stopped loading it at the same branch table in `0x825C7008`. Registering the four destinations at `0x825C70B0` allowed a later unattended title screen to enter the demonstration and render the carrier scene. The disc index contains five demonstration replay files. This scene was observed separately from the player-controlled Scenario takeoff.

### Main menu on a fresh profile

A further cold start used the XInput backend and a virtual Xbox 360 controller. Start during the movie and again at the title opened the storage flow. The title found no saved data, offered to create it and confirmed creation. The runtime mounted its `save:` volume and the player directory received five 29,232-byte files named `Over-G.sav0` through `Over-G.sav4`, plus a container header. These are initial slot files. A completed mission and a cold reload are separate save tests.

The main menu displayed New Game, Load Game, Replay Gallery and Credits. D-pad Down selected Load Game, D-pad Up returned to New Game, and A opened the Beginner and Expert difficulty choices.

### First Scenario launch

The Beginner route opened the pilot-name dialog. The test entered a name with the Windows keyboard and confirmed it in the SDK dialog. The game saved slot 01, reported success and opened the Contents menu. The slot's file digest changed while the four empty slots retained the same digest.

Selecting Scenario showed the controller loading screen, a city and a carrier group. The next scene showed the player's cockpit on the carrier, with a visible HUD, deck, radar and launch clearance from the carrier operator. The D3D12 log reported repeated empty resolve regions during the preceding menu. Mission success, radio audio fidelity and recovery of completed progress require further tests.

Increasing thrust with the controller's right bumper started the launch. The deck moved beneath the aircraft and the HUD instruction changed from increasing thrust to increasing altitude. The right stick button changed the camera. Start opened the pause menu with a tactical map, and the player could return to the flight and switch to an exterior view.

One carrier clearance was identified independently in both picture and sound. Two cockpit captures showed the English clearance subtitle. In the audio captured from the game process, 1.542 seconds of speech matched entry 56 (zero-based) of the disc's English global voice bank. Normalized correlation was 0.951 in the 180 to 1800 Hz band. The next-best match among the 3,174 English and Japanese entries scored 0.223. These observations identify the voice for this cue.

### Cold restart before the first mission clear

The process used for the first takeoff exited. A new process loaded the same private profile using the main menu's Load Game option. Slot 01 showed the saved pilot name, the Greenhorn class and `Area1(Beginner)/00`. The title confirmed the load and returned to the campaign. The [post-clear restart](#cold-restart-after-clearing-area-1) used a later save.

### Physical controller

A player then started a separate fresh profile using [play.cmd](../play.cmd) and a physical USB Xbox-compatible controller. Windows exposed the pad at XInput index 1. Start and A were followed by visible transitions through the title, game data creation and the pilot slot screen. In Scenario the player took off from the carrier. The HUD showed 209 kt and 810 ft with the landing gear reminder, then 296 kt and 1,140 ft in a later exterior view. Frame captures and physical stick readings were recorded while the game had focus. This run establishes player-one operation with an actual controller.

The title displayed Mission Complete. The debriefing identified Scenario 1, *Intercept the bogeys*, and reported 1 min 41.43 s of flight time, five targets, 6,600 target points, 3,000 technical points, zero penalty and rank A (9,600 total). The player continued through the replay save confirmation to the next briefing. The process then stopped at the unregistered guest function `0x820C22E0`.

In this run, 104.6 to 108.6 seconds of real time elapsed between the opening carrier camera switching to the cockpit and the final flight HUD switching to the closing cinematic. The Mission Complete banner appeared 112.5 to 116.6 seconds after the cockpit appeared. These intervals use the Windows capture timestamps of the last frame before and first frame after each transition, sampled about two seconds apart. The debriefing's flight timer uses its own start and stop conditions. A reference run is needed to compare game speed.

The missing function is a seven-instruction leaf in slot `+0x20` of the table at `0x8200701C`. It advances two fields by the supplied offset and returns. The [translation hint](../config/entry.toml) registers its 28 bytes. The regenerated build includes all twelve entries in this table. An isolated test using the retail XEX and regenerated code exercised three translated callers, including a write and read of 3, 6 and 39 bytes. All 17 checks passed, including bounds and cursor updates.

### Cold restart after clearing Area 1

After the player's process exited, the test copied the complete save container and verified all seven files against the snapshot. A fresh process opened the copy with French selected and an SDL controller. In Load Game, slot 01 showed Area 2 on Beginner, one aircraft and one minute of total flight time. Loading that slot opened Central Africa. The new build advanced past the globe where the earlier process had stopped and displayed the area briefing on the operations map. The test selected mission 1.1A, the F-14A Tomcat and wingman Leon. The game displayed its control guide, the two-aircraft cinematic and then the cockpit, HUD and radar.

The save menu, area name, speaker dialogue, aircraft description and selection prompts appeared in French. The decorative newspaper columns around these menus remained in English.

The player subsequently relaunched from the standard XInput launcher with the per-profile `user_language = 4` setting. The physical controller opened the French title menu and Load Game screen, where the original slot 01 displayed the same Area 2 progress.

With French selected, audio from three briefing lines matched entries 0, 13 and 37 of the English briefing voice bank. The respective correlations were 0.970, 0.861 and 0.947. Each matched entry also had the index specified by its on-screen text record. These lines cover the president's address, the area briefing and the assignment for mission 1.1A.

### Mission failure and retry in Area 2

A ground impact in mission 1.1A triggered the French mission failure banner and the flight replay. Selecting Retry in the replay pause menu restarted the two-aircraft cinematic and returned to the F-14A cockpit with the HUD, radar and three targets displayed. After closing the process, all five campaign slots, the save header and its thumbnail still matched their pre-test SHA-256 hashes. The only new files under the test profile were two shader caches.

### Automated clear of Area 2 mission 1.1A

A private Windows QA build loaded a fresh copy of the Beginner Area 2 save, selected mission 1.1A in French and flew the player's F-14A from the cockpit to the success banner without human input. The test pilot used the player-one controller interface. It cycled past other selectable aircraft, selected both Tu-95s in the first group and turned toward the third aircraft when the second group appeared.

The target breakdown listed three Tu-95s for 5,400 points. The final results showed 47.35 seconds of flight time, a 600-point no-damage bonus, zero penalty, rank S and 6,000 total points. The title then awarded the F-16C Fighting Falcon. An earlier diagnostic run with pauses also finished with three Tu-95s and rank S. The regular play launcher still starts the game with the player's controller.

### Central Africa campaign progression

The private Windows QA pilot loaded a copy of the completed 1.1A save in French and selected the Central Africa tactical mission. The game's debrief counted four targets, zero penalty and rank A (3,720 points). Saving and returning to the map made the strategic mission available.

In the strategic mission, the player-one controller used Back to skip taxiing after clearance, took off from the runway and completed both objective groups in sequence. The debrief listed five BTR-80s, 3 min 56.93 s, zero penalty and rank C (4,200 points). The next save opened Area 3, Far East. The French region selector displayed the new area, and its first tactical mission could be entered.

### Far East campaign progression

The first Far East tactical mission ran on the same Beginner save copy. The QA controller flew the F-14A with Leon Ryoji Minakata as wingman. After the controller issued the retail AIR order, the flight HUD and the wingman's current order both confirmed its acceptance. The debrief counted four MiG-25s across two waves, 1 min 15.21 s, zero penalty and rank A (5,400 points). The game awarded the F-15J Eagle. After returning from replay, the saved slot recorded 5,400 points for tactical mission 1 and opened tactical mission 2.

The second Far East tactical mission used the F-16C and the game's standard loadout. The title credited two Baskunchak transports. The results showed 1 min 24.15 s, zero penalty and rank C (4,200 points). Both Far East tactical tests used the same Windows x64 QA executable, SHA-256 `5a3857fabb3be49c753a1fb888623b25cd675c71f910c0561d05d6b3050e8cea`, with French UI and subtitles.

### Opening audio and video on a shared clock

In the second launch, the window frames and audio captured from the game process used Windows performance-counter timestamps. At movie positions 0.400 and 3.436 seconds, the matched audio led the matched picture by approximately 18 and 3 ms respectively. The video sampling uncertainty is up to one source frame (33.4 ms), with up to 2 ms per accepted audio packet. Both observations are consistent with synchronization to within a frame. The movie was skipped on this run, so it did not measure absolute synchronization through the ending.

### Comparison with Xenia Canary

Xenia Canary `20106f8` ran the same verified disc on the same Windows PC, with D3D12 at native scale. The French reference session played the opening without controller input through the last matched frame at movie time 111.878 s. The test ended before the title screen. Comparing each captured picture to the disc movie gave the following rates. [Reference build](https://github.com/xenia-canary/xenia-canary-releases/releases/tag/20106f8)

| Playback | Matched moving frames | Elapsed capture time | Movie seconds per capture second |
| --- | ---: | ---: | ---: |
| Windows native port | 38 | 116.545 s | 1.0000633 |
| Xenia Canary, D3D12 | 51 | 111.413 s | 1.0000317 |

The reference audio followed the captured video by a median 38.4 ms across 49 matched samples. The uncertainty from sampling the video is up to 33.4 ms. The two native shared-clock samples above are too few to establish a persistent difference in synchronization. Actual Xbox 360 output and game simulation speed still require separate measurements.
