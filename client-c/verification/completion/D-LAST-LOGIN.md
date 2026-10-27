# Follow-up — October 25, 2026

[D-CONTINUE.md](D-CONTINUE.md) now qualifies the shared waiting flag and the source-specific close behavior used by last-login140. This supersedes the older continue-metadata limitation below only for that flag. The three redraw flags,broader scheduling and interface animation remain unqualified. The original last-login evidence is retained unchanged.

---

# Last-login information — controlled acceptance, October 24, 2026

The candidate now applies LAST_LOGIN_INFO140, opens the appropriate cached welcome panel when the original would, and displays the original login-address, unread-message and recovery text. Address lookup runs asynchronously. This closes one missing receive route: the current source inventory is69 routes out of71,with only tracking133/226 missing. Phase D and final gameplay/release qualification remain open.

Evidence: `runs/2026-10-24-last-login-sol/`. Final checks are `suite-final3.log`,`reproduce-final3/`,and eleven `regression-final3-*-wrapper.exit` files. The existing registered `test_component` was extended; `last_login_candidate_control` is an opt-in diagnostic,not a new TEST_BINS entry.

## Original behavior

The unchanged revision225 client.read handler at client.java:10281 reads an address,g2 login age,g1 recovery age,andg2 unread count from9 bytes. It always stores those four fields. Only a nonzero address with viewportInterfaceId==-1 triggers the following:

1. Publish the numeric IPv4 string and request DNS lookup.
2. Queue closeInterfaces231 and close sidebar/chat/main interfaces.
3. Clear reportAbuseInput and its mute option,while retaining report interface ID,count input,social input and ordinary typed input.
4. Scan component slots in order and open the first matching component's layer:clientcode650 when recovery age is201,otherwise655. No match leaves the viewport closed.

updateInterfaceContent at client.java:5978–6041 supplies the exact650..655 strings,capitalization,plural forms and unread colours. Zero unread messages are yellow;positive counts are green. Recovery values200 and201 have distinct source meanings. Zero address clears the address label without changing unrelated colours.

The native implementation stores this state separately from chat and prepares those component labels before drawing. It uses the existing staged close231 sender. On close-output failure,the four fields and numeric DNS have already changed,as ordered in the source;interfaces/report state remain unchanged,and native output bytes/cipher are preserved. Invalid lengths/metadata reject before mutation.

## Reference capture and comparisons

LastLoginOracle calls actual read140 and updateInterfaceContent from the unchanged jar,using real title/media/interface packs. It records four source redraw/continue metadata fields separately;these do not have equivalent native flags and are **not qualified** by this task.

| Check | Fresh result |
| --- | --- |
| Original packet corpus |2,406 read140 states,678 close231 observations |
| Registered native check |Exact four stored fields,interfaces,report/input preservation,DNS strings,encrypted close bytes and full outgoing ISAAC state |
| Actual separate candidate |2,406 encrypted packet-phase observations through client_update,full incoming cipher,source outgoing wire/cipher and24 fragmented payloads |
| Content branches |All650..655 text and colour controls,including zero/one/many/max counts and login/recovery boundaries |
| Native full panels |24 complete512x334 images,4,104,192 pixels and boundary guards match original |
| Candidate draw path |24 actual client_draw viewport panels,2,478,029 original foreground pixels match |
| Negative expectations |Separate stored-state,text,wire,pixel and DNS-worker mutations reject with exit3 |

The packet corpus covers zero/low/high-bit/all-one addresses,login ages0/1/2/65535,recovery values0/1/2/199/200/201/202/255,unread0/1/2/65535,viewport-1/0/42,sidebar/chat combinations,96 deterministic random cases,and missing/duplicate target components. Long255-byte hostname inputs and142 initial-null DNS inputs are included;102 retain null after read140. Native empty DNS storage represents the original null and renders Java's literal null when the address label needs it. Every admitted opening,including a missing target widget,queues231. Existing viewport or zero address suppresses both closure and new DNS requests.

This cache contains650 at widget6066,651 at6069,and655 at6194;welcome roots are5993 and6073. Clientcodes652..654 are absent. Their text/colour checks call the actual original method on three direct component probes;no missing widgets were invented. The full panel images use only the real cached roots. Source colours are restored before panel capture.

The candidate observer captures chat state at the packet handler before later tick work. Fixed player/cycle counters hold unrelated inputs. This qualifies the packet phase,not all subsequent tick effects. Candidate panel comparisons cover source foreground pixels;the registered native comparison covers complete local panels,not a complete populated gameplay screen.

## DNS worker behavior

The original signlink.run has a significant concurrency quirk:if a newer dnslookup arrives while an old lookup is blocked,the old completion can replace the newer numeric display and clear the newer pending request. The native client preserves that observed behavior and permits one active lookup at a time.

SignlinkDnsOracle checks five worker states and three resolver calls:initial numeric display,new request while busy,old completion and newer-request loss,a subsequent successful request,and lookup failure publishing unknown. Its generated signlink copy retains the original run/dnslookup code except for exactly three external-boundary substitutions:cache-directory startup,UID startup,and the resolver call. The first two avoid disk-cache access;the resolver supplies controlled blocking/results without an external DNS query. The verifier checks each substitution occurs once and retains the generated source. The original source and jar are untouched. A class-origin guard rejects an accidental launch against the uninstrumented worker before any worker/cache activity;the wrong-classpath control rejects with exit1. Packet/UI capture continues to use the unchanged jar's signlink implementation,not this worker copy.

The candidate matches those five source-worker states,call counts and IPv4 byte order. Controlled allocation/thread-creation failures leave the numeric display intact. Shutdown releases the main thread's ownership without blocking on lookup;the worker owns its result storage until completion. The pending-worker shutdown control waits for that controlled worker to terminate and verifies no worker remains.

An earlier native policy rejected stale completions and queued the latest request. Source review identified that difference before acceptance. It was replaced with the original request-loss behavior;the earlier controls/logs remain historical. Final source-worker expectations were captured from the source branch,not edited to fit the native result. Arbitrary OS resolver results,internationalized hostnames and exact thread scheduling are not qualified. Completion publication is polled by the main native loop;the controlled interleaving is the claimed concurrency scope. The current worker implementation targets Windows;other platforms retain the numeric display.

## Safety, regressions and preservation

26 malformed-length/null/metadata guards pass without mutation. Capacity/sticky-overflow/disconnected close failures preserve output/cipher and interface/report state. Exactly one byte of free capacity permits successful close/recovery. Zero address/existing viewport remain no-op cases for closure and DNS even when output is unavailable. Unrelated clientcodes retain their text and colour. Real transport tests cover fixed packet fragmentation;alternate fixed-size lengths are unit-only guards.

Final full registered suite,current separate candidate/replay and opt-in builds pass with no compiler warning/error markers. Strict component_packets,legacy client and component-test builds pass. The old manually linked incoming private/public and public-send diagnostics were rebuilt against current sources. All eleven final regression wrappers exit0:incoming private/public,outgoing public/private,social actions,report abuse,game requests,chat display,overhead chat,the camera/interface/reset receive batch,and map download. Credential-gated live login remains skipped;CRC checks pass. Historical complete C5/recovery frame corpora were not rerun.

All three fixtures reproduce exactly in the final verifier:packet state,panel images and source DNS-worker states. Failed initial captures are retained:the first two assumed all six clientcodes had cached widgets. The corrected capture distinguishes three cached widgets from three method probes. The source-review DNS correction and earlier successful-but-superseded controls are also retained transparently. A final boundary review added the initial-null DNS inputs:the actual original recapture changes142 input DNS values,102 post-read DNS values and158 address-label strings;all other fields,wire/ciphers and the full panel fixture remain identical (nullable-dns-corpus-diff.json). The prior fixture is retained;new expectations came from recapture,not manual edits.

All118 saves match the preceding map-download baseline;all828 packed maps match by name,size and SHA256. Installed rs2client.exe remains A85F23714A8C89BF00D22B9A62E73295E9B1438DBEF830301C0EB524A41928D4;the original jar remains57EF9307E2B31674CB55F0F48C1643CDED751E1A97D5035B28F75DB7FD1E6CC7. Production43595 was observed NOT LISTENING. No production server start/stop/restart,client installation,commit,user-file deletion,delegation,production account activity or human messages occurred. Test sockets/processes and controlled resolver workers are terminal.

This accepts the represented packet state,conditional opening,content,controlled DNS behavior and panel scope. It does not close the missing redraw/continue metadata representation,external account-management actions,arbitrary resolver behavior,natural login workflows,or final combined gameplay qualification. Those limits remain in the current remaining-work index. Five existing source/test/build files and eight new reference/control/verifier/fixture files are packaged with dirty baselines,task-only diff,current sources,identities and separate binaries.

All71 original/native/engine framing lengths still agree. Corrected incoming inventory is69/71;raw incoming60/62 omits nine grouped zone entries,and outgoing60/71 remains a literal heuristic with known118 miss/11 false match. These are not overall completion percentages. The next bounded receive task is tracking133/226 and its associated outbound81/input lifecycle,using isolated controls. Current overview,remaining work,capabilities,coverage and both handoffs are synchronized.
