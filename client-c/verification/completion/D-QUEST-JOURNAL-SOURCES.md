# Historical correction — quest progress journals did not exist in revision225

Quest progress journals were introduced on **20 July 2004**, after the **18 May 2004** target. Their absence is expected for this revision. The earlier TODO to obtain revision225 journal text is withdrawn; do not implement a later feature as a225fix.

Sources: [Lost City version FAQ](https://2004.lostcity.rs/news/184) explicitly confirms no quest journals in itsMay18target; [maintainer's later update](https://2004.lostcity.rs/news/224) dates their introduction toJuly20,2004. The inventory below remains accurate, but its previous interpretation as missing target support was incorrect. Inventory books remain separate from quest progress journals.

---

## Earlier source availability audit (interpretation superseded above)

# Quest-journal source availability — October 26, 2026

User scope: implement quest-journal support **where the bundled sources provide an implementation**. No eligible quest-list journal implementation was found. No application change was made.

The actual normal server pack contains 8033 scripts and 52 clickable quest-list entries. All 52 lack an IF_BUTTON handler. Searches of the active content tree and sibling `Server-main/content/scripts` found no quest-list click scripts or stage-dependent journal procedures to connect. The Java/TypeScript client sources do not supply the missing server-side journal content.

The bundled implementations found are different features:

| Existing source | Implemented behavior |
| --- | --- |
| `content/scripts/general/scripts/quests.rs2` | Quest progress colours, quest points and completion screens |
| `content/scripts/tutorial/scripts/guides/quest_guide.rs2` | Tutorial explanation of the quest side panel |
| `content/scripts/quests/quest_grandtree/scripts/gloughs_journal.rs2` | Inventory Read handler for Glough's journal, item785 |
| `content/scripts/quests/quest_upass/scripts/old_journal.rs2` | Inventory Read handler for Journal of Randas, item1493 |
| `content/scripts/quests/quest_upass/scripts/history_of_iban.rs2` | Inventory Read handler for History of Iban, item1494 |

All three inventory Read handlers are already present in the normal packed trigger table. Their narrative book text is not quest-progress journal text. Connecting those item handlers to quest-list buttons would change their access and potentially quest variables; it would not supply the requested quest journal.

TODO: obtain revision-225 quest-list journal text and stage conditions before implementing those missing handlers. Completion messages, NPC dialogue or later-revision journals are not substitutes. Existing source comments such as “probably for quest journal” do not provide an implementation.

Evidence: `runs/2026-10-26-quest-journal-sol/normal-packed-inventory.json` lists all52 buttons and the three existing inventory handlers; `normal-inventory.log` and `.exit` record a successful read-only inventory of the actual normal server pack. The earlier `packed-inventory.json` is a corroborating inventory from the disposable runtime. No compilation, client test campaign, server restart, save modification or application patch was required for this availability check. Earlier stair/clue/audio fixes remain unchanged.
