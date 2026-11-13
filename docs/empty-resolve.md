# Empty Xenos resolve rectangles

Some Over G Fighters scenes issue resolve packets that have no covered pixels after clipping or surface-pitch limits. ReXGlue 0.10.1 can reject an aligned empty rectangle with `Resolve region is empty`. Its outward alignment to 8-pixel boundaries can also turn an empty rectangle at an unaligned coordinate into a nonempty one.

The optional plugin checks the clipped dimensions before alignment. It also handles a rectangle that becomes empty only after the horizontal surface-pitch clamp. Both GPU backends check these zero dimensions before copying or clearing. This Windows build uses D3D12. Nonempty coverage retains the SDK's alignment, copy and color/depth clear handling. Unsupported vertex fetches and MSAA modes still fail. A separate build patch points libmspack at its original C source so ordinary Windows checkouts can build without symbolic-link privileges.

## Build and check

Use 64-bit Windows, Visual Studio C++ Build Tools, a Windows SDK, CMake, Ninja, Git and Clang 18 or later. The pinned ReXGlue source and its submodules download into a new work directory. To run the register tests, set `REXSDK` to the installed Windows x64 ReXGlue 0.10.1 distribution, or use the project layout at `../private/tools/rexglue/win-amd64`.

```powershell
./scripts/build-resolve-plugin.ps1 -WorkDir C:/build/overg-resolve -LlvmBin C:/llvm/bin -Jobs 1 -Verify -StageBesideExecutable C:/qa/overg.exe
& C:/qa/overg.exe --gpu_plugin=overg-resolve
```

The script stages `rexgpu-overg-resolverd.dll` beside the test executable. Omit the plugin argument to use the ordinary Xenos plugin again. Use a new work directory if upgrading from an earlier revision of the patch. The port accepts the normal guest data and profile arguments on either launch.

## Verification

The tests execute the same graphics objects linked into the alternate plugin. Eight rectangles cover empty lines, an outside or reversed scissor, pitch clipping, one covered pixel, and a nonempty clipped region. Each case runs for a color or depth source, raw or converted copy, and all four combinations of color and depth clear flags. The patched implementation passed all 128 combinations and rejected two malformed inputs. Substituting only the stock SDK's draw.cpp produced 96 failures with the same fixture. A line at x=13 produced an 8-by-8 region and a fully clipped rectangle produced an 8-by-40 region.

With the supported XEX (SHA-256 `51cc8b46170e7e965a9a4655727c98e7c9e83abc73a70aae8cc156d26edef453`), separate runs of the same private Windows executable loaded the stock and alternate plugins. The title menu and the Area 2 map were recognized in both runs. Across-plugin image differences were smaller than the sampled animation differences within either launch. Neither scene logged an empty-region error with either plugin, so these runs establish menu compatibility. A paired capture of a campaign sequence that reproduces the error is still needed to quantify its log rate and frame timing.
