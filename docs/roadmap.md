# Roadmap

## Milestones

| Stage | Work | Exit condition | Status |
| --- | --- | --- | --- |
| 0. Disc and project definition | Identify the disc and executable, survey the workstation, compare existing ports and define a first playable milestone. | Record the hashes, repeat the disc inventory with a second reader and document scope and acceptance tests. | Inventory and planning complete |
| 1. Toolchain and private input | Resolve Clang, activate the Visual Studio x64 environment, install the pinned SDK, extract the disc locally and verify the XEX. | The tool versions and SDK checksum are recorded. The extracted XEX matches the inventory. A deliberately incorrect input produces a useful error. | Complete |
| 2. Translation and native build | Initialize the host project, generate the guest C++, resolve analysis errors and link a Windows x64 executable. | From a second checkout, reproduce the build and log entry into the guest at `0x8235EBA8`. Record any unresolved imports or indirect calls. | Complete, see [boot evidence](first-boot.md) |
| 3. Boot, video and menu | Trace file opens, first GPU submission, movie playback and input. | Reach a responsive menu on cold start. Pass both the full introduction and skip tests. Document any temporary diagnostic bypass separately. | Both openings reach the title. The skip path reaches difficulty selection. Two early movie frames have shared-clock A/V measurements. Full-playback menu entry and late A/V measurements remain open. [Startup tests](first-boot.md#startup-tests) |
| 4. First playable mission | Load a mission from a new profile, verify cockpit, HUD, controller axes, audio and completion, then persist progress. | Another tester repeats the named mission and reloads its progress after restarting the application. | A player cleared *Intercept the bogeys* on Beginner with a physical controller. A separate cold-start test loaded the cleared save and reached Area 2 mission 1.1A. Independent repetition of the first clear is pending. [Post-clear restart](first-boot.md#cold-restart-after-clearing-area-1) |
| 5. Campaign and fidelity | Play the offline campaign, test the disc languages, compare flight timing and record GPU, audio and save regressions. | Complete the campaign route and the [acceptance matrix](specification.md#acceptance-tests). Record the comparison platform and measured performance. | In progress. On Beginner, the private QA pilot cleared the first South America tactical mission with rank A and 7,800 points. The saved slot opened tactical mission 2. [Far East results](first-boot.md#far-east-campaign-progression) and [South America results](first-boot.md#south-america-campaign-progression) |
| 6. Public release candidate | Package project-owned sources, provide reproducible local setup, credit dependencies and test the published instructions. | On a clean Windows system, a tester builds from the release and their own disc, completes the smoke mission and resumes the save. The public archive passes its content review. | Pending |

## Reproduce the current build

Follow the [Windows build and boot procedure](first-boot.md) from a clean checkout with a verified copy of the reference disc. The manifest and host scaffold are already tracked. The build script runs code generation before the first CMake configure.

## Early decisions driven by tests

- If code generation misses an indirect branch target, verify the target against the guest instruction stream and add the smallest reproducible manifest hint. A successful link alone cannot prove that a mission's indirect calls resolve.
- For a movie regression, compare captured game frames and audio with the local disc movie and check the transition to the title. Keep shared-clock synchronization measurements separate from waveform matches. [Playback results](first-boot.md#full-and-skipped-opening)
- If a mission loads with missing audio, identify the requested wave bank and cue before changing the mixer. Bank version 34 alone does not identify the codec used by every entry.
- If graphics or flight timing differ, reproduce at the original resolution and settings before adding PC enhancements. Use the measurements to decide whether a host-side adaptation is needed.
