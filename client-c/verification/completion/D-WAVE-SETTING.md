# Sound-effect setting and playback adapter — October 25, 2026

Follow-up: [D-MUSIC.md](D-MUSIC.md) qualifies music settings andcontrolled shared sound/MIDI requests;physical listening remains pending. Earlier scope statements below remain historical tothis report.

Follow-up: [sound-queue lifecycle](D-SOUND-QUEUE.md) now qualifies timing/replay/retry/login lifecycle. This report retains its original setting/admission scope. Remaining acceptance follows [FINISH-LINE.md](FINISH-LINE.md).

The candidate now applies clientcode4 sound-effect settings. Values0–3 enable new sound requests and select0,-400,-800 or-1200 hundredths of a decibel. Value4 disables new admission while retaining the last volume and existing queue. Other values retain both fields. The SDL output adapter applies the bundled web client's gain formula to newly submitted8-bit samples.

Evidence: `runs/2026-10-25-wave-setting-sol/`. Final reproduction is `reproduce-final2/`;final suite and regression wrappers use `suite-final2` and `regression-final2-*`. Controlled acceptance covers setting state,packet12 admission and the captured output adapter. The full original sound scheduler and audible device output remain separate work. Phase D is active; Phase E is pending.

## Sources and changes

The unchanged revision225 Java `client.updateVarp`, `setWaveVolume`, `read12`, `useMenuOption465/960` and `read150/175/193` supply the state and admission rules. The real cache identifies clientcode4 as varp169. Java stores the attenuation in `signlink.wavevol`;it does not scale the generated WAV samples in that method. The bundled `webclient/src/3rdparty/audio.js` supplies `setWaveVolume` and its gain `10^(dB/20)`,also used by its Client.ts setting handler.

`VarpContext` retains enabled state and attenuation. The existing source-specific varp update/set/toggle/reset paths now apply code4 as well as code5. Cold initialization enables sound with zero attenuation. Normal setting Off does not flush queued or already submitted audio. Packet12 gates new requests on the enabled field and preserves the original raw loop byte,including zero. Existing invalid/unloaded track guards remain native safety behavior.

The SDL adapter retains the unscaled waveform generator and computes gain only when the setting changes. At nonzero attenuation it converts each unsigned8-bit sample using the source gain,nearest-integer conversion and saturation. This quantization is an explicit native adapter policy;it is not a bit-for-bit browser/hardware output claim. Volume changes affect newly submitted buffers;live adjustment of already submitted SDL buffers is not qualified here.

## Checks that passed

- **960 original/native/candidate setting states:**direct update,150 signed-byte,175 integer,193 reset and toggle/set. Inputs cross prior values0/1/4/77,both enabled states,prior attenuation0/-1200 and normal/invalid/extreme input values. Local value,server cache,enabled state,retained volume,full outgoing bytes and every ISAAC field agree.
- **72 original read12 admission states:**cached sound2,both enabled states,prior queue counts0/49/50,loops0/1/2/255 and delays0/1/65535. Complete active queue entries agree. This demonstrates Off retaining prior entries and the50-entry cap for the documented valid track;it does not qualify every track or malformed server ID.
- **Actual candidate:**552 encrypted incoming packets pass through `client_update`,with full incoming ISAAC comparison. The control holds `audio_tick` at the packet observation boundary so packet-phase state can be compared independently of scheduling. Setting actions use the production dispatcher with valid synthetic script probes;this task does not add a new cached-button pixel/hover corpus.
- **Original web gain:**the unchanged audio.js module runs with browser AudioContext/document boundaries replaced. All four gain values match native doubles within1e-14. All1,024 byte inputs across those gains satisfy the declared native conversion policy.
- **Actual output adapter:**production `audio_play` and queue/tick code run with SDL device/output calls captured. All22,048 bytes from the real cached sound at four source gains match that policy. An existing queued sound is still submitted after admission is disabled. No speakers are used by this control.
- **Waveform regression:**the unchanged Java WaveOracle regenerates870 complete cached WAV records with its documented deterministic noise input,loops1/2. Its fixture is SHA256-identical to the pinned expectation and the registered native comparison passes. This is a separate waveform layer,not a claim about physical-device audio or arbitrary loop counts.
- **Negative controls:**four copied local/enabled/attenuation/cipher expectations reject with exit3. Original fixtures remain untouched. `tools/wave_setting_parity.ps1` regenerates both source fixtures,requires exact hashes and runs native/candidate controls.

The existing registered `test_audio` includes setting/gain checks. The output/candidate control remains opt-in;TEST_BINS is unchanged. The original Java and web sources are read-only and have no instrumentation edits.

## Verification, preservation and limits

The full registered suite and separate playable/replay candidates pass. Four changed/dependent translation units compile with `-Wall -Wextra -Werror`;the candidate also builds strictly. Relevant opt-in binaries and four manually linked diagnostics were rebuilt for the changed context layout. Fifteen existing regression families pass:incoming private/public,outgoing public/private,social,report abuse,game requests,chat display,overhead chat,receive batch,map download,last-login,input tracking,Continue and mouse-button settings. This is fresh evidence on the changed sources. Historical full C5/recovery image corpora were not rerun;credential-gated live login remains skipped and CRC checks pass.

The setting and gain fixtures reproduce exactly. Before/current sources,task-only diff,identities,four binary snapshots and the completion audit are retained. All118 saves and828 packed maps match the mouse-button baseline. The installed normal client and original jar are unchanged. Production43595 was observed absent;no server action occurred. No installation,commit,user-file deletion,delegation,production account or human message occurred. The candidate uses isolated loopback;the adapter control captures device calls. The older registered audio test retains its existing device-dependent MIDI smoke checks;those are not new wave audibility evidence.

Unqualified work includes low-memory mode,full queue timing and replay/retry logic,overlap clocks,normal-login queue resets,live-buffer attenuation,arbitrary sound IDs/loop counts,physical devices and sustained gameplay. The current native scheduler runs at a different observation phase from the original;holding its tick in the control explicitly avoids claiming that timing is accepted.

Next bounded goal:original sound-queue timing,replay/retry and normal-login reset lifecycle. Music/clientcode3,remaining UI/scripts/redraw/animation,real-server/player workflows,combined play/release and the prior populated-camera renderer error6/kind3 remain open. Incoming source routes remain71/71;this is not an overall completion percentage.
