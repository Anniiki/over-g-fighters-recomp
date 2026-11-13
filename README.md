# Over G Fighters Recomp

Over G Fighters Recomp is a planned native Windows PC port of the Xbox 360 game. It uses the ReXGlue SDK to translate the original PowerPC executable into C++ ahead of time. Each player supplies their own copy of the supported disc.

**Status on November 13, 2026.** The reference disc and executable have been identified. Code generation, the first native build and gameplay verification are pending. There is no playable release.

## Project goal

Build a Windows x64 application that can play the original offline campaign from the supported disc. A player must be able to fly with a controller, complete missions, save progress, close the application and continue after restarting it. Flight timing, cockpit instruments, HUD, radio cues and mission events will be compared with the Xbox 360 version.

The first playable milestone is smaller. Starting from a clean profile, reach the menu, enter one named mission, fly it to its success condition, return to the menu and reload the resulting progress after a cold restart. Record the mission and aircraft so another tester can repeat the run.

## Planning documents

| Document | What it establishes |
| --- | --- |
| [Inventory and prior work](docs/inventory.md) | Disc and XEX fingerprints, game resources, development tools and lessons from other ports |
| [Project specification](docs/specification.md) | Supported target, build design, scope, fidelity requirements and acceptance tests |
| [Roadmap](docs/roadmap.md) | Implementation order, evidence needed at each milestone and the first build procedure |

The public project is intended to contain original host code, build scripts, configuration and documentation. The ISO, extracted game files, generated translation and local game executable belong on the player's machine.
