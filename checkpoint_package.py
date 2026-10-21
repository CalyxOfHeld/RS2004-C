from pathlib import Path
import hashlib
import json
import zipfile

root = Path(__file__).resolve().parent
run = root / 'client-c/verification/completion/runs/2026-10-21-world-loops'
def write(path, text):
    (root / path).write_text(text, encoding='utf-8')
def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest().upper()

native_sha = sha(root / 'client-c/bin/rs2client.exe')
assert native_sha == '3035DF94DAFA31A55C038CB1F25DEC795118BD7513EBA85AA463B3ACF0077919'

write('TEST-THIS-CHECKPOINT.md', '''# Windows client checkpoint — October 21, 2026

Feature work is paused for your testing. This is a working checkpoint; the full native client is still incomplete.

## Launch

The current executable is already installed in this checkout. Double-click `play_client.bat` in `2004scape C Rewrite`. The existing local server was listening on port 43595 when packaged. If it is no longer running, run `start_local_server.bat` and wait for World ready first.

The launcher automatically uses the existing local test account and keeps its progress. For another account, see MANUAL-TESTING.md. Close another client using that account before launching; login code 5 means already logged in or pending logout. Close the client window to exit.

The ZIP is an update for this existing checkout: extract its contents directly into `2004scape C Rewrite`, allowing replacement of matching client/source/document files. It requires this checkout's existing engine, content and cache. It contains no player saves or .env files. Do not use older checkpoint ZIPs: one historical package contained a withdrawn save-purge launcher.

## Suggested checks

1. Walk on the main scene; rotate the camera with arrow keys. Confirm inventory and equipment match the account.
2. Drop and take an item you can spare; check inventory quantities and the ground pile.
3. Talk to a banker and open the bank. Deposit/withdraw an item and coins using X, drag a bank slot, close and reopen. Confirm quantities and the normal inventory sidebar return.
4. Trade with a shopkeeper, buy and sell one ordinary item; check coins, inventory and stock.
5. Use available stairs up and down; check the scene and minimap change together. Try normal doors and dialogue choices.

Tutorial restrictions, available items and NPC dialogue depend on the real account and server state. No fixture commands are required for ordinary testing. Minimap click navigation, public chat and full combat/skilling coverage remain unfinished. Escape closes active menus/interfaces and otherwise selects the inventory tab; use window close to exit.

For a bug report, provide the account name (never password), location, action, expected result, actual result, and a screenshot or console log. Do not delete saves to reproduce a problem.

## Evidence

The packaged executable passed the recorded 37-program suite with zero compiler warnings, plus separate instrumented live SDL scenarios for ground quantity/reveal, bank X/drag, natural bank dialogue, shop buy/sell and stairs. These are bounded checks, not full gameplay certification. See the world-loops run README for exact scope and failed probes. Audio matches the original Java output for 435 sounds at two loop counts with a shared noise input; device timing and MIDI remain unverified.

See NEXT-STEPS-AND-GEMINI.md for the continuation plan and GEMINI-AUDIT-TASK.md for a ready-to-copy bounded assignment. The ZIP checksum is in world-loops-test-checkpoint.sha256.
''')

write('NEXT-STEPS-AND-GEMINI.md', '''# Paused checkpoint and next move

Paused October 21, 2026 at the user's request. No further feature implementation is running. The server is left available for manual testing. Last account usage check: 71% of the five-hour window used, 11% weekly. Write another runnable handoff at 95% used / 5% remaining if work resumes and reaches that threshold. The present handoff is written now.

## Resume order

1. Read the user's checkpoint feedback and reproduce/fix reported failures first. Preserve all accounts, history, cache and server behavior.
2. Complete minimap navigation from the bundled revision-225 reference: inverse click transform, original route serialization, destination marker/clear behavior, then natural route, cancellation and region-crossing evidence. Only preparation snapshots and a Makefile build refactor were made for this phase; minimap navigation has not been implemented.
3. Close remaining item-use/spell and connected combat/skilling/social/effect gaps in dependency order from AUTONOMOUS_PLAN.md. Unsupported zone families 23/42/69/191 need source-backed coverage.
4. Continue original-render comparisons for roofs, bridges, occlusion and ground piles, then sustained session checks. Passing the current suite does not establish complete parity.

## Work suitable for Gemini

These are bounded assignments with outputs that can be checked against local source. No Gemini session has been started and no GPT delegate has been used for this assessment. Model choice alone does not establish correctness.

| Assignment | Deliverable | Review boundary |
| --- | --- | --- |
| Protocol/interface coverage audit | CSV mapping original Java, server codecs and native handlers with exact file/line references | Read-only audit; mark unknowns explicitly |
| Checkpoint manual QA | Reproduction steps, screenshots/logs and observed quantities for bank/shop/doors/stairs | Separate disposable account; no save edits or server changes |
| Evidence/document audit | Scenario-to-evidence index, stale claims and proposed corrections | Distinguish actual server acceptance from fixture preparation |
| Cache/definition inventory | Missing item/NPC/animation coverage report from bundled real definitions | No invented assets or later-revision references |
| Small regression proposals | Boundary cases and independently derived expected values for a defined helper | Review before adding code; do not copy implementation as the oracle |

Start with the protocol audit in GEMINI-AUDIT-TASK.md. Give one narrow assignment at a time. Keep encryption/framing, entity rebasing, renderer ordering and audio arithmetic under close implementation review because errors can affect many systems. A later coding assignment should name one function, its reference behavior, allowed files and acceptance criteria.

## Checkpoint boundaries

Native executable SHA-256: 3035DF94DAFA31A55C038CB1F25DEC795118BD7513EBA85AA463B3ACF0077919.
The executable is the one used for the accepted live scenarios. The later Makefile refactor compiled shared objects once and passed the same 37-test suite; the packaged executable was retained rather than rebuilt after pausing.
Existing server PID 3128 owned ports 43595/8889/8899 at packaging. Verify ownership again before starting any server. New test saves stack211a, stack211b, bank211 and bank212 are persistent and must be preserved too.
No Git remote is configured; delivery is the local test ZIP and installed executable. No commit or remote push was made.
''')

write('GEMINI-AUDIT-TASK.md', '''# Gemini assignment: revision-225 minimap protocol audit

Produce a read-only audit to prepare the next native client task. Do not implement features. Read AGENT.md, the current top of HANDOFF.md, NEXT-STEPS-AND-GEMINI.md, and AUTONOMOUS_PLAN.md first.

Use only the bundled revision-225 Java client, TypeScript port and engine/content references. Do not use OSRS, later revisions, web guesses or generated behavior. Keep all access and outputs inside this workspace. Do not start agents or delegate to GPT.

## Scope

Trace main-scene walking versus minimap walking through original input handling, click-coordinate transformation, camera rotation/zoom, route selection, serialization and destination marker clearing. Locate the exact original/server/native definitions rather than trusting opcode numbers in prose. Explain where each extra minimap payload byte originates and which handler validates it. Identify the native input/render hooks still missing. Do not broaden into combat or rewrite routing.

## Deliverables

Write only under `client-c/verification/completion/gemini-audit/`:

- `minimap-coverage.csv`: behavior, direction, opcode, length/framing, original Java reference, server reference, native reference, implemented/partial/missing/unverified, existing evidence, open question.
- `minimap-findings.md`: five or fewer prioritized gaps, exact source lines, a small implementation proposal and acceptance scenarios. Separate source-derived expectations from behavior actually observed.

Do not edit production code, saves, cache, launchers, .env files or credentials. Do not start/stop servers or clients. Do not claim execution or successful tests without logs. If a reference is absent or ambiguous, record the precise unresolved point. The main implementer will review every mapping and expected value before using it.
''')

manual = (root / 'MANUAL-TESTING.md').read_text(encoding='utf-8')
manual = manual.replace('Escape closes an active interface/menu first, then exits when none is open. Closing the window also exits.', 'Escape closes an active interface/menu first; otherwise it selects the inventory tab. Close the window to exit.')
manual = manual.replace('Ground-item synchronization and Drop/Take have a bounded live dagger-loop verification.', 'See TEST-THIS-CHECKPOINT.md for bank X/drag, shop buy/sell, ground quantity/reveal and stair checks. These have bounded recorded live scenarios.')
write('MANUAL-TESTING.md', manual)

handoff = (root / 'HANDOFF.md').read_text(encoding='utf-8')
handoff = handoff.replace('# Handoff: revision-225 native client — active completion roadmap', '# Handoff: revision-225 native client — paused for user testing', 1)
handoff = handoff.replace('No updated ZIP yet; launch the in-place client.', 'Test package: `world-loops-test-checkpoint.zip`, checksum in its `.sha256` sidecar. This updates the existing checkout; it is not a standalone server distribution. Launch the installed `play_client.bat`; see TEST-THIS-CHECKPOINT.md.', 1)
start = handoff.index('ACTIVE next work:') if 'ACTIVE next work:' in handoff else handoff.index('PAUSED at the user')
end = handoff.index('\n\n', start)
handoff = handoff[:start] + 'PAUSED at the user\'s request for a testable client, next-step plan and Gemini assessment. No feature work is running. Read NEXT-STEPS-AND-GEMINI.md and GEMINI-AUDIT-TASK.md. Resume with user-reported failures, then minimap navigation and the remaining roadmap. User requests a runnable handoff at95% usage/5% remaining; this handoff is written early at the requested pause. Last check:71% five-hour and11% weekly used. No Git remote is configured; the local ZIP is the delivery, with no remote push or commit.' + handoff[end:]
write('HANDOFF.md', handoff)
(root.parent / 'Server-main/HANDOFF.md').write_text(handoff, encoding='utf-8')

write('client-c/verification/completion/progress.md', '''# Current progress — October 21, 2026

PAUSED at the user's request for manual testing. Read the top of HANDOFF.md, TEST-THIS-CHECKPOINT.md and NEXT-STEPS-AND-GEMINI.md. Full functionality and original visual parity remain incomplete.

Verified checkpoint: live ground quantity/reveal, modal/sidebar separation, bank Deposit/Withdraw X and server-confirmed drag, natural banker dialogue/player portrait, natural shop Buy/Sell, and ordinary stairs up/down. Region rebasing retains received actor positions and path queues; plane changes rebuild minimap. Fixture teleports/preparation establish bounded scenarios, not natural travel or tutorial completion.

Audio: 435 actual cache sounds at loop counts1/2 match 20,359,589 original Java WAV bytes with a shared noise input. Fixed five-harmonic terminator desynchronization, invalid track bounds and invalid-loop duration arithmetic. MIDI/device timing/random-generator parity remain unverified.

All37 programs passed with zero warnings: world-loops/full-suite-audio-fixed.log and navigation/incremental-suite.log, each with exit0. The latter also covers the Makefile refactor to shared objects/dependency files. No minimap navigation feature was implemented. The retained executable SHA is in HANDOFF.md and the release manifest.

Detailed scenarios, failed probes, reference provenance and lossless PNG captures: runs/2026-10-21-world-loops/README.md. History from earlier work remains in HANDOFF.md and prior run directories. The progress file was found empty during packaging; this summary reconstructs current status from those records.

Preserve every existing and new account, cache and history. Server PID3128 on43595/8889/8899 was left running; verify listeners before startup. Next: user feedback, minimap navigation, remaining item-use/spell/combat/skilling/social/effect gates, renderer comparisons and sustained sessions. Gemini's first bounded assignment is a read-only minimap protocol audit; no delegation has been started.
''')

readme = (run / 'README.md').read_text(encoding='utf-8')
if not readme.startswith('# Paused test delivery'):
    readme = '# Paused test delivery\n\nFeature work paused at the user\'s request. The tested executable and selected evidence are packaged in world-loops-test-checkpoint.zip. See the root test guide and continuation plan. Any earlier future-tense checkpoint/finalization notes below are superseded by this delivery. Full parity remains incomplete.\n\n' + readme
if '## Separate natural stair descent' not in readme:
    readme += '\n## Separate natural stair descent\n\n`stairs-live.input/log` accepted staircase1738 Climb-up from plane0 to1. The later appended descent events in that file were beyond its capture limit and were not executed. `stairs-down.input/log` is the separate accepted staircase1739 Climb-down operation from plane1 to0. Both update the actual server position and minimap.\n'
(run / 'README.md').write_text(readme, encoding='utf-8')
results = json.loads((run / 'results.json').read_text(encoding='utf-8'))
results['checkpoint'] = 'Paused for user testing; world/interface/audio checkpoint'
(run / 'results.json').write_text(json.dumps(results, indent=2) + '\n', encoding='utf-8')

before = json.loads((run / 'saves-before.json').read_text(encoding='utf-8-sig'))
preservation = [{'name': Path(x['Path']).name, 'exists': Path(x['Path']).is_file(), 'unchanged': Path(x['Path']).is_file() and sha(Path(x['Path'])) == x['Hash']} for x in before]
(run / 'save-preservation.json').write_text(json.dumps(preservation, indent=2) + '\n', encoding='utf-8')

files = set()
def add(relative):
    p = root / relative
    assert p.is_file(), relative
    files.add(p)
for name in ['AGENT.md', 'HANDOFF.md', 'AUTONOMOUS_PLAN.md', 'MANUAL-TESTING.md', 'TEST-THIS-CHECKPOINT.md', 'NEXT-STEPS-AND-GEMINI.md', 'GEMINI-AUDIT-TASK.md', 'LICENSE', 'play_client.bat', 'play_client.ps1', 'test_client.bat', 'test_suite.bat', 'start_local_server.bat', 'check_local_server.ps1', 'client-c/Makefile', 'client-c/bin/rs2client.exe', 'client-c/bin/SDL2.dll']:
    add(name)
for folder in ['client-c/src', 'client-c/include', 'client-c/platform', 'client-c/tests', 'client-c/verification/completion/reference/src']:
    for p in (root / folder).rglob('*'):
        if p.is_file() and p.suffix.lower() in {'.c', '.h', '.bin', '.json', '.java', '.md', '.txt', '.ts', '.py'}:
            files.add(p)
for name in ['progress.md', 'capabilities.md', 'capabilities-current.md', 'scenarios.md']:
    add('client-c/verification/completion/' + name)
for name in ['README.md', 'results.json', 'save-preservation.json', 'source-review.diff', 'audio-reference-manifest.json', 'audio-guard-test.log', 'full-suite-audio-fixed.log', 'full-suite-audio-fixed.exit', 'accepted-quantity.log', 'accepted-reveal.log', 'owner-quantity.input', 'observer.input', 'bank-deposit.input', 'bank-deposit.log', 'bank-withdraw-drag.input', 'bank-withdraw-drag.log', 'banker-live.input', 'banker-live.log', 'shop-target.input', 'shop-target.log', 'stairs-live.input', 'stairs-live.log', 'stairs-down.input', 'stairs-down.log', 'travel.input', 'travel.log']:
    add(str((run / name).relative_to(root)))
for prefix in ['accepted-quantity', 'accepted-reveal', 'bank-deposit', 'bank-withdraw-drag', 'banker-live', 'shop-target', 'stairs-live', 'stairs-down']:
    capture = run / (prefix + '.bmp.png')
    if not capture.exists():
        capture = sorted(run.glob(prefix + '.bmp.frame-*.bmp.png'))[-1]
    add(str(capture.relative_to(root)))
for name in ['incremental-suite.log', 'incremental-suite.exit']:
    add('client-c/verification/completion/runs/2026-10-21-navigation/' + name)

manifest = {'date': '2026-10-21', 'status': 'paused_for_user_testing', 'native_sha256': native_sha, 'full_client_complete': False, 'suite_programs': 37, 'suite_exit': 0, 'compiler_warnings': 0, 'existing_checkout_required': True, 'save_preservation': preservation, 'files': {p.relative_to(root).as_posix(): sha(p) for p in sorted(files)}}
write('world-loops-release-manifest.json', json.dumps(manifest, indent=2) + '\n')
add('world-loops-release-manifest.json')
package = root / 'world-loops-test-checkpoint.zip'
assert not package.exists(), 'Preserve an existing release rather than overwrite it'
with zipfile.ZipFile(package, 'w', zipfile.ZIP_DEFLATED, compresslevel=6) as z:
    for p in sorted(files):
        rel = p.relative_to(root).as_posix()
        assert not any(part in {'engine', '.env', 'node_modules', 'saves'} for part in p.relative_to(root).parts)
        z.write(p, rel)
with zipfile.ZipFile(package) as z:
    assert z.testzip() is None
    for rel, expected in manifest['files'].items():
        assert hashlib.sha256(z.read(rel)).hexdigest().upper() == expected, rel
write('world-loops-test-checkpoint.sha256', sha(package) + '  ' + package.name + '\n')
print(json.dumps({'package': str(package), 'bytes': package.stat().st_size, 'files': len(files), 'sha256': sha(package), 'original_saves_unchanged': all(x['unchanged'] for x in preservation)}, indent=2))
