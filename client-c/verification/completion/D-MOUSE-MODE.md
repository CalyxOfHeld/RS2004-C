# Follow-up — October 25, 2026

The sound-effect setting/admission and captured gain adapter recommended below now have controlled acceptance in [D-WAVE-SETTING.md](D-WAVE-SETTING.md). Original sound-queue scheduling and device qualification remain open.

---

# Mouse-button setting and menu choice — October 25, 2026

The candidate now applies revision225's mouse-button setting. Exactly value1 selects one-button mode: a primary click opens the menu when it contains more than two entries. An Add Friend top action also opens the menu under that condition in two-button mode. Other values retain ordinary primary-click behavior. Short item-drag releases make the same choice, and a press during an existing item drag is deferred.

Evidence: `runs/2026-10-25-mouse-mode-sol/`. Final source/native/candidate reproduction is `reproduce2/`; final full-suite and regression logs are named `suite-final` and `regression-final-*`. This is controlled setting/input/menu-placement acceptance. Phase D remains active; Phase E remains pending.

## Original source and implementation

References are the local revision225 `client.updateVarp`, `read150/175/193`, `useMenuOption465/960`, `isAddFriendOption`, `handleMouseInput`, `showContextMenu`, and the original drag-release block in `updateGame`. Cache `varp.dat` identifies clientcode5 as varp170 and varp0 as a neutral control. The four actual cached setting buttons are913/914 under904 and6278/6279 under4445.

`VarpContext` now retains the clientcode metadata and `mouse_buttons_option`. An explicit update applies code5; ordinary packet/set updates apply it only when the local value changes. Toggle actions always invoke the side effect and ignore the set operand. Set actions retain the original same-value suppression. Signed-byte packets and Java integer wrap are preserved. Reset193 walks changed local/server values in order rather than copying arrays without side effects. The option is retained until a source update changes it; it is not polled from varps every draw.

Menu choice uses the final top typed action,including Add Friend and its2000 action offset. It requires more than two entries. Presses during a drag and the original spell-selected reserved rectangle are suppressed. A non-drag release opens at the original press position. This does not qualify successful inventory swaps or every inventory action's drag-initiation whitelist.

The new225 menu-opening path uses the original strict viewport/sidebar/chat rectangles and their distinct clamp ordering. Native storage remains in whole-window coordinates. The original21-pixel clamp calculation versus22-pixel stored height is retained,including its tall-chat-menu result. The old generic menu-opening API remains available for historical fixture callers; the actual client uses the225 path. Existing drawing and row-hit routines are reused.

The setting-button155 encoder now stages ISAAC and checks capacity before committing. Failed output leaves the option/local varp/cipher unchanged,with exact-capacity recovery checked. This is a native safety policy,not a claim about original array exceptions or failed network delivery. Other outgoing encoders are not promoted to transactional behavior by this change.

## Evidence and boundaries

- **1,744 setting states:**actual original update/read/action methods versus native and candidate. Direct updates,150 signed-byte,175 integer,193 cache reset,toggle/set,same-value retention,extreme integers and a neutral clientcode control compare local value,server cache,mouse option,all output bytes and every outgoing ISAAC field. This includes16 states for the four real cached buttons.
- **5,120 input states:**4,800 original `handleMouseInput` cases and320 original drag-release slices. Crosses six option values,zero/one/two/three/twenty entries,Continue44/2044,Add Friend406/2406,Add Ignore436,left/right buttons,four positions,spell selection and an existing drag. Input menus are deliberately prepared to test the final top-action decision;this does not qualify all menu construction or sorting paths. Social labels omit the name tag that would perform an actual social operation;real Add Friend execution retains its separate report.
- **Actual candidate:**864 encrypted varp packets and24 split header/payload controls run through `client_update`,including full incoming ISAAC checks. All5,120 input states use the public queue and event-processing path. Menus,option/waiting/drag state,global rectangles and full outgoing wire/cipher match. Sixteen cached settings clicks use actual component-tree positions,hover drawing,menu construction and dispatch. No account or production connection is used.
- **Placement and scope:**all observed menu rectangles match the original across viewport,sidebar,chat and reserved positions,including tall menus. Labels use the documented short ASCII repertoire with a colour tag. This task does not compare menu pixels,arbitrary long/non-ASCII labels,all clipping behavior,already-visible menu interactions,physical devices or same-cycle setting-packet/click scheduling.
- **Safety/integrity:**null/empty/over-limit menu guards and setting-button output-capacity retention/exact recovery pass. Six copied expectations with corrupted local varp,option,wire,cipher,menu visibility or rectangle data reject with exit3. Pinned source fixtures were regenerated from the original classes rather than manually edited.

The original Java files and jar are unchanged. The isolated copied client replaces only the body of `updateGame` with its exact contiguous original drag-release fragment plus a counter. This explicitly omits unrelated prelude/tail behavior;it is not a full original tick. All other original methods are retained verbatim. A class-identity guard rejects the original unmodified class when the wrong classpath would bypass that slice,and produces no forbidden fixture. The extraction,diff,generated source and identities are retained. `tools/mouse_mode_parity.ps1` reproduces the extraction,capture,SHA256 comparison,negative class guard and native/candidate runs.

The registered `test_context_menu` was extended;no registered test binary was added. The candidate control is opt-in.

## Regressions and preserved work

The full registered suite,separate playable/replay candidates and relevant opt-in controls pass. Seven changed/dependent translation units compile with `-Wall -Wextra -Werror`. Four older manually linked diagnostics were rebuilt. Fourteen final regression families pass and regenerate their original expectations:incoming private/public,outgoing public/private,social,report abuse,game requests,chat display,overhead chat,receive batch,map download,last-login,input tracking and Continue. Credential-gated live login is skipped;CRC checks pass. Historical complete C5/recovery image corpora were not rerun.

Failed setup evidence remains visible. The first oracle used an API unavailable in the installed Java8 runtime. An early social label accidentally contained the name tag and triggered original Add Friend without an initialized player;it was corrected to the intended non-executing colour label. Initial indentation warnings and a PowerShell source-guard argument error were corrected. Earlier1,728-state captures predate the additional cached-button states and are historical. Final fixture expectations come from the1,744-state source capture.

All118 saves and828 packed maps match the preceding Continue baseline. Installed normal client SHA256 remains `A85F23714A8C89BF00D22B9A62E73295E9B1438DBEF830301C0EB524A41928D4`;original jar remains `57EF9307E2B31674CB55F0F48C1643CDED751E1A97D5035B28F75DB7FD1E6CC7`. Production43595 was observed absent;no server action occurred. There was no installation,commit,user-file deletion,delegation,production account use or human message. Task sockets and processes terminate after checks.

## Next bounded work

Check sound-effect enable/volume setting clientcode4 and its downstream playback behavior. Music/clientcode3,remaining interface scripts/actions/redraw/animation,real-server/player workflows,combined gameplay and release qualification remain open. Incoming routes remain71/71,a source fact rather than a completion percentage. The prior populated-camera renderer error6/kind3 remains open.
