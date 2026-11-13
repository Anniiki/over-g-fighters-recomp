# Frame pacing on Windows

Presentation rate, display timing and game time answer three different questions. PresentMon records when Windows receives a presentation and, when available, when it changes the display. The mission clock and scripted events show how quickly the guest simulation advances. Use the same build, scene and measurement window to compare them.

## Capture a repeatable scene

1. Record the executable hash, Windows and driver versions, GPU, display refresh rate, variable refresh setting, output resolution, window mode and any frame limiter. Keep the same settings throughout each comparison. Record the guest video settings, mission, aircraft, difficulty, save checkpoint and input sequence.
2. Let the French or English main menu settle, record its selected item and leave the game window in the foreground for **120 seconds**. Disable video capture, window observers and framebuffer readback for the baseline. Keep overlay and sensor settings consistent and record any change of focus or scene. Run capture utilities from an already prepared background process or allow a lead-in before the measurement window.
3. Repeat for **120 seconds of flight** from one named checkpoint. Use the same aircraft, camera, input sequence and mission-clock interval. Keep loading and briefing in separate measurement windows. Record two guest events with both guest time and elapsed wall time, such as control handoff and a scripted radio cue. Use an external observation or a lightweight event log for the timing marks. A separate capture can document the screens.
4. Run each scene with a fresh private game cache, then repeat from the same save with the populated cache. Record which game and driver caches were warm. A dedicated QA profile keeps the player's cache and save available. Repeat any candidate fix under the same conditions and measure the original configuration again.

## Collect and summarize

The example below uses the PresentMon 2.6.0 CLI and its legacy interval columns. Set the three variables in PowerShell to the installed CLI path, game PID and a local evidence directory. Create that directory before starting the capture.

```powershell
& $presentMon --process_id $gamePid --timed 120 --terminate_after_timed `
    --v1_metrics --qpc_time --set_circular_buffer_size 8192 `
    --output_file "$evidence\menu.csv" 2> "$evidence\menu.stderr"
python scripts/summarize-presentmon.py "$evidence\menu.csv" `
    --stderr "$evidence\menu.stderr" --pid $gamePid
```

If the capture includes a lead-in or a scene transition, pass `--start-qpc` and `--end-qpc` in raw `TimeInQPC` ticks. The range includes the first tick and excludes the last. To convert timestamps reported in seconds or nanoseconds, use the machine's performance counter frequency and the same QPC origin. Keep UTC markers alongside the QPC values so that an external observation can identify the scene.

The script produces a JSON group for each application name, PID, swap chain and presentation mode. It reports p50, p95, p99, p99.9 and the maximum interval in milliseconds. Thresholds are strict. A 25 ms observation does not count toward `above_25_ms`. The percentage below 1 ms uses the number of valid presentation intervals as its denominator. Blank and `NA` entries count as missing. Measured zeroes remain in the distribution and have their own count.

Use `MsBetweenPresents` for application submission intervals and `MsBetweenDisplayChange` for available display intervals. Record both with the count of valid observations. Select the swap chain that renders the game and report any change in presentation mode. Read the capture diagnostics. Warnings about access and lost ETW events have separate implications. An overflow warning applies to the recording as a whole and can affect extreme percentiles. Where possible, repeat the capture and compare successive `TimeInQPC` differences with the corresponding presentation intervals.

## Interpreting the periodic pattern

In one foreground D3D12 menu recording, 7,671 presentations over 35.2 seconds averaged 217.9 presentations per second. The 2,028 intervals under 1 ms had a median of 0.294 ms. The remaining intervals had a median of 6.20 ms. The short interval usually returned every third or fourth presentation. This produced a regular comb-shaped trace. An overlay reading near **221 FPS** can therefore coincide with an instantaneous reading near **0.3 ms**. The mean interval at 221 FPS is about 4.52 ms. More than one cadence can contribute to an average.

In the same segment the display-change interval reached 7.73 ms at p99 and 12.96 ms at maximum. The full recording reported an ETW overflow warning. The result describes this menu window. Test the reported stutter in flight as a separate case, and record the distribution, mission-clock rate and controller response before attributing it to a particular subsystem.

## Empty GPU resolve rectangles

An optional D3D12 plugin handles Xenos resolve rectangles with zero covered pixels. Its register tests cover pixel copies and color and depth clears. [Build procedure and paired run results](empty-resolve.md)

## Idle achievement notifications

ReXGlue 0.10.1 attaches the achievement notification dialog at startup. Its UI drawer requests another paint while any dialog is attached, even when the notification queue is empty. Over G Fighters now attaches the standard dialog when a notification arrives and detaches it when its queue drains. Game refresh, the D3D12 presentation flags and the external overlay settings remain at their original values. A headless integration check posted two synthetic notifications from a worker thread. Each rendered for about 4.5 seconds, in order, and the idle drawer stopped emitting geometry. A later notification reattached it. [SDK UI redraw loop](https://github.com/rexglue/rexglue-sdk/blob/bd833a2ab69d305f99ddf57418fc38ffec8af93d/src/ui/imgui_drawer.cpp#L436-L445) and [SDK toast queue](https://github.com/rexglue/rexglue-sdk/blob/bd833a2ab69d305f99ddf57418fc38ffec8af93d/src/ui/overlay/achievement_toast.cpp#L27-L53)

These Windows 11 tests used a Ryzen 7 5800X3D, Radeon RX 6950 XT, a 160 Hz desktop and RTSS. The French Contents menu had Scenario selected. The player profile and shader cache had already been used. No external frame limiter was added. The measurements below use application presentation intervals. The flight run used a separate QA executable and a copied profile.

| Scene and conditions | Sample | Median | P99 | Maximum | Other observations |
| --- | ---: | ---: | ---: | ---: | --- |
| Original Contents menu, independent flip | 7,671 presents in 35.2 s | 6.05 ms | 6.37 ms | 9.46 ms | 26.44% below 1 ms. |
| Updated Contents menu, foreground throughout, independent flip | 2,089 presents in 36 s | 16.73 ms | 18.81 ms | 150.43 ms | No intervals below 1 ms and 11 above 50 ms. |
| Updated Area 2 mission 1.1A, F-14A, composed flip | 7,195 presents in 120 s | 16.56 ms | 18.85 ms | 69.19 ms | One interval above 50 ms. The game was in the foreground for the first 107 s. The 99th percentile between display changes was 23.12 ms across the full run. |

In a separate 120-second sample, the updated menu produced no sub-millisecond intervals. It did show 12 clusters of pauses about ten seconds apart. The extra UI paints in the original build can conceal pauses in the game's own output. The menu timing issue remains open. The flight sample did not reproduce that periodic pattern. Both results are specific to the tested scenes.

Video capture can also affect these results. In a separate same-session comparison on the original build, the longest observed interval between display changes was 193.77 ms over 32.2 s with Windows Graphics Capture active and 12.96 ms over 35.2 s after stopping it. The presentation mode switched from composed flip to independent flip. Record capture state in each report.
