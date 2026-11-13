# Over G Fighters Recomp

Over G Fighters Recomp is a native Windows PC port in development. It uses the ReXGlue SDK to translate the Xbox 360 PowerPC executable into C++ ahead of time. Each player supplies their own copy of the supported disc.

**Status on November 13, 2026.** A locally built Windows x64 executable renders the copyright, Space Imaging, Taito and Ubisoft screens on a fresh profile. The opening movie, responsive menus and campaign still need end-to-end verification. [Build procedure and test results](docs/first-boot.md)

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
