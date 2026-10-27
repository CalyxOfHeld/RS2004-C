# Follow-up — October 25, 2026

The mouse-button setting and menu-choice goal recommended below is now accepted within the controlled scope in [D-MOUSE-MODE.md](D-MOUSE-MODE.md). The next bounded task is sound-effect enable/volume clientcode4. The Continue acceptance and its limits below remain unchanged.

---

# Continue-button waiting and reset lifecycle — October 25, 2026

The candidate now preserves revision225's shared `pressedContinueOption` behavior. The first Continue action sends encrypted RESUME_PAUSEBUTTON235 and enters the waiting state. Every buttonType6 label then reads “Please wait...” in its ordinary colour, and its menu entry disappears. Repeated or stale Continue actions emit no further packet. An action still clears item/spell selection when it is suppressed, as the original does.

Evidence: `runs/2026-10-25-continue-sol/`. The final corpus and images are in `reproduce-final3/`; final regression wrappers use `regression-final-*`. This is controlled acceptance of the waiting flag and its reset paths, not acceptance of every interface, script, animation or gameplay workflow. Phase D remains active; Phase E remains pending.

## Source behavior and implementation

The reference is the unchanged local revision225 jar and its corresponding `deob/client.java`: `useMenuOption` action44 (including the2000 offset), `handleInterfaceInput` buttonType6, `drawInterface` text override, `closeInterfaces`, `read` interface controls and last-login140, and normal/reconnect login branches. The oracle calls the original methods directly and makes no source substitutions or instrumentation changes.

`ComponentConfig.pressed_continue` holds the shared flag. Both component drawing paths use the waiting text. The varp-aware path overrides active text and active colour with the ordinary waiting text/colour, matching the original ordering. Menu construction suppresses buttonType6 entries while waiting, and the action dispatcher independently rejects duplicate sends. Existing component text is retained rather than replaced in storage.

| Trigger | Original waiting-flag result preserved |
| --- | --- |
| First Continue action | Send235 plus the two-byte component ID; set waiting |
| Continue while already waiting | No wire or cipher change; clear item/spell selection |
| Interface receive14,28,129,168,195 | Clear waiting after a complete accepted packet |
| Tutorial/sticky interface185 | Retain waiting |
| `closeInterfaces`, including report and last-login callers | Clear only when a sidebar or chat interface was open; a viewport-only close retains waiting |
| Normal login2 or moderator login18 | Clear waiting |
| Reconnect15 | Retain waiting |

Report callers share the same close helper; prior report regressions ran, but this corpus does not independently seed waiting at every report-editor transition.

The existing native interface animation-reset and count-dialog behavior is not promoted to source parity by these flag checks. Three original redraw flags remain unrepresented/unqualified. The last-login report's earlier “continue metadata unqualified” statement is superseded only for this shared flag and the documented close paths.

The resume and close encoders now stage ISAAC state and check output capacity before committing. A failed resume retains the waiting flag and cipher; a failed close retains interfaces, waiting and cipher. Capacity overflow remains sticky until recovery. Rejecting invalid component IDs and retaining state on transport failure are native safety policies, not claims about Java exceptions or failed network delivery.

## Evidence layers

- **1,134 original/native/candidate states:**648 Continue cases,54 close actions,270 interface-control packets,54 tutorial185 packets and108 last-login140 packets. Inputs cross both waiting states,three values for each interface,IDs0/1/32767/65535,one/two/five actions and alternate action44/2044. Both item/spell selections begin set. Full waiting/interface/selection/menu state,outgoing bytes and every outgoing ISAAC field agree. Last-login probes use addresses0.0.2.1 and192.0.2.1; they are not zero-address probes.
- **48 complete source/native panels:**8,208,384 pixels match the actual original `drawInterface`,using synthetic text components and real cached p11/p12/b12 fonts. Waiting,ordinary/active text,active colour,alignment,shadow and non-Continue controls are covered. These are valid component probes,not48 complete cached gameplay interfaces.
- **Actual candidate:**432 encrypted receive packets pass through the real `client_update`,including18 split header/payload controls and full incoming ISAAC comparison.48 actual viewport panels match all10,112 painted original foreground pixels. Whole-scene background pixels are not compared in this task. Selection tooltips are held outside this isolated interface-draw observation.
- **Real UI input path:**three synthetic queued primary clicks,processed through the platform-equivalent event/hover-draw/update sequence,emit exactly one235 and advance ISAAC exactly once. This is an isolated synthetic component,not a human/server dialogue workflow.
- **Login lifecycle:**injected native2/18/15 replies exercise the actual reset branches without account credentials or authentication traffic. Existing “Connected to LIVE SERVER” output in those captures follows an explicitly labeled injected reply and is not server-login evidence.
- **Safety and negative controls:**resume capacity,sticky failure,disconnected immediate output,cipher retention,exact capacity recovery,duplicate suppression,ID bounds and close-state recovery pass. Six copied expectations with corrupted waiting,selection,interface,wire,cipher or painted pixel data each reject with exit3. Pinned source fixtures remain untouched by negative tests.

The reusable `tools/continue_parity.ps1` compiles and invokes the unchanged-jar oracle,regenerates both fixtures,requires their SHA256 hashes to match,and runs native plus candidate controls. The existing registered `test_component` includes these checks; no registered test binary was added. The candidate control remains opt-in.

## Final verification and retained failures

The final full registered suite,separate playable/replay candidates and relevant opt-in controls pass. Seven changed or dependent translation units compile with `-Wall -Wextra -Werror`. Four manually linked older chat diagnostics were rebuilt. Thirteen prior regression families pass on the final builds:incoming private/public,outgoing public/private,social,report abuse,game requests,chat display,overhead chat,receive batch,map download,last-login and input tracking. Their wrappers regenerate original fixtures and verify the pinned expectations. Credential-gated live login is skipped; CRC checks pass. Historical complete C5/recovery screen corpora were not rerun.

Failed setup evidence is retained. The first Java build used the wrong ClientStream package and Jagfile constructor signature; both were corrected against local sources. Early UI-click controls omitted event processing and the preceding hover draw; the harness was fixed without changing production input handling. A manual public-send fault build initially linked its included chatbox implementation twice; its link excludes the duplicate object. Final review expanded the source corpus with item/spell selection observations and corrected suppression cleanup. An intermediate candidate image then included a whole-client selection tooltip; the interface-draw boundary now explicitly clears that unrelated overlay. Earlier captures are historical and are not final acceptance evidence.

All118 saves and828 packed maps match the input-tracking baseline. The installed normal executable and original jar remain unchanged. No installation,commit,production account use,production server restart,user-file deletion,delegation or human messages occurred. Traffic used an isolated ephemeral loopback peer; DNS was replaced in the native control with an immediate controlled failure,so no external lookup was made. Task processes and sockets terminate after checks.

## Plan position

Incoming source routes remain71/71; that count is not an overall completion percentage. Next bounded task:the mouse-button setting (`updateVarp` clientcode5) and its primary-click menu behavior. Music/sound settings3/4,remaining scripts/actions,real-server/player workflows,combined gameplay and delivery qualification remain open. The previously documented populated-camera renderer error6/kind3 remains open. Original redraw scheduling and interface animation reset require separate qualification.
