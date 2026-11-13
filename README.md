# Over G Fighters Recomp

Over G Fighters Recomp is a native Windows PC port in development. It uses the ReXGlue SDK to translate the Xbox 360 PowerPC executable into C++ ahead of time. Each player supplies their own copy of the supported disc.

**Status on November 13, 2026.** A player used a physical Xbox-compatible controller to start a fresh Beginner campaign, take off from the carrier and fly the first Scenario mission. The opening movie plays with its original audio. Pilot-name entry currently uses the Windows keyboard. A separate cold restart recovered the initial pilot save. Mission completion and recovery of completed progress are still under test. [Build procedure and test results](docs/first-boot.md#physical-controller)

## Play with a controller

After [building the port](docs/first-boot.md#reproduce-the-build), connect your Xbox-compatible controller and double-click [play.cmd](play.cmd). The launcher uses XInput and stores your campaign data in `%LOCALAPPDATA%\Over G Fighters Recomp`. Keep the same profile for **Load Game** after restarting. For an SDL-compatible controller, launch `.\play.cmd sdl` from the checkout.

1. Press Menu (Start) to skip the opening movie, then press it again at the title. On a new profile, confirm creation of the game data with A.
2. Choose **New Game**, a difficulty and a save slot. Enter your pilot name with the Windows keyboard and confirm the dialog. Save, then choose **Scenario** in Contents.
3. In flight, use the left stick for pitch and roll, RB and LB for thrust, the triggers for rudder, the right stick button for camera view and Start to pause. The first Scenario mission opens on the carrier.

## Project goal

Build a Windows x64 application that can play the original offline campaign from the supported disc. A player must be able to fly with a controller, complete missions, save progress, close the application and continue after restarting it. Flight timing, cockpit instruments, HUD, radio cues and mission events will be compared with the Xbox 360 version.

The first playable milestone is smaller. Starting from a clean profile, reach the menu, enter one named mission, fly it to its success condition, return to the menu and reload the resulting progress after a cold restart. Record the mission and aircraft so another tester can repeat the run.

## Planning documents

| Document | What it establishes |
| --- | --- |
| [Inventory and prior work](docs/inventory.md) | Disc and XEX fingerprints, game resources, development tools and lessons from other ports |
| [Project specification](docs/specification.md) | Supported target, build design, scope, fidelity requirements and acceptance tests |
| [Roadmap](docs/roadmap.md) | Implementation order, evidence needed at each milestone and the first build procedure |

The repository contains the host scaffold, build script, configuration and documentation. The ISO, extracted game files, generated translation and game executable stay on the player's machine. The host scaffold uses the [ReXGlue license](third_party/ReXGlue-LICENSE.txt).
