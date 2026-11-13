# Inventory and prior work

Survey date: November 13, 2026. Disc measurements below come from read-only inspection of the supplied image. The workstation survey describes the development machine, not player requirements.

## Reference disc

| Property | Observed value |
| --- | --- |
| Catalog edition | USA and Europe, serial TI-2001, catalog languages English, French, German and Spanish |
| Image size | 7,834,892,288 bytes |
| Full image SHA-256 | `47e6c331baee6b10bbaf0f2fd6172446dc5d35790bccb6af083505f4e0cf61d7` |
| Full image SHA-1 | `97cc539a1c987fc5dbded1e80832040154cbb68a` |
| XDVDFS game partition | Offset `0x0FD90000`, volume descriptor at `0x0FDA0000` |
| Directory inventory | 81 files total across the root and six subdirectories, 3,427,056,800 bytes of file payload |

The full-image SHA-1 and size match the Redump entry distributed in the libretro database. Individual menu, subtitle and voice language options still need testing on this disc. [Catalog record](https://github.com/libretro/libretro-database/blob/9a899f5b13ed1c355491e342203be29bc573323b/metadat/redump/Microsoft%20-%20Xbox%20360.dat#L16324-L16327)

## Executable

| Property | Observed value |
| --- | --- |
| Entrypoint file | `default.xex`, the only XEX, 7,454,720 bytes |
| SHA-256 | `51cc8b46170e7e965a9a4655727c98e7c9e83abc73a70aae8cc156d26edef453` |
| Container and original PE name | XEX2, `ea3m.pe` |
| Title ID and Media ID | `544907D1` and `72CA730E` |
| Version and base version | Both `0x00000002` (0.0.0.2) |
| Guest image base and entry address | `0x82000000` and `0x8235EBA8` |
| Import records | 140 for `xam.xex` and 241 for `xboxkrnl.exe` |

Import records can refer to the same service more than once. Measure missing runtime services from actual resolution and execution logs. Static library names include D3D9, XMEDIA, XACT and XONLINE. No game code was executed during this inventory.

## Resource groups

| Disc content | Measurement | First compatibility check |
| --- | --- | --- |
| Indexed game data | At the disc root, `test.000` is 1,095,296,453 bytes and `test.IDX` is 147,456 bytes. The index contains resource names. | Keep both files intact. Trace path lookup, seeking and reads while loading the first mission. The full index layout is unverified. |
| Audio | 74 XWB wave banks in `data/stream/`, 2,188,875,776 bytes total. All have the signature `DNBW` and bank version 34. The filenames split into 35 ending in `Eng.xwb`, 35 in `Jpn.xwb` and four music banks. | Check engine loops, radio speech, effects and music independently. Record which bank and language the running game selects. |
| Opening movie | `data/menu/movie/opening_hd-cc.wmv`, 133,578,122 bytes. ASF container, WMV3 at 1280 x 720 and 30000/1001 fps, WMA v2 stereo at 48 kHz. Header duration 130.997 seconds. | Play through to the menu and test skipping separately. The movie frame rate gives no measurement of simulation timing. |
| Other files | Two PNGs, one disc update package and the executable complete the 81-file count. | Keep disc resources private and exclude system update content from the published project. |

The file count, directory count, payload total and paths were independently reproduced with `xdvdfs tree` v0.8.3 on the full image. The media probe read the movie header directly from the disc image. [XDVDFS tool release](https://github.com/antangelo/xdvdfs/releases/tag/v0.8.3)

## Development workstation

| Component | Observed state |
| --- | --- |
| Reference PC | Windows 11 Pro x64, Ryzen 7 5800X3D, Radeon RX 6950 XT, approximately 32 GB RAM |
| Windows toolchain | Visual Studio 2022 Build Tools 17.14.45, MSVC 14.44 and Windows SDK 10.0.26100.0 |
| Build tools | CMake 3.31.6 and Ninja 1.12.1 run from the Visual Studio installation. Neither command resolved in the default PowerShell session. |
| Utilities | Git 2.54.0, GitHub CLI 2.96.0, Python 3.14.5 and FFprobe 8.1.1 |
| First-build additions | LLVM 20.1.8 and the ReXGlue 0.10.1 Windows SDK were installed in the private tools directory. The build script locates them there or accepts explicit paths. [Build and boot evidence](first-boot.md) |

## What other ports demonstrate

- **Midnight Club: Los Angeles.** Its Windows build pins the SDK, applies documented patches, extracts the player's disc and recompiles locally. A partial build can resume. Its manifest keeps the entrypoint, output directory and title-specific analysis hints separate. Adopt this build structure and derive all addresses from the Over G Fighters XEX. [Build guide](https://github.com/RenanQueiroz/mcla-recomp/blob/15004085bbf233715af65c2243e8cbd79a2ea12b/docs/building.md) and [manifest](https://github.com/RenanQueiroz/mcla-recomp/blob/15004085bbf233715af65c2243e8cbd79a2ea12b/mcla_manifest.toml)
- **Import Tuner Challenge.** Its manifest records missed tail-call targets. Its function notes record guest addresses, tentative names and implementation status. Use those conventions for indirect calls observed in this title. ITC's custom Vulkan renderer and 30 Hz game logic are specific to that port. [Manifest](https://github.com/Spooky098529/ITC-Recomp/blob/b322c1d6be1fb01d427d035c18d142d1926ebaae/itc_recomp/itc_manifest.toml), [function notes](https://github.com/Spooky098529/ITC-Recomp/blob/b322c1d6be1fb01d427d035c18d142d1926ebaae/docs/functions/functions.md) and [rendering notes](https://github.com/Spooky098529/ITC-Recomp/blob/b322c1d6be1fb01d427d035c18d142d1926ebaae/README.md)
- **Over G Fighters in Xenia Canary.** A July 2025 Windows D3D12 report for this Title ID reached gameplay and observed minor flickering at 1440p. Use it to select visual checks for the cockpit and HUD at native resolution and at 1440p. It does not establish native port compatibility or identify the exact disc hash used by the reporter. [Compatibility report](https://github.com/xenia-canary/game-compatibility/issues/716)
