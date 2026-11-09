# Commander trackers

Rules checked against the [official Comprehensive Rules](https://magic.wizards.com/en/rules), effective [November 6, 2026](https://media.wizards.com/2026/downloads/MagicCompRules%2020261106.txt).

- **Commander damage:** 21 or more combat damage from one particular commander is the normal loss threshold (903.10a). Partners are separate (702.124d). The app never sums multiple sources to determine this warning. Renaming or changing control must not merge source identities.
- **Damage and life:** the ordinary shortcut subtracts life once, atomically with the damage receipt. Record-only mode handles already-adjusted life and exceptional damage results; the user records poison or other effects separately. The primary opponent dial supports signed corrections: unlinked damage is removed first, then linked damage with a corresponding life refund. Undo reverses the precise latest transaction; the detailed receipt editor accepts only positive incoming damage. Prevented damage should not be recorded as damage dealt.
- **Poison:** the default limit is 10 (122.1f). The configurable threshold supports variants; it is a warning setting, not a full variant rules engine.
- **Tax:** additional generic mana for the next command-zone cast is twice that commander's previous command-zone casts (903.8). A first cast has no additional tax; after recording it, the next cast has +2. Separate commanders do not share cast counts. This excludes other cost increases/reductions.
- **Optional counters:** energy, experience and rad are nonnegative player counters. Rad's inherent trigger occurs at the beginning of the player's **precombat main phase**, not upkeep (729.1). The app displays a reminder without resolving mills, life loss or counter removal automatically.
- **Spells this turn:** one manually maintained table-wide spell count, usable as a storm aid. New turn clears this count only. Enter the spell total appropriate to the actual storm trigger; the app does not infer spell copies, triggers or cast timing.
- **Monarch / initiative:** independent flags for whether the local player holds each designation. Opponents' ownership and dungeon progress are not modeled in Solo mode.
- **City's blessing:** a local marker that normally persists for the game; toggling it off is available as a manual correction.
- **Day/night:** a single local representation of the shared table state (neither/day/night), changed manually. No automatic turn-based transformations are inferred.

Threshold warnings remain informational because cards can alter normal winning/losing conditions. All trackers, source names, visibility and settings are included in the saved session.

## Shortcut trackers

Treasures are a manual artifact-token tally; editing does not tap/sacrifice tokens or add mana automatically. Shield is a manual permanent-counter tally, not a player counter. Either shortcut can show these or Poison, Energy, Experience, Storm and Rad; switching visibility never resets a count. The gear's random tools change no game counters. Symbol provenance is documented in [symbols.md](symbols.md).
