# ARCANA on Guition JC3636K718

**v0.17.2:** About links to **https://nova-dynamics.ch** and displays a QR code for the website. The MagicTG Kreuzlingen group link remains available. This build booted successfully on a physical knob after USB installation. Knobs with the existing OTA layout can install [v0.17.2](https://github.com/michaelDengl/arcana-mtg-life-counter/releases/tag/v0.17.2) wirelessly.

**v0.17.1:** fixes delayed wake-wheel input escaping protection while restoring the Life Only screen; includes v0.17.0 saved idle-display controls and experimental NimBLE multiplayer. Install [v0.17.1](https://github.com/michaelDengl/arcana-mtg-life-counter/releases/tag/v0.17.1) through the knob's **Update** menu. No two-knob BLE test or battery measurement has been performed; use the retained Wi-Fi option with earlier firmware. See [implementation and validation notes](../docs/ble-display-candidate.md).

For existing Wi-Fi knobs select **Multi ARCANA > confirm name > Connection > Wi-Fi** before Host/Join. Updates continue to use Wi-Fi regardless of the game connection setting.

This ESP-IDF build uses the existing ARCANA game engine and LVGL 9.2.2 interface on the Guition JC3636K718 360 × 360 knob. It does not use or downgrade to the vendor's LVGL 8 demo. Pin/configuration provenance is in [hardware notes](../docs/hardware.md).

The first hardware installation requires a complete, verified factory flash backup. Build success alone does not establish that display orientation, touch or rotation work on a physical unit.

## Watch faces and brightness (v0.16.3)

The firmware now compiles the same twelve watch-face renderers as the desktop. **Gear > Watch Face** browses three radial pages; selection changes only local presentation and is saved independently of gameplay. **Gear > Brightness** saves after 750 ms without another adjustment, or on leaving the screen. Device-profile storage retains its existing address and alternating checksum slots. Legacy profiles preserve their identity and existing hardware brightness default. Saved 0% brightness can be recovered with a clockwise gesture, restoring 10% without editing life.

Mana Confluence adds a five-mana-colour rim, a small separate Mana font subset, bronze and ivory controls, and static stained-glass panes. All other face previews remain pixel-identical.

No game autosaving has been reintroduced. These additions build for the existing OTA slots; see [wireless installation](OTA.md). New face performance and the settings flow require an on-device check after installation.

## Bring-up status (2026-11-20)

The first image built successfully and was installed on the connected ESP32-S3 revision 0.2 unit after a full 16 MB factory backup was read and verified. Bootloader, partition table and application flash verification all passed. A clean captured boot confirms 16 MB QIO flash at 80 MHz, 8 MB OPI PSRAM at 80 MHz with a passing memory test, successful ST77916/CST816S initialization, and `ARCANA: JC3636K718 firmware started; Wi-Fi disabled`. The application image is 1,274,624 bytes. The private backup and serial logs remain outside Git.

The user confirmed the physical face matches the digital twin. A follow-up update reverses the encoder to the confirmed correct direction and adds battery sensing; the user confirmed battery bars appear after unplugging USB. Captured small-buffer frames took roughly 720 ms, with some menu redraws exceeding three seconds. A further full-frame PSRAM/DMA-staging and queued-touch update was successfully flashed and boot-tested. Two subsequent 45-second serial captures contained no over-100-ms loop warnings; confirmation during active user interaction is still pending. The nine desktop/storage suites pass, including an idle-face test that requires zero redraws after animations settle; they do not substitute for physical latency checks.

## Build on Windows

Install Python 3.11 and Git, then run from the repository root:

```powershell
.\tools\build_firmware.ps1
```

The helper prepares `.tools/esp-python`, installs PlatformIO 6.2.0 / esptool 4.9.0 and downloads the pinned Espressif32 6.12.0 toolchain (ESP-IDF 5.5). Downloads can take several minutes. Artifacts are in `build-esp32/jc3636k718/`; `flasher_args.json` records the individual images and offsets.

To adjust display orientation:

```powershell
.\tools\build_firmware.ps1 -Configure
```

Use the **ARCANA hardware** submenu. The legacy Wi-Fi SSID/password options are no longer used: devices create and discover their own ARCANA networks. Generated `firmware/sdkconfig.jc3636k718` remains ignored because older configurations can contain Wi-Fi credentials. An optional ignored `firmware/sdkconfig.local` can hold defaults on a fresh configuration. Do not publish old credential files or firmware containing private credentials.

## Enable the programming port

The factory firmware exposes USB HID by default. Swipe up from the bottom of the touchscreen to open the main menu, enter Settings and turn HID off. **Exit Settings to save**, then power-cycle the device and reconnect the USB data cable. The vendor getting-started guide also mentions holding BOOT to enter download mode if needed; do not confuse this with a rotary press.

List connected serial devices:

```powershell
.\.tools\esp-python\Scripts\python.exe tools/knob_device.py ports
```

Replace `COM3` below with the actual port. The script confirms ESP32-S3, 16 MB flash and disabled secure boot/flash encryption before any write.

```powershell
.\.tools\esp-python\Scripts\python.exe tools/knob_device.py inspect --port COM3
.\.tools\esp-python\Scripts\python.exe tools/knob_device.py backup --port COM3
```

Backup reads all 16 MB, compares the result with device flash and writes a SHA-256 metadata file. Both files stay in ignored `backups/`. Keep a separate copy before experimenting with other firmware. Backups may contain the factory application's network credentials and should remain private.

## First installation

With a successful build and the verified backup path from the previous command:

```powershell
.\.tools\esp-python\Scripts\python.exe tools/knob_device.py flash --port COM3 --backup backups/YOUR-FACTORY-BACKUP.bin --initialize-storage
```

The installer checks the backup belongs to the same device, clears the new ARCANA NVS/PHY and storage areas, writes the build's bootloader/partition/application images and verifies them. This replaces the factory application and its settings. The external microSD card is not formatted or used.

Subsequent ARCANA updates use the same command **without `--initialize-storage`** to preserve the saved device name and identity. Do not use the first-install flag for routine updates.

To restore the factory software, enter USB download mode and write the verified full backup back to address zero using esptool. This restores the factory application/settings and replaces ARCANA's saved data:

```powershell
.\.tools\esp-python\Scripts\python.exe -m esptool --chip esp32s3 --port COM3 --baud 460800 write_flash 0x0 backups/YOUR-FACTORY-BACKUP.bin
```

The application formats storage automatically only when the entire storage partition is erased. Existing unmountable data produces an error instead of being silently destroyed. Only the multiplayer device profile uses alternating generation/checksum slots for recovery after an interrupted name save. Matches and Undo history remain in RAM and are never saved or recovered. Old match slots are left untouched and ignored.

## Hardware checks after installation

1. Confirm the startup screen and every round menu fit the panel.
2. Start a Single ARCANA game; touch both life halves and hold each for ±5 per second.
3. Confirm clockwise increases and counterclockwise decreases the selected value.
4. Check opponent tiles, both side counters, gear menus, curved Undo and brightness.
5. Change the multiplayer name and confirm it, then restart: startup offers New Game, Update and About, the match is gone, and the name remains available.
6. Test a low brightness setting and restore it with the knob; keep USB attached for recovery.

Serial output at 115200 baud reports boot, PSRAM, storage and driver errors. Capture logs before changing display initialization or flash settings. Do not flash a similar-looking K518/W518/Waveshare board configuration.

## Experimental wheel responsiveness

The current worktree contains the accepted wheel-response v2 build, now installed and boot-verified on all five devices. It was first tested on the owner's fifth device (`44:bd:8d:6f:22:08`), then rolled out to the other four after the owner's approval. The shipped Wi-Fi build and its source/configuration are preserved in ignored `build/releases/wifi-stable-20261120/`, with SHA-256 metadata; the shipped application hash is `8e7bc6a197c59a54490fb8ea350c679220d7b036cdd58032ebf5f0336e79ded0`. The previous experiment remains in `build/experiments/wheel-v1/` (application hash `a203311bd9048a4eec0e8c43f32ed79cf18503edf9113e3a0750aaaa80f3d790`). Current binaries are in `build/experiments/wheel-v2/` (application hash `fa1487d052e1bb2d9316b0a656badab29a8d7928210a0f9560fde16b06b03856`). A normal build produces the current experimental code.

The first 30-second Single ARCANA wheel capture on the shipped build measured mean frame time 526.110 ms (502.232 ms drawing, 23.877 ms LCD transfer), mean wheel queue time 117.388 ms and maximum 353 ms. History recording averaged 0.236 ms. That capture's queue timestamp used the latest pending pulse and can underestimate the oldest waiting click; the experiment fixes batching and timestamps it consistently under a short critical section.

The experiment clips the curved Undo control to its visible bottom sector, removes inherited theme animations from its invisible hit target, and avoids restyling unchanged life fonts, warning colours, counter badges and Undo arcs/glyphs. Accepted game-state wheel edits request an immediate LVGL refresh instead of waiting for the normal refresh timer. GPIO direction, pulse debounce and game/Undo semantics are unchanged. Native redraw tests measure 36,825 pixels for the first edit and 16,092 for a steady edit, with budgets covering Undo expiry too. Incremental/full redraw comparisons exercise digit/font boundaries, zero/negative life and lethal markers. Central life/warning pixels must agree within one colour level; only tightly bounded antialiasing variation is allowed on rotated tab/Undo edges. All ten native suites and eight host-tool tests pass; flash verification and clean boot passed on device five.

The follow-up physical capture measured mean frame time **117.076 ms**, versus 526.110 ms before (about 4.5 times faster in these recordings). Mean drawing time was 111.507 ms and LCD transfer 5.568 ms; the median frame was 130.084 ms and p95 215.332 ms. The detected-pulse-to-final-transfer metric averaged 230.084 ms, with p95 344.649 ms and maximum 421.656 ms. Mean transferred area was 16,554 pixels per handler call. The owner reported that the wheel already feels close to instant and this version was retained. These were natural manual-use captures, not a replay-controlled benchmark; frame percentiles use the latest 128 samples, and the old queue metric uses different timestamp semantics. Rendering remains the largest measured cost, so this is not a claim of sub-50 ms input latency or 60 FPS.

Profiler output now includes `wheel_to_frame` in microseconds, measured from the oldest detected pulse in an accepted game-state batch to completion of the final LCD transfer, and `frame_pixels` in pixels transferred per handler call. The timing excludes physical motion before the existing pulse detector recognizes a click and the panel's subsequent scanout; it is not an optical measurement. Ignored/clamped inputs and the initial wake gesture do not produce `wheel_to_frame` samples.

The v2 follow-up used a controlled live/frozen/live Undo-caption comparison: four warm-up edits and 24 measured edits per phase, with the same scratch Commander state and +1 sequence. The original model (including history, recent Undo and revision) is copied in RAM and restored; multiplayer and ordinary profiling must be inactive. Screen setup is excluded from the per-edit timings. On v1, the live phases both averaged 142.609 ms; freezing only the caption reduced the mean to 80.984 ms. This includes drawing and LCD transfers, not physical wheel input.

V2 hides the nonvisual lookup label so its full 156px box cannot invalidate the caption, and only changes a glyph's hidden flag when visibility actually changes. It also avoids a redundant whole-screen rounded child mask on the main life face, whose artwork already fits inside the 348px outer ring. Menu masks remain enabled. The native main-face comparison found no artwork outside the round display; colour differences were bounded to four levels due to changed compositing. The repeated hardware comparison measured live means of **67.653 / 67.695 ms**, and frozen 43.679 ms (about 2.1 times faster with the curved caption updating). Native redraw budgets measure 36,825 pixels for the first edit, 11,637 for a steady edit and 19,947 for Undo expiry. All ten native suites and ten host-tool tests pass; flash verification and clean boot passed. An idle 30-second capture after this test reported zero frames and zero input events, with the normal 10% dimmed backlight.

The subsequent active 30-second wheel capture contained 214 accepted wheel batches and 359 drawing passes. Mean frame time was **46.683 ms**, drawing 43.363 ms and LCD transfer 3.320 ms. Detected-pulse-to-final-transfer time averaged **96.956 ms**, with median 91.643 ms, recent p95 154.637 ms and maximum 194.641 ms. Mean transferred area was 8,484 pixels. Compared with the previous manual capture, mean frame time fell from 117.076 to 46.683 ms and mean pulse-to-transfer time from 230.084 to 96.956 ms; these natural-use captures are not identical input replays. Frame time is not equivalent to full-screen FPS, and the input timing excludes pre-detection motion and panel scanout. The owner confirmed this version feels faster and the display looks right, so v2 was retained on the personal device. The four sold units subsequently received the same v2 image, with matching-backup validation, flash verification and clean boot checks on every unit; profile storage was preserved.

Run the repeatable comparison from a Single ARCANA main life face with:

```powershell
.\.tools\esp-python\Scripts\python.exe tools/profile_device.py --port COM7 --benchmark-undo --output build/render-comparison.log
```

Leave the controls still until `RENDER_BENCH END restored=1`. The USB-only `b` command runs this comparison. Captures from busy/unsupported views are rejected. Normal `profile_device.py` use still records 30 seconds of real activity.

## Multiplayer and remaining hardware work

Single ARCANA leaves the radio off. **Multi ARCANA > Host** creates an open 2.4 GHz access point after confirming the format and player count. Its SSID is `ARCANA-<name>-<MAC suffix>`; Join scans for these networks asynchronously and displays their host names. Tapping a host connects, obtains an address using DHCP, discovers the current game and joins it using the existing acknowledged UDP protocol. No home-network credentials are stored or needed. Back from browsing or the lobby stops the radio; leaving/ending a match also stops it. The station uses modem power saving while connected. Desktop instances can participate when their computer joins the host's ARCANA Wi-Fi network; normal desktop-to-desktop sessions still use the computer's LAN.

Hosts can start with empty slots and accept late players. Wi-Fi supports up to ten connected clients plus the host; a twelve-seat 2HG match can still include manually tracked seats. This remains a passwordless local-game protocol, with no host migration. All devices need the direct-Wi-Fi firmware; earlier builds with no configured SSID cannot discover it. Native regression coverage includes delayed radio readiness, resolving the live game ID after connection, late joins, life synchronization and cancelling joins; real radio verification is recorded separately below.

USB network diagnostics use a separate RAM-only test profile/match, leaving the saved device name untouched. Uppercase commands: `N` status, `H` test host, `J` scan, `K` join the USBHOST test lobby, `S` start, `A`/`D` change test life by one, `Q` stop and return to the UI. Diagnostics reject entry during a real match and stop automatically after two minutes. Lowercase performance/RGB commands retain their existing meanings.

On 2026-11-20, device five (`44:bd:8d:6f:22:08`) received this firmware after a verified 16 MB factory backup. Flash verification and clean boot passed. USB diagnostics successfully created `ARCANA-USBHOST-6f2209` at `192.168.4.1`, opened the UDP sockets, started a partial test match, completed an asynchronous scan, and stopped the radio with no reported error. The scan found no updated ARCANA hosts. The other four devices subsequently received the same firmware, with matching-backup validation, flash verification and clean boot checks on every unit; their profile storage was preserved. All five devices are now updated. Those installation checks connected devices one at a time. The user subsequently reported a flawless five-device multiplayer playtest on 2026-11-21, with approximately four hours of battery runtime. The later power-management candidate is being tested only on the owner's fifth device; cross-device automatic-sleep compatibility and its new battery runtime are separate outstanding checks. Ten native suites and ten installer/profiler/RGB tool tests pass.

With two updated devices connected by USB and no live match running, the repeatable radio test is:

```powershell
.\.tools\esp-python\Scripts\python.exe tools/test_device_network.py --host-port COM7 --guest-port COM6 --output build/wifi-pair-test.log
```

Use the actual two port numbers. This tests scanning, DHCP, starting before a guest arrives, admission, the roster and life synchronization in both directions. It stops both diagnostic sessions on exit.

Battery sensing uses calibrated ADC1 channel 5 (GPIO6), averaged over 16 samples every five seconds. The schematic's 10k/10k divider measures VCC_OUT rather than BAT+ directly. While running on battery, the icon uses an approximate single-cell voltage-to-charge curve; it is not a fuel gauge. When PC USB power is detected, or the measured supply exceeds 4.35 V, the battery outline shows a USB-power symbol instead of a fabricated state of charge. The charger STAT signal is not routed to a readable MCU pin, so charging-versus-full cannot be reported reliably. Power-bank detection can also be ambiguous when its supply voltage overlaps the battery range.

The hardware-verified encoder direction is clockwise increase / counterclockwise decrease. A separate CPU1 touch task buffers press/release events while CPU0 draws, with an eight-millisecond deadline on I²C reads. LVGL draws into a full-frame PSRAM buffer; a small internal DMA buffer transfers rows to the LCD. Shared UI refresh is driven by state changes, including network life updates and Undo expiry, rather than repeatedly invalidating an idle screen. No background match-save task is created. The UI and game state stay on CPU0; only explicit device-name changes write the small profile. Menu transitions use one fading cover, and the curved Undo label reuses glyph objects. Disabled Undo dims only its glyphs rather than creating an alpha layer for its large touch target. The RGB ring provides a dim battery-status light. Vibration, speaker and microphone are not controlled.

### Idle dimming

After five seconds without touch or wheel activity, backlight PWM drops to 10%. Settings already at 10% or lower stay unchanged. The first touch wakes the display to the selected brightness and consumes that entire gesture, including its release, so waking cannot edit life or activate a button. A wake rotation also consumes its initial burst; another rotation after 250 ms without pulses works normally. Holding the touchscreen keeps it awake. Dimming preserves the match and networking. The power-management build can additionally sleep between events while retaining the same backlight output. Battery-runtime improvement requires a separate discharge test.

### Idle power management (power-v1)

**Validation status:** the revised USB-guard image is installed and independently flash-verified on the owner's COM7 device. It boots with matching ELF hash, passes USB power diagnostics, and starts/stops an access-point test match and asynchronous Wi-Fi scan. The three common fuel-gauge address probes received no ACK. Battery-side sleep/wake is verified: the cumulative SDK counters reported 3,353 successful sleep intervals and 68 GPIO wakeups, and the user confirmed that wake, wheel and menus work normally on battery. USB reconnect restored the external-power guard without resetting the device. Cross-device sleep remains untested because only one unit is available. The first candidate lost USB responsiveness after idle and is not accepted. The accepted wheel-v2 rollback remains archived.

The owner-device power candidate enables 80-240 MHz dynamic frequency scaling and FreeRTOS tickless automatic light sleep. UI work, including drawing and input dispatch, holds a 240 MHz CPU lock. The main task blocks until input or the next LVGL/network deadline rather than polling every five milliseconds. Touch and the knob use GPIO wake notifications; the proven three-millisecond direction-pulse filter runs only during a pulse, and touch polling runs only while an interaction is active. GPIO1, GPIO2 and GPIO7 are wake sources. No deep sleep is used, and in-memory matches remain available.

Backlight PWM uses the sleep-compatible RC_FAST clock and KEEP_ALIVE output. Selected brightness, five-second dimming, wake-gesture suppression and the existing 5/255 RGB-ring policy are unchanged. There is no optional battery mode. External/USB power deliberately inhibits automatic light sleep so flashing and diagnostics remain accessible. The revised candidate additionally uses the charger-rail reading, because host USB selective suspension can stop the SOF-based connection indication. Unknown battery readings conservatively inhibit sleep. The access-point host also holds a no-light-sleep lock; it can still scale CPU speed while idle. Connected clients use the Wi-Fi driver's existing modem power saving and coordinated automatic sleep.

Idle state/keepalive messages run once per second instead of every 300 ms. Edits broadcast/send on the next application tick; unacknowledged commands retain the 300 ms retry. Unchanged heartbeats no longer request a UI refresh. The version-1 packet format, six-second timeout, late joins and open-seat tracking remain compatible with the preceding Wi-Fi firmware. Native tests cover quiet idle, immediate edits after a heartbeat/ack, loss/retry, replay handling and team synchronization.

Read the sleep counters and read-only battery-gauge address probes without resetting the device:

```powershell
.\.tools\esp-python\Scripts\python.exe tools/power_diagnostics.py --port COM7 --output build/power-status.log
```

For actual sleep testing, leave the switched-on device unplugged for 30 seconds, reconnect, and run the command before the 80-second battery sample history expires. `POWER` reports cumulative successful sleep callbacks, GPIO wakeups and SDK sleep-interval time (including entry/exit overhead); these counters are not a current measurement or an exact duty-cycle measurement of the host-observed unplug window. USB-connected idle alone cannot validate battery sleep. Serial `e` prints power diagnostics and `i` probes common gauge addresses and dumps battery samples. These are diagnostics, not current measurements or a battery-runtime prediction.

The 2026-11-22 owner-device battery test recorded supply samples of 4,126-4,136 mV, mapped by the existing curve to an approximate 92-93%. Reconnected USB produced 4,456-4,462 mV and activated the external-power guard. The ring driver reported no error. The raw capture is retained locally at `build/power-v1-battery-diagnostics.log`; the accepted owner checkpoint is `build/experiments/power-v1-usb-guard/`, with image hashes, source and validation metadata. The other four devices remain on wheel-v2. A full discharge test is still needed to measure improvement over the reported four-hour baseline.

### Battery-status ring

The vendor demo describes a 13-pixel WS2812 chain on GPIO0; the supplied base schematic shows 12 RGB packages (LED2-LED13). A partial ring reported on the physical unit means the actual hardware/transfer behavior needs checking rather than assuming the demo count covers it. ARCANA now sends a uniform frame for up to 64 pixels, with surplus data harmlessly forwarded beyond shorter chains. Green is 5/255 (about 2%) at/above 30% battery; red is 5/255 (about 2%) below 30%. With USB power or an unavailable battery reading, the ring is off. After unplugging USB, allow up to five seconds for the next sample. It follows the same coarse voltage estimate as the battery icon.

The ring is steady, updates only on colour changes and uses the built-in RMT DMA driver with the complete frame buffered; the peripheral is disabled after each transfer. It does not animate, and its brightness is independent of the screen. Driver errors leave gameplay running and are reported in the serial log. Extra LED current and battery-runtime impact are unmeasured.

### Diagnosing a dark ring

Connect USB and close other serial monitors. Read the driver's current state and recent battery samples (kept only in RAM, about 80 seconds):

```powershell
.\.tools\esp-python\Scripts\python.exe tools/ring_diagnostics.py --port COM3 --action status --output build/ring-status.log
```

For a direct visual test that bypasses the battery/USB policy, use `--action dma` or `--action rmt`. Each forces uniform red at 5/255 for 15 seconds while USB stays connected, then automatically returns to battery mode. `dma` uses the buffered transfer; `rmt` uses the original non-DMA method with a larger hardware buffer. The last tested backend remains selected until restart. `--action auto` cancels a test early. Diagnostics do not change the match or profile.

Observe the LEDs during each test: a successful transmit call does not prove that the physical LEDs lit. `failed=1` or a non-OK error identifies a driver failure; `rgb=0,0,0` with `available=0 external=1 usb=1` is the intentional USB/off policy. To inspect unplugged sensing, unplug USB for at least six seconds, reconnect and run `status` before the 80-second sample history expires. Samples include supply millivolts, USB detection, estimated percentage and availability. Serial `d` dumps status/history, `t` tests DMA, `u` tests non-DMA and `x` returns to automatic mode.

### On-demand performance capture

With the device connected by USB and no other serial monitor open:

```powershell
.\.tools\esp-python\Scripts\python.exe tools/profile_device.py --port COM3 --output build/profile-active.log
```

Use the touch menus and wheel during the 30-second capture. The helper does not restart the device. Firmware records timings in fixed-size memory and prints a summary at the end; normal operation has no per-frame logging. Serial `p` starts a capture and `r` ends it early.

Match saving has been removed permanently; the old `--saving` option and serial saving switches no longer exist. Every new capture reports `match_saving=removed`. The separate device-name profile remains persistent.

The summary reports microseconds for UI refresh/build, LVGL handler calls that flush a frame, LCD flush time (including copy, byte swap and DMA wait), and touch/wheel queue delay. `draw_work` is frame-handler time minus LCD time, so it also includes input callbacks and LVGL scheduling; it is an approximation of drawing cost. `history_record` measures recording an accepted edit's history/recent snapshots; it excludes the caller's initial next-state copy. `undo_apply` measures normal/grouped Undo and Redo in the model. `undo_label` measures layout/update of the curved Undo letters, excluding their later rasterization. These probes read the clock only during a requested capture.

Queue delay starts when the sampler detects an event, not at physical contact. Average and maximum cover the whole capture; p50/p95 use the latest 128 samples. Counts of zero mean the operation did not occur. Backlight requested/output levels are included. Use separate life/Undo and menu captures to distinguish model/history cost from rendering.

On 2026-11-20, the installed firmware passed a 30-second idle capture on the user's unit: no UI refreshes, LCD transfers or background saves occurred, and the backlight reported selected 70%, output 10%, dimmed. This verified the earlier idle path. Battery runtime and physical wake behavior require separate checks.

Subsequent mixed-interaction captures on the same unit measured average frame-handler time of 624 ms with saving enabled (22 frames, worst 2.87 s), versus 372 ms with automatic match saving paused (49 frames, worst 621 ms). The paused capture reported zero background saves with Undo still enabled; the user reported much smoother interaction. These were manually operated captures with different interaction mixes, not identical replay benchmarks. LCD transfer averages stayed around 22-27 ms. Those earlier captures motivated removing match saving and adding separate Undo probes and cheaper rendering. A menu-only attempt also recorded saves, so it does not establish rendering performance without storage activity.

The firmware CMake configuration explicitly follows Windows directory links when collecting LVGL sources. This handles Component Manager's cached source junctions, which LVGL 9.2.2's original glob can otherwise omit and cause missing-symbol linker errors.

### Battery-sensing investigation, 2026-11-22

The supplied base schematic `JC3636K718_P.pdf` identifies U1 as **ETA6003Q3Q**, a charger with dynamic power-path control. It has no I2C charge-percentage or coulomb-counter interface; see the [manufacturer datasheet](https://www.eta-semi.com/wp-content/uploads/2022/03/ETA6003_V2.7.pdf). No dedicated fuel gauge appears in the supplied main/base schematics. GPIO6 senses VCC_OUT through a 10k/10k divider, so its calibrated voltage remains an approximate indicator affected by system load and the power path. More ADC samples cannot turn it into a fuel gauge. Charging and fully charged cannot be reliably distinguished through this connection.

The power candidate includes address-only, read-only probes for common gauges at 0x36, 0x55 and 0x62. On the owner device, all three probes received no ACK. This supports the schematic findings but does not prove that every possible gauge is absent. Reliable state-of-charge reporting would require either battery-specific discharge calibration (still an estimate) or an additional gauge wired to the actual cell. No external hardware modification or unsupported gauge driver is part of this update.


## Wireless updates and contact information (0.15.x)

The OTA-enabled build adds Update/About below New Game, Nova Dynamics website and MagicTG Kreuzlingen WhatsApp QR links, password-protected phone/browser Wi-Fi setup, and stable GitHub Release checks. Two firmware slots occupy the old application region while NVS and device-profile storage stay at their previous addresses. The guarded installer initializes OTA boot-selection data; it does not erase names or home Wi-Fi settings on routine updates. Earlier non-OTA devices need one USB installation before wireless updates are available.

See [OTA setup, versioning and draft-release workflow](OTA.md). The owner device successfully provisioned home Wi-Fi and installed the published v0.15.1 image from GitHub over Wi-Fi on 2026-11-22. Monitoring confirmed `ota_1` boot, valid image state and a subsequent check reporting the release as current. The owner's accepted power firmware and backups remain archived separately.

Version 0.15.2 adds a setup-network Wi-Fi QR and a setup-only captive portal. Local DNS and HTTP redirects allow the phone to offer its Wi-Fi sign-in page after joining; the manual address remains available. Back, timeout and saved credentials close the setup services, and setup DNS options are cleared before later multiplayer use. Native tests cover QR rendering/navigation and bounded DNS parsing. Phone-specific automatic opening and the new release's hardware behavior remain for the owner's later wireless test.

### Black display perimeter (0.15.3)

The LCD is cleared across all 360 x 360 addressable pixels before the first frame/backlight. Every DMA transfer then zeros pixels outside radius 176 around the panel's pixel-center midpoint, leaving the 348px outer teal ring and its antialiasing intact. Precomputed row spans clear only the outside portions; rectangles entirely inside the face bypass the work. This does not add an LVGL clipping layer or change the battery-status LEDs. The digital twin applies the same perimeter policy.

All eleven native suites pass. Regression coverage checks full and partial RGB565/XRGB8888 buffers, row padding and guard bytes; all captured UI screens require a black perimeter. Existing incremental/full redraw comparisons and wheel redraw budgets still pass, with 36,825 / 11,637 / 19,947 pixels for first edit, steady edit and Undo expiry. The owner device received independently verified firmware and booted v0.15.3 with valid image state; its startup log confirms the complete LCD clear, and saved home Wi-Fi still connects. On 2026-11-23 the owner confirmed that the unwanted edge lights disappeared and the edge is clean.
