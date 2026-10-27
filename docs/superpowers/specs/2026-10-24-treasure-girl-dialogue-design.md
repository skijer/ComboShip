# Treasure Girl dialogue

User-approved direction: short, in-character dialogue plus optional shop help, reached through A / “Talk to shopkeeper” at the keeper view. Left/right continues to browse. No forced tutorial on entry or on repeat visits. The user explicitly approved this integration after the recommendation.

Use the existing native message system in both games. In MM, retain the actual EnOssan actions and add only a private keeper-dialogue adapter boundary; no new browsing or camera controller. In OoT, adapt the existing keeper-view A path and its text/phase handling while preserving the accepted Data4 camera implementation.

Flow: keeper A → a brief character response → optional “How does this shop work?” / “Back” choice. Help uses a few native message pages covering prices/confirmation, unique dungeon-clear seals, and one-time offers versus bottle refills. Finishing help or choosing Back returns to the keeper view. B in this conversation returns to the keeper; ordinary shelf B/cancel semantics remain unchanged. Neither dialogue nor help may reserve, grant, charge or save an item.

Copy direction (builder choice): friendly Treasure Chest Shop Gal, with a light callback such as “No guessing games here. Pick the treasure you want!” Avoid implementation terms, seeds, actor IDs and forced exposition. Do not imply stock rerolls or every offer is renewable. Show the exact A action “Talk to shopkeeper” where the native UI permits it; keep native choice/control/page widths and bottom merchandise placement correct.

Approaches considered: generic greeting only is simpler but leaves the rules unexplained; a mandatory tutorial explains the rules but interrupts repeat visits; the approved optional help combines character with discoverable instructions. Copy may be adjusted for native line fit without another approval loop.

Preserve all camera, stock, receipt, save, title, shimmer and spin fixes in source milestone be3d5660de7f88732cf8e5b6a5868c036d35043d. Publication remains held for bridge-source integration and configured runtime acceptance.
