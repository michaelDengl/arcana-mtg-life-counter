# Development

Dependencies: C++17, CMake >=3.24, Git, SDL 2.30.11, LVGL 9.2.2, and vendored nlohmann/json 3.11.3. LVGL/SDL versions are pinned in CMake FetchContent. LVGL's own SDL driver is disabled: our SDL transport supports the surrounding simulator controls while sharing the actual LVGL device UI.

Official LVGL desktop guidance: https://lvgl.io/docs/open/9.2/integration/ide/pc-simulator.html . LVGL supports desktop simulation and interchangeable display/input drivers. This project supplies those small adapters explicitly.

CMake supports standard FetchContent source overrides, e.g. `-DFETCHCONTENT_SOURCE_DIR_LVGL=D:/path/to/lvgl` and `-DFETCHCONTENT_SOURCE_DIR_SDL2=D:/path/to/SDL`, for offline source trees. Use a new build directory when changing compilers. CI builds the simulator and runs core, saved-game/storage, LVGL interaction, SDL smoke and separate-process recovery tests on Windows 2022.

The lightweight core test executable uses explicit checks, which remain active in Release. No hardware access or timing-dependent assertions are required.

## Verified locally

Built with LLVM-MinGW 20250305 (Clang 20.1.0) and CMake 3.31.6. All five CTest suites passed. The normal SDL Windows window was launched, reported responsive, accepted a window-close request and exited with code 0. The provided build and test batch scripts were executed successfully. The generated smoke screenshot was visually inspected. Visual Studio/MSVC compilation is configured in CI but was not run locally because the installed Build Tools lacked the C++ workload.

## Numeric font

The checked-in `src/assets/life_digits.c` is an 80px, 4bpp digits-and-minus subset generated from LVGL 9.2.2's bundled Montserrat-Medium.ttf using `lv_font_conv` 1.5.3. Builds do not require Node or font conversion. To regenerate after fetching LVGL:

```powershell
npm exec --yes --package=lv_font_conv@1.5.3 -- lv_font_conv --font .deps/lvgl/scripts/built_in_font/Montserrat-Medium.ttf --range 0x2d,0x30-0x39 --size 80 --bpp 4 --format lvgl --no-compress --lv-include lvgl.h --output src/assets/life_digits.c
```

The Montserrat license is included beside the generated font. The UI falls back to smaller native fonts for long totals, without scaling bitmap text.

## Storage tests

The SDL tests always use isolated save files in the build directory. `process_recovery` starts one real simulator process to create a four-player match at 49 life, closes it, then starts another process and clicks Continue to verify recovery. Neither process touches the normal per-user save. UI tests also cover explicit End Game disabling Continue, and storage tests verify the finished marker survives file replacement.

## Interface and symbol verification

Version 0.5 adds the opponent-only tabs, ?5 life controls, configurable side trackers, round popup/picker, five-second grouped Undo and game tools. All five suites cover these changes. Glyph provenance and exact font commits are recorded in [symbols.md](symbols.md); builds use the checked-in generated font and do not need a font converter.

Version 0.5.1 refines the visual controls: annular side rocker buttons, life and popup buttons clipped to their circles, uniform teal token borders, and a centered gear without the LIFE caption. Existing engine, save, touch, SDL and recovery suites pass. Custom circular hit testing uses LVGL 9.2's `lv_obj_event_private.h` payload definition; revisit that narrow include when upgrading LVGL.

Version 0.5.2 adds continuous bright-teal inner/outer dividers, splits the life total around a central gear, adds the red heart, corrects side rocker colors, narrows Undo and renders complete root-level count badges. Existing five suites pass; rendered screenshots were inspected. No save schema change is needed.

Version 0.5.3 removes the inner life buttons and trims the added outer divider to the two upper corner arcs and lower arc, with horizontal inward joins. Life is adjusted through rotary input; outer counter controls remain. All five suites pass, including the SDL smoke/recovery flow exercising a counter touch plus life keyboard input.

Version 0.5.4 smooths the heart with a single antialiased Font Awesome glyph, uses the two-pixel inner-ring gap for the outer dividers, derives horizontal endpoints from circle intersections, extends the center divider to both rim edges, colors the upper/lower divider green/red, and centers outer rocker symbols in their visible annular fields. The title is white. Existing five suites pass.

Version 0.6.0 adds tap/hold input on the entire colored life halves: tap +/-1, hold +/-5 every 1000 ms with no extra release tap. Shared LVGL timing handles delayed frames, cancellation and power state. Deterministic interaction checks cover both sides, successive hold beats, release and power-off. Colored arc segments are clipped at the exact center-divider column to align their endpoint. Gear now has four options including End Game, no Trackers button, and a centered Close. All five suites pass.

Version 0.7.0 adds white skull/reason warnings for life, poison and commander damage, right-side units with leading zero, tangential CMD captions and full annular opponent hitboxes for two through six players. A Montserrat subset at 11px keeps five opponent captions legible. The center divider stops at the colored inner segments, whose actual starts meet the divider at top and bottom. UI checks cover 00?11, warning recovery and wedge edges for every player count. Battery behavior is unchanged.

Version 0.7.1 adds the horizontal battery indicator above the life title with all six charge bands and charging/unknown states. Simulator battery defaults to an explicitly illustrative 75%; hardware reporting is provided through IHardware. Boundary renders and touch-through are checked in the UI suite. Opponent captions now contain only Pn and damage, preserving rotation and full wedge hitboxes. No power-saving changes are included.

Version 0.8.0 replaces the settings list with five radial color sections and white icons, including a native sun icon. Central grey Back on black returns to the life face; tool results return to settings. Brightness supports the complete 0?100 range through slider, encoder and +/- with simulator dimming and an IHardware backlight hook. Tests cover limits, slider, buttons, one-step return and preservation of game stats. Hardware driver mapping and persisted brightness are not implemented.

Version 0.9.0 makes shortcut and badge taps open a seven-sector counter picker directly, with retained counts and selected checkmark. All gear tools share reusable radial sectors, polar hit testing and central Back. Reset/end confirmations use question/yes/no sectors; coin uses result/reflip halves; starter rolls use winner, individual results and reroll sectors; brightness uses a top slider and lower minus/plus sectors. The compact 11px Montserrat font now includes printable ASCII for counter names and roll values. UI tests cover direct taps, center cancellation, each counter sector near its outer edge and brightness controls.

Version 0.10.0 adds a 180 ms content fade-in on navigation, throttled to 33 ms updates with quadratic ease-out. The frame remains opaque. Rebuilds cancel stale transitions, power-off completes/cancels them, and stat updates do not start new animations. UI tests check initial/intermediate/final opacity, the stationary frame and instant stat refreshes. Rotation is intentionally deferred until hardware rendering is verified.

Version 0.11.0 makes all opponent commander-damage screens radial, reusing polar hit testing, fade transitions and central Back. Explicit plus/minus and chain icons distinguish damage edits from life-link mode. Source cycling, independent incoming damage and linked/unlinked corrections retain existing logic. Opponent tab edge tests cover all player counts and now return through central Back.

Version 0.12.0 adds radial format and player selectors, format-specific starting life and opponent life editing, explicit 2HG teams with 40 shared life and poison 15, separate commander recipients and a recipient switch. Startup says LIFE COUNTER. Format setup cancels without discarding the current match. Save schema 4 migrates versions 1-3. Core/storage/UI tests cover all presets, shared-life undo, independent recipient ledgers, invalid saves, Continue, reset and cancelled setup. The 40-life Commander 2HG preset follows the requested configuration.

Version 0.13.0 lets Commander 2HG select 2-6 teams (4-12 players). Each setup step changes one whole team. The model synchronizes every pair independently; snapshots, history and validation support twelve seats. Schema 5 migrates previous array sizes. More than five enemy tabs use a 120-degree arc with compact labels and full wedge hitboxes. 2HG starter rolls choose among teams, keeping the result menu readable. Tests cover every team count, upper/lower setup bounds, twelve-player touch selection, shared life, reset/Undo/Redo, schema 4 migration, Continue and six-team starter rolls.

Version 0.14.0 adds Single/Multi radial selection, a compact name keyboard and independent atomic device profiles. Host/Join uses real LAN multicast discovery and passwordless admission. The host coordinates acknowledged life/counter deltas, retries, coalesced bursts, live named tiles, read-only remote views, shared 2HG life/poison and timeout/rejoin behavior. Eight suites include real sockets and independent host/client processes on dedicated test discovery ports 42689/42690, preserving normal lobbies on 42688. UI checks cover keyboard input, profile reload, discovered games, guest waiting, host start, named life updates, read-only peer editing and identity retention through a new single match. Live lobby recovery across host shutdown and ESP32 Wi-Fi drivers remain unimplemented.

Hosting can start with fewer devices than configured, including one device and partial 2HG teams. The host retains explicit unclaimed seats, can manually edit their life, and broadcasts them as dim placeholders. Late arrivals claim a stable free seat and inherit its canonical life/counters without disturbing others; reconnecting identities retain their own seats. Protocol/UI checks cover starting 2/4, late team joins, placeholder replacement and manually tracked life transfer.
