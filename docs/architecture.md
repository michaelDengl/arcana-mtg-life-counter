# Architecture

`src/core` contains the game state and rules, without SDL, LVGL or Windows dependencies. A snapshot includes one local player, independent incoming commander sources identified by stable IDs, separate own-commanders' cast counts, tracker visibility, a poison limit and manual status markers.

Every accepted game mutation records one atomic before/after snapshot and a description. Undo/Redo uses a cursor, truncates redo on a new branch, and retains at most 64 actions. A commander-damage receipt updates source damage and optional life loss together, after validating both operations. Invalid input and integer overflow leave state and history unchanged. Full snapshots favor simple, deterministic correction; a future networking adapter can wrap the same operations with versioned action IDs.

`src/ui` renders all device views in a 360 x 360 circular LVGL object. Event bindings belong to AppUI and remain stable while a view exists. Callbacks request navigation; rebuilding occurs on the next update, after event dispatch. Number changes update labels instead of rebuilding a whole view. List rows rebuild after configuration/history changes. A revision counter connects state changes to UI refreshes.

`src/platform/IHardware.hpp` exposes accumulated rotation, touch and logical power. `simulator` implements it, transports LVGL partial draw buffers using their reported stride, and renders desktop-only controls in SDL. No embedded pin mappings or drivers are provided. The desktop framebuffer remains host-only; the shared UI and generated numeric font can be compiled for an embedded display adapter.

Damage amounts are drafted locally in the UI and committed explicitly. The knob changes the current value or scrolls a list; visiting a nonnumeric view never silently adjusts life. UI tests exercise actual LVGL pointer dispatch against the same adapter, including scrolling, name editing and power state.

`GameSnapshot` also records the selected pod size and whether the match is active. Starting a game creates sources for exactly the configured opponents. Ending clears Undo/Redo and stores an inactive marker. Startup always precedes the dial; Continue is enabled only for an active recovered match. P1 edits local life; opponent tabs select an incoming commander source. A linked-damage ledger prevents corrections from refunding life that was never deducted.

`Serialization.cpp` uses vendored nlohmann/json 3.11.3 to write a versioned schema including state, 64-entry history and cursor. Loading validates integer bounds, names, counters, source IDs, owners, linked damage, history continuity and cursor consistency before replacing any state.

`SessionPersistence` depends on `IStorage`, coalesces edits after 600 ms, and immediately persists lifecycle changes. Save failures remain dirty and are retried. Desktop FileStorage writes a sibling temporary file and uses atomic replacement (MoveFileExW on Windows). The SDL adapter selects the per-user preference path; tests use isolated files. Shutdown/power/restart flush pending changes. An ESP32 storage adapter can use the same JSON and debounce service later.

Wireless communication is not implemented.

## Recent edit groups and shortcut state

Shortcut choices are part of GameSnapshot and never move or reset counter values. The shared UI handles short taps and long holds separately; long holds wait for release before rebuilding, preventing release events from accidentally selecting a new row. Editors and the picker use a circular popup with a centered scroll area.

The game engine accepts a monotonic action time from the UI. Recent edits retain before snapshots for the rolling five-second window independently of the 64-entry advanced history, so rapid edits beyond that history limit can still be grouped. Undo Recent restores the first eligible before-state and records a synthetic, undone history entry covering the group, allowing atomic Redo. Labels use overflow-safe signed-delta formatting. Old timestamps are not restored from disk, so stale sessions never reactivate quick Undo; the normal saved history remains available.

Schema 3 adds the Shield tally and two shortcut selections. Schema 1/2 loads append zero-valued new counters and default shortcuts, consistently across all stored history snapshots.

RandomTools uses IRandom for unbiased coin and D20 draws. The desktop adapter uses std::random_device to seed mt19937 and uniform_int_distribution. Tests inject a fixed sequence, including ties. Only tied leaders reroll; pathological ties are bounded and never resolved with an arbitrary winner. Random-tool results are ephemeral and do not alter match state.

Life-half pointer events distinguish taps from held edits. The first repeat becomes due at 1000 ms; later repeats use elapsed time rather than frame counts, combining missed beats into one safe signed adjustment. Release flushes a due final beat without an extra +/-1. Region changes, view changes and power-off cancel the hold. Gear and overlay controls sit above the halves and do not trigger life input.

Format presets are explicit GameFormat values in each snapshot. Opponent life and fixed seat-to-team assignments are stored with history. In 2HG, local life mutations synchronize P1/P2 and enemy life mutations synchronize the selected pair (P3/P4, P5/P6, etc.) atomically; poison belongs to the local team. CommanderSource records include the recipient (P1 or P2) so damage is never combined across teammates. Only Commander/2HG create incoming commander sources. Schema 5 supports twelve seats, migrates schema 4 six-seat arrays, and validates format bounds, legal seat counts, fixed team mappings, shared-life consistency and recipient bounds; old schemas migrate to Commander.

DeviceProfile stores an independent versioned identity through IStorage. AppUI accepts optional profile/multiplayer services; the default keeps existing single-device callers working. New game branches through Single/Multi before format setup. Device profiles never enter GameSnapshot/history.

Multiplayer owns the live roster, discovery list and host/client state. IPeerTransport supplies bounded datagrams; the desktop LanTransport uses nonblocking IPv4 UDP, multicast discovery and ephemeral gameplay endpoints. A host assigns stable seats by device identity and admits up to the selected capacity. Only each member's endpoint may submit that member's edits. Sequence numbers and acknowledgements deduplicate retries; only the front command is in flight and rapid edits merge into an unsent command. Full host snapshots periodically reconcile clients, rebasing pending edits so concurrent 2HG changes converge. Team life and poison propagate to teammates; other counters stay local. GameState::synchronizeLocal records remote team corrections coherently without starting a five-second local Undo group.

Live network state is not persisted. A still-running host can reconnect a device with its saved identity; a host restart creates a new lobby. Single-device saves remain valid. Host migration and an ESP32 network adapter are not implemented.
