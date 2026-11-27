# Over G Fighters Recomp

A Windows x64 port of Over G Fighters, written in C++23. ReXGlue statically translates the Xbox 360 PowerPC executable. The host code implements title-specific fixes, controller input for the pilot-name dialog, player settings and achievement notifications. Each player supplies their own USA/Europe disc.

## Campaign status

On Beginner, a player finished the opening mission with a physical controller. A cold restart loaded the cleared slot and opened Central Africa. Tests on a copy of that career subsequently cleared the Central Africa and Far East strategic missions. In South America, the save records 7,800 points for the stealth mission and 6,840 for the transport escort. A strategic VIP escort run reached phase 3. After the mission failed, Retry returned the player to the cockpit. A full campaign playthrough and installation on a clean Windows system remain open.

French menus and cockpit subtitles have been observed. A process audio recording matched an English radio cue in Central Africa with French selected. The spoken language in the Amazon missions is still unverified.

## Build on Windows

Use Visual Studio 2022 Build Tools with Desktop development with C++, a Windows SDK, Clang 20.1.8, CMake 3.25 or later, Ninja and the ReXGlue 0.10.1 Windows AMD64 SDK. The tested SDK archive has SHA-256 `26dbcd19e6976a7ad349917bffc58c1a0f21ad43d71a963d16b623c8e70d1f4d`. [LLVM release](https://github.com/llvm/llvm-project/releases/tag/llvmorg-20.1.8) · [ReXGlue release](https://github.com/rexglue/rexglue-sdk/releases/tag/v0.10.1)

1. From the checkout, extract your disc with `xdvdfs unpack "<disc.iso>" "../private/game"`. The supported `default.xex` is 7,454,720 bytes with SHA-256 `51cc8b46170e7e965a9a4655727c98e7c9e83abc73a70aae8cc156d26edef453`. Verify it with `certutil -hashfile "../private/game/default.xex" SHA256`. [Disc fingerprints](config/disc-files.sha256) · [XDVDFS 0.8.3](https://github.com/antangelo/xdvdfs/releases/tag/v0.8.3)
2. Open an x64 Native Tools Command Prompt for Visual Studio. Put the LLVM and ReXGlue `bin` directories on `PATH`. From the checkout run `rexglue codegen overg_manifest.toml`. This generates the translation in `generated/default` and the SDK build definitions in `generated/rexglue.cmake`.
3. Save the following build description as a local `CMakeLists.txt` at the root of the checkout.

```cmake
cmake_minimum_required(VERSION 3.25)
project(overg LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(REXSDK_VERSION 0.10.1)
include(generated/rexglue.cmake)
add_executable(overg WIN32 src/main.cpp src/language.cpp src/gamepad_keyboard.cpp)
rexglue_setup_target(overg GPU_PLUGINS xenos)
```

4. Configure and build. Replace `C:/SDK/rexglue` with the installed SDK directory containing `bin`, `include`, `lib` and `share`.

```text
cmake -S . -B out/build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_CXX_COMPILER=clang++ "-DCMAKE_PREFIX_PATH=C:/SDK/rexglue"
cmake --build out/build --target overg
```

## Play

From the checkout, run the following in Command Prompt to use the extracted disc, an Xbox controller and the profile used by the earlier Windows launcher.

```text
out\build\overg.exe "--game_data_root=%CD%\..\private\game" "--user_data_root=%LOCALAPPDATA%\Over G Fighters Recomp" --input_backend=xinput
```

For a standalone installation, put the extracted disc in a `game` directory beside `overg.exe`. Copy `rexruntimerd.dll`, `rexgpu-xenosrd.dll` and `TracyClientrd.dll` from the RelWithDebInfo build to the same folder as the executable. The target PC needs the Microsoft Visual C++ v14 x64 runtime. [Microsoft runtime download](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170)

On direct launch the port discovers `game/default.xex` and checks its SHA-256 fingerprint before loading the game. The default Windows profile is `%LOCALAPPDATA%\Over G Fighters Recomp`. If only the earlier standalone SDK profile exists, the port uses it. Explicit game, profile and cache paths take precedence over their respective defaults.

Use `--input_backend=sdl` for an SDL controller. Set `user_language = 4` in the profile's `overg.toml` for French, 1 for English, 3 for German or 5 for Spanish. In the pilot-name dialog, the D-pad moves, A types, Y changes case, X deletes, Start confirms and B cancels.

## Source and dependencies

The public source includes the C++ host, the [translation manifest](overg_manifest.toml), [guest function and hook definitions](config/entry.toml) and the [ReXGlue notice](third_party/ReXGlue-LICENSE.txt). Game files, generated translations and player saves stay on the player's machine.
