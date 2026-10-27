# Downloaded map lifecycle — controlled acceptance, October 24, 2026

The native client now receives DATA_LAND132/DATA_LOC220 and their completion packets80/20, retains their compressed buffers, saves completed data, and resumes scene construction on PLAYER_INFO184. Missing files no longer cause an immediate empty rebuild. This closes four missing receive routes for the controlled revision225 scope below. Phase D remains active; final gameplay/release qualification remains open.

Evidence: `runs/2026-10-24-map-download-sol/`. Latest reproducible checks: `reproduce-final/`; full suite: `suite1.log`; prior-feature checks: ten `regression1-*-wrapper.exit` files. The registered test remains `test_map_streaming`; two diagnostic executables are opt-in and were not added to TEST_BINS.

## Source behavior retained

The unchanged original client supplies the expectations. Relevant handlers are client.java:9889 (80),10122 (20),10206 (220),10464 (132),9912 onward (237),10800 onward (184),and8930 onward (buildScene). Signlink supplies cache load/save behavior; the engine's RebuildGetMapsHandler supplies the991-byte chunk size.

- Chunks contain byte mapsquare coordinates, unsigned two-byte offset and total size, then data. The last matching descriptor wins, including duplicate descriptors. Unknown maps do nothing. A changed total size replaces the old buffer with zeroed storage; same-size chunks retain unwritten bytes. Chunks leave scene state unchanged.
- REBUILD_NORMAL237 checks initial cached data against nonzero CRCs and requests missing/bad files in descriptor order, land before locations. A zero CRC ignores even an existing cache file. An empty request still emits150 plus a zero length. Same-center237 is ignored once scene state is nonzero; while waiting in state0 it is processed again.
- Completion saves the current buffer and sets scene state1. The original does not recheck its CRC or require all map files to finish. The next184 can build a land-only scene; later location completion permits another build. A cache-write failure still permits a build from RAM.
- Source signlink addresses physical files by hashed logical keys; the native existing cache uses names such as m50_50. Checks compare logical names and exact bytes, not physical disk filename layout.

The native cache writer publishes through a newly created same-directory temporary file and Windows replacement. A failed publication preserves the prior file and removes its own temporary file. Downloaded/cache buffers are released on replacement, logout and shutdown. RS2_MAP_CACHE_DIR selects the isolated cache used by these tests.

## Original and native comparisons

MapDownloadOracle calls the original read handlers and actual buildScene using the unchanged jar, real packed config/models/textures/media/title and m50_50/l50_50. Its in-memory signlink worker copies bytes on load/save and can simulate failed writes. Presentation draw is suppressed; real viewport/font binding and minimap creation still execute. External error reporting is disabled. The fixture pins these controls and outgoing seed; it does not patch original handlers.

| Check | Fresh result |
| --- | --- |
| Original capture |374 states,367 receive packets,7 framed237 observations,24 done observations,7 completed actual buildScene snapshots |
| Registered native test |All374 states,full nullable compressed buffers and logical cache snapshots,encrypted requests/keepalives and complete outgoing ISAAC state match |
| Separate candidate |367 actual ISAAC-framed receive packets through client_update,complete incoming and outgoing cipher states,12 fragmented headers;the same374 snapshots and7 rebuilds match |
| Geometry |All4x105x105 heights and4x104x104 tile flags/collision entries match at each of the7 rebuilds |
| Candidate recovery |13 malformed variable-length/range receive guards;corrupt-map184 retains terrain/collision/loading;fresh237 and real map downloads then rebuild successfully |
| Reproduction |Original fixture regenerated exactly in reproduce1/2/3/final;latest wrapper exits0 with coverage markers and source/binary/cache identities |
| Negative expectations |Separate buffer,encrypted-wire and height mutations each reject with exit3;original fixture remains unchanged |

The corpus includes missing maps, reversed991-byte land chunks, player updates while waiting, land-only completion followed by location completion, repeated done, ignored same-center237, cache hits, bad initial CRC, repeated237 while waiting, failed cache save with successful RAM rebuild and later retry, zero-CRC cache exclusion, duplicate descriptors, out-of-order holes, resized/zero/65535-byte buffers, unknown maps, and300 deterministic random chunk cases. The237 count includes the ignored same-center observation;it is not a count of seven emitted150 requests.

## Safety and regressions

The strict failure control passes4 injected allocation failures,5 cache I/O failure stages (create,partial write,flush,close,replace),189 malformed lengths/ranges/null and rebuild-descriptor guards,and8 corrupt compressed-map rejections. Prior cache bytes and map ownership survive;temporary files are absent after failed publication. Request capacity/sticky overflow/disconnected/invalid-input failures preserve output and cipher state as defined. Valid compressed-map decoding recovers after an allocation failure.

Maps are decompressed with the existing bounded exact decoder. Declared decoded size is limited to the original100000-byte scratch capacity;land/location streams are validated before clearing terrain/collision. These guards safely reject cases where the original would throw or abort its build. They do not claim atomic recovery for every allocation inside downstream world/model construction. Fixed-size done length guards are unit checks;actual transport cannot encode an alternate length for a fixed-size packet.

The full registered suite passes with no compiler warning/error markers. Current separate playable candidate/replay and strict map_streamer,protocol,legacy client and map-test builds pass. Current rebuilt private/public inbound and public-send diagnostics pass their wrappers. All ten regression wrappers exit0:incoming private/public,outgoing public/private,social actions,report abuse,game requests,chat display,overhead chat and the camera/interface/reset/multiway receive batch. Credential-gated live login is skipped;its CRC check passes. Historical full C5/recovery image corpora were not rerun.

Failed/setup evidence is retained: initial original capture lacked minimap/texture setup and entered error reporting;the verified task-owned Java process was stopped and its command line saved. The next launch had a PowerShell argument-quoting error. Original source/jar were never edited. The fault control's initial strict build caught an unused counter;the corrected build passes. Early map compilation warnings were corrected. Those earlier results do not supersede the fresh acceptance logs.

## Preservation and limits

All828 packed map files match map-cache-before.json by name,size and SHA256. All118 saves match the preceding receive-batch baseline. Installed rs2client.exe remains A85F23714A8C89BF00D22B9A62E73295E9B1438DBEF830301C0EB524A41928D4;the original jar remains57EF9307E2B31674CB55F0F48C1643CDED751E1A97D5035B28F75DB7FD1E6CC7. Production43595 was observed NOT LISTENING. This task did not start/stop/restart the production server,install a client,commit,delete user files,delegate,or use a production account. Task sockets and processes are closed. All test cache writes use new isolated directories retained as evidence.

This accepts the controlled map transport/cache/rebuild lifecycle with real50_50 data and the described safety controls. It does not qualify arbitrary maps/model-rendered frames,natural boundary travel,all rebase/ground-item interleavings,or a final combined gameplay session. The native descriptor bound is16;the current engine limits a request to9 mapsquares/two files each. Native cache publication targets Windows;the non-Windows branch reports write failure and retains RAM. Broader renderer and interface limits from the preceding receive batch remain open.

The fresh71-type source audit finds68 behavior routes and3 missing:tracking133/226 andlast-login140. All71 original/native/engine framing lengths agree. Raw generator59/62 incoming and60/71 outgoing remain heuristics,not completion percentages. Next recommended bounded task:last-login140,then tracking133/226/outbound81. Overall status,remaining work,current capabilities,coverage andboth handoffs are synchronized.
