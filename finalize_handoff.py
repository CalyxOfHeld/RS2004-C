from pathlib import Path
import hashlib, json, zipfile

root = Path(__file__).resolve().parent
completion = root / 'client-c/verification/completion'
def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest().upper()
native_sha = sha(root / 'client-c/bin/rs2client.exe')
assert native_sha == '78346F76B3BE7C689B2C472F60E092FA647B64F31D92AD742943D3AEE406D28F'
def write(name, text): (root / name).write_text(text, encoding='utf-8')

write('TEST-THIS-CHECKPOINT.md', '''# Test the current Windows client — October 21, 2026

## Launch

The current client is already installed in this checkout. Double-click play_client.bat in `2004scape C Rewrite`. The existing server was confirmed listening on43595/8889/8899 in PID3128 at handoff. If it is no longer running, use start_local_server.bat and wait for World ready. The launcher preserves account progress and uses the existing local test account automatically. For another account, see MANUAL-TESTING.md. Login code5 means already logged in or pending logout. Close the client window to exit.

The newest complete update is `handoff-checkpoint-2026-10-21.zip`, with a .sha256 sidecar. Extract its contents directly into the existing `2004scape C Rewrite` folder, allowing matching client/source/doc files to be replaced. Existing engine/content/cache are required. No saves or .env files are included. Older ZIPs are historical snapshots; never restore the withdrawn save-purge launcher from an old package.

## Visible changes to try

1. Close any bank/shop interface, then left-click nearby walkable ground on the minimap. The client should send a route and display its destination flag. Rotate with arrow keys and try another click. Movement remains driven by the real server.
2. Watch the destination flag clear on arrival. Ordinary NPC, scenery and ground-item approaches now set the same flag. Server-clear and region-rebase handling are implemented but still need live verification.
3. Other visible players now have ordinary minimap dots. Friend colouring is unfinished. NPC and ground dots use the original coordinate rounding.
4. Bank Deposit/Withdraw X, bank dragging/close/reopen, shop Buy/Sell, ground Drop/Take and stairs are useful existing checks. Available items, tutorial restrictions and scripts depend on the actual account/server state.

Minimap clicking is live-only. An open context menu, active inventory drag or main modal blocks it. Nonzero minimap zoom/angle drift and wider navigation parity remain unfinished. Escape closes an active menu/interface and otherwise selects Inventory; window close exits.

For a bug report, include account name (never password), location, clicked position/action, camera direction, expected/actual outcome and screenshot or console log. Preserve saves.

## What has actually been checked

The older world-loops executable passed37 programs and separate instrumented live ground, bank, shop, dialogue and stair scenarios. Eight subsequent small changes built with zero warnings, including the current minimap input implementation. Tests and live scenarios were not rerun for those changes. Audible MIDI, friend colouring, complete gameplay and original visual parity are not established.

Current build/evidence scope: client-c/verification/completion/runs/2026-10-21-minimap-click/README.md. HANDOFF.md identifies the latest source, hashes, gaps and continuation order. Cursor/Grok audit recovery is deferred; do not spend more tokens searching for its partial output.
''')

tasks = ['midi-status', 'minimap-offset', 'player-dots', 'destination-flag', 'approach-flag', 'song-packet', 'packet-integers', 'minimap-click']
latest = '''# Current handoff — October 21, 2026

## Read this section first

Stopping at the requested usage handoff boundary. The last usage reading before writing was93% used/7% remaining; the user reserves the final5% for handoff. No further implementation is running. The small final task consolidated TEST-THIS-CHECKPOINT.md and continuation instructions. All lower sections are historical and are superseded where they conflict with this section.

Latest runnable native: `client-c/bin/rs2client.exe`.
SHA-256: `NATIVE_HASH`.
Complete update: `handoff-checkpoint-2026-10-21.zip`; authoritative archive checksum is its .sha256 sidecar. It overlays this existing rewrite checkout and requires the existing engine/cache/content. `play_client.bat` launches the installed native on127.0.0.1:43595. TEST-THIS-CHECKPOINT.md is the consolidated user guide. No remote is configured; no Git push or commit was made. The Git working tree is predominantly untracked; do not reset it or discard changes.

## Eight small code tasks since the tested world-loops checkpoint

1. MIDI backend checks open/play/status/restart errors and reports failed song commands; short writes/signature buffers reject.
2. Minimap dots quantize target/player coordinates separately, matching original Java.
3. Ordinary visible-player minimap dots use original cache mapdots2. Friend colouring awaits social state.
4. Original cache mapflag renders scene-walk destination; arrival/packet19 clears it, region rebase shifts it, distant markers respect the mapback mask.
5. Shared operation approach helper updates the same flag after a successful nonempty route send.
6. MIDI_SONG54 uses byte10 name termination and exact metadata framing; unsigned metadata assembly avoids signed shifts.
7. Shared g4/g8 assemble unsigned values and map to signed Java values without C signed-shift overflow/out-of-range casts.
8. Live minimap clicks use original x582..727/y14..164 rectangle, inverse camera rotation and collision routing. Opcode165 shares route compression and appends the original14-byte trailer, included in packet length. Zero angle/zoom match current rendering. Successful sends set the flag; local movement is not fabricated. Menu/drag/main-modal gates apply; same-tile/failed routes send nothing. Ctrl forwards the run modifier.

Each task has source snapshots, source-review.diff, build.log, build.exit, README and manifest under `client-c/verification/completion/runs/2026-10-21-<task>/`. Every build exited0 with zero warnings. No tests or live sessions were run for these eight changes. Do not transfer old acceptance claims to this binary. The final minimap implementation is unverified in a live session.

## Earlier evidence that remains valid for its recorded checkpoint

The old `world-loops-test-checkpoint.zip` preserves executable3035DF94DAFA31A55C038CB1F25DEC795118BD7513EBA85AA463B3ACF0077919. Its37-program suite and the later shared-object Makefile suite passed. Ground COUNT150->200/public reveal, bank X/drag, natural bank dialogue/player portrait, natural shop Buy/Sell and stairs up/down have bounded instrumented live SDL evidence in runs/2026-10-21-world-loops. Fixture placement/preparation is explicitly distinguished from natural travel/acquisition. Failed probes are retained. Captures were losslessly converted to PNG; replay scripts still name original BMPs.

Original Java audio oracle matches all435 cache sounds at loop counts1/2 for20,359,589 WAV bytes with a shared deterministic noise table. This is not random-generator, MIDI or device-timing parity. It fixed actual tone terminator/track-bound/invalid-loop errors; fixture and provenance remain packaged.

## Next work

- Address the user's feedback first. A live minimap click/camera/arrival/cancel/region check and regression coverage are the immediate verification gaps; tests were not requested for these small tasks and were not run. Do not label them passed from build output.
- Review original-input parity, nearest endpoints, route compression limits and nonzero minimap angle/zoom before claiming full navigation completion. Original/server sources are authoritative; do not use OSRS or later revisions.
- Network-driven MIDI still calls the shared bzip_decompress without output-capacity bounds and with fatal error handling. This is a known follow-up, not fixed by packet framing checks. Song CRC/size validation, temporary-file collisions, jingle delay and device behavior also remain incomplete.
- Continue remaining item-use/spell/combat/skilling/social/effect and renderer/session gates from AUTONOMOUS_PLAN.md. Zone families23/42/69/191 remain unsupported. Roof/bridge/occlusion and complete gameplay/visual parity remain unfinished.

## Preservation and external-agent work

Keep all operations/temp files within2004scape-LostCityRS. Preserve cache, history and every save, including stack211a/stack211b/bank211/bank212. Existing server PID3128 was confirmed owning43595/8889/8899 at this handoff; leave it running and verify ownership before any startup. Do not stop unrelated processes or restore the old launcher's save purge. No .env or test-account passwords are packaged.

The user clarified that Cursor/Grok partially attempted the external audit before exhausting usage. Its output has not been located or reviewed; the expected gemini-audit directory is empty. The user explicitly deferred searching. Do not search again without a new request or supplied path/output. No external findings were accepted here. GEMINI-AUDIT-TASK.md retains its historical filename but directs review of current implementation rather than duplicate work. Do not spawn GPT delegates. User requested Sol or Astra for continuation; no new task/model switch has been started.

---

'''.replace('NATIVE_HASH', native_sha)
old = (root / 'HANDOFF.md').read_text(encoding='utf-8')
write('HANDOFF.md', latest + old)
(root.parent / 'Server-main/HANDOFF.md').write_text(latest + old, encoding='utf-8')
progress = completion / 'progress.md'
progress.write_text('# Final current checkpoint\n\nRead the new top of HANDOFF.md and TEST-THIS-CHECKPOINT.md. Native SHA-256: `' + native_sha + '`. Latest complete package: handoff-checkpoint-2026-10-21.zip. Eight small changes built cleanly; tests/live checks were not rerun. No further code work is running. Cursor/Grok recovery deferred.\n\n' + progress.read_text(encoding='utf-8'), encoding='utf-8')
brief = root / 'GEMINI-AUDIT-TASK.md'
brief.write_text(brief.read_text(encoding='utf-8').replace('Updated October 21, 2026 after four small client changes.', 'Updated October 21, 2026 after eight small client changes.').replace('Current binary hash and files are pinned in the approach-flag run manifest.', 'Current binary hash and files are pinned in the minimap-click run manifest.'), encoding='utf-8')
plan = root / 'NEXT-STEPS-AND-GEMINI.md'
plan.write_text('Current handoff is now written at the top of HANDOFF.md. Latest complete package: handoff-checkpoint-2026-10-21.zip. No further implementation is running. Earlier usage/status wording below describes progress before finalization.\n\n' + plan.read_text(encoding='utf-8'), encoding='utf-8')

with zipfile.ZipFile(root / 'world-loops-test-checkpoint.zip') as z: names = set(z.namelist())
with zipfile.ZipFile(root / 'minimap-click-client-update.zip') as z: names.update(z.namelist())
names.update(['HANDOFF.md', 'TEST-THIS-CHECKPOINT.md', 'NEXT-STEPS-AND-GEMINI.md', 'GEMINI-AUDIT-TASK.md'])
for task in tasks:
    for p in (completion / ('runs/2026-10-21-' + task)).rglob('*'):
        if p.is_file() and p.suffix in {'.c', '.h', '.py', '.log', '.exit', '.md', '.json', '.diff'}:
            names.add(p.relative_to(root).as_posix())
for folder in ['src', 'include', 'platform', 'tests']:
    for p in (root / 'client-c' / folder).rglob('*'):
        if p.is_file() and p.suffix in {'.c', '.h', '.bin', '.json', '.md', '.txt', '.py', '.ts'}:
            names.add(p.relative_to(root).as_posix())
names.discard('world-loops-release-manifest.json')
assert all((root / n).is_file() for n in names)
assert all(not n.endswith('.sav') and '.env' not in Path(n).parts and 'engine' not in Path(n).parts for n in names)
manifest = {'date': '2026-10-21', 'native_sha256': native_sha, 'status': 'handoff_written', 'full_client_complete': False, 'current_build_exit': 0, 'current_build_warnings': 0, 'current_tests_rerun': False, 'current_live_checks_run': False, 'files': {n: sha(root / n) for n in sorted(names)}}
write('handoff-release-manifest.json', json.dumps(manifest, indent=2) + '\n')
names.add('handoff-release-manifest.json')
archive = root / 'handoff-checkpoint-2026-10-21.zip'
assert not archive.exists()
with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED) as z:
    for n in sorted(names): z.write(root / n, n)
with zipfile.ZipFile(archive) as z:
    assert z.testzip() is None
    for n, expected in manifest['files'].items():
        assert hashlib.sha256(z.read(n)).hexdigest().upper() == expected
write('handoff-checkpoint-2026-10-21.sha256', sha(archive) + '  ' + archive.name + '\n')
assert (root / 'HANDOFF.md').read_bytes() == (root.parent / 'Server-main/HANDOFF.md').read_bytes()
print(json.dumps({'package': str(archive), 'files': len(names), 'bytes': archive.stat().st_size, 'sha256': sha(archive), 'handoffs_identical': True}, indent=2))
