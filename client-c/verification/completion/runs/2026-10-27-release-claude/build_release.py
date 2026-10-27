from pathlib import Path
import hashlib,json,shutil,subprocess,zipfile

root=Path.cwd().resolve()
name='2026-10-27-current-225-claude'
releases=root/'client-c/verification/completion/releases'
package=releases/name
if package.exists():raise SystemExit('Release directory already exists; preserve it.')
package.mkdir()
overlay=package/'workspace'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def put(relative):
    source=root/relative;target=overlay/relative
    target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(source,target)

# Entire native implementation, not just this task's edited files: normal make
# must not rebuild an old source tree over the supplied current executable.
native=[]
for folder in ['src','include','platform']:
    native += [p for p in (root/'client-c'/folder).rglob('*') if p.is_file()]
native += [root/'client-c/Makefile']
for p in native:put(p.relative_to(root))
for p in ['client-c/bin/rs2client.exe','client-c/bin/rs2replay.exe','client-c/bin/SDL2.dll',
          'test_client.bat','start_local_server.bat','check_local_server.ps1','play_client.bat','play_client.ps1',
          'README.md','MANUAL-TESTING.md','TEST-THIS-CHECKPOINT.md','RELEASE-CURRENT.md',
          'engine/data/pack/server/script.dat','engine/data/pack/server/script.idx']:
    put(p)
server=subprocess.check_output(['git','-C',str(root/'engine'),'diff','--name-only'],text=True).splitlines()
server=[p for p in server if p.startswith('src/') and p.endswith('.ts')]
content=subprocess.check_output(['git','-C',str(root/'content'),'diff','--name-only'],text=True).splitlines()
assert len(server)==11 and len(content)==3,(server,content)
for p in server:put('engine/'+p)
for p in content:put('content/'+p)
assert sha(overlay/'client-c/bin/rs2client.exe')=='ff10df0c1aa457d7ce15a087a6044b316f12a5c0bb5c04e039b8efe29c1a250f'
assert sha(overlay/'client-c/bin/rs2replay.exe')=='1da5cdc1674cea82358c0806fce00372c119535c76da9d31b187d7b865463caf'
assert 'CLIENT_OBJS = build/src/client/client225.o' in (overlay/'client-c/Makefile').read_text()
assert 'set "NODE_DEBUG=false"' in (overlay/'start_local_server.bat').read_text()

# Build-only source archive, with registered tests/fixtures and bundled SDL SDK.
sources=list(native)
for folder in ['tests','lib/SDL2/x86_64-w64-mingw32']:
    sources += [p for p in (root/'client-c'/folder).rglob('*') if p.is_file() and '__pycache__' not in p.parts]
sources += list((root/'client-c/verification/completion/tools').glob('*.h'))
sources += list((root/'client-c/verification/completion/tools').glob('*.c'))
sources=sorted(set(sources))
with zipfile.ZipFile(package/'source.zip','w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
    for p in sources:z.write(p,p.relative_to(root).as_posix())

evidence=package/'evidence';evidence.mkdir()
reports=['D-JAVA-AUDIO-OUTPUT.md','D-INPUT-PHASE.md','D-CHAT-DIAGNOSTICS.md','D-STAIR-ROUTE-GAPS.md',
         'D-CLUE-READ.md','D-RETAINED-RENDERER.md','D-QUEST-JOURNAL-SOURCES.md']
for p in reports:shutil.copyfile(root/'client-c/verification/completion'/p,evidence/p)
checks={
 'input-phase-sol':['reference.bin','native3.log','native3.exit','suite.exit','normal-smoke2.exit','identities.json'],
 'retained-renderer-sol':['camera-final2.log','camera-final2.exit','camera-native-final.exit','java-0-final.log','java-432-final.log','completion-audit.json'],
 'clue-read-sol':['normal-check.log','normal-check.exit','normal-pack.exit','identities.json'],
 'stair-routes-sol':['normal-routes.log','normal-routes.exit','normal-pack.exit','identities.json'],
 'exact-audio-sol':['worker-pcm.log','worker-pcm.exit','audio-final.exit','music-final2.log','wave-final2.log','identities.json'],
 'chat-diagnostics-sol':['behavior-final2.exit','compiler.exit'],
 'idle-logout-sol':[]}
for folder,files in checks.items():
    dest=evidence/folder;dest.mkdir()
    for filename in files:
        p=root/'client-c/verification/completion/runs'/('2026-10-26-'+folder)/filename
        if not p.is_file():raise RuntimeError('Missing evidence '+str(p))
        shutil.copyfile(p,dest/filename)
# October 27 original-Java frame parity on this exact binary.
parity=root/'client-c/verification/completion/runs/2026-10-27-current-parity-claude'
dest=evidence/'current-parity-claude';dest.mkdir()
shutil.copyfile(parity/'README.md',dest/'README.md')
shutil.copyfile(parity/'integrated-final/parity/parity-summary.txt',dest/'integrated-final-parity-summary.txt')
shutil.copyfile(parity/'tour-final/parity/parity-summary.txt',dest/'tour-final-parity-summary.txt')
shutil.copyfile(parity/'tour/input.src.txt',dest/'tour-input.txt')
suite=(parity/'suite-final.log').read_text(encoding='utf-8-sig',errors='replace')
assert 'ALL TESTS PASSED' in suite and 'suite exit 0' in suite
# The packaged binary adds only console-branding strings to a63f9ca4; both
# recordings were strictly replayed again on it.
rerun=root/'client-c/verification/completion/runs/2026-10-27-release-claude'
for kind in ['integrated','tour']:
    summary=rerun/('parity-'+kind+'-ff10df0c')/'parity-summary.txt'
    assert '== PARITY MATCH' in summary.read_text(encoding='utf-8-sig'),summary
    shutil.copyfile(summary,dest/(kind+'-packaged-binary-parity-summary.txt'))
(dest/'suite-final-tail.txt').write_text('\n'.join(suite.splitlines()[-6:])+'\n',encoding='utf-8')

(package/'README.md').write_text('''# Current revision-225 Windows release — October 27, 2026

Keep the usual workflow. In the rewrite workspace:

1. Run `start_local_server.bat`. Wait for **World ready**.
2. Run `test_client.bat`. It builds and launches the normal revision-225 client on **127.0.0.1:43595**.

There is no alternate candidate launcher. `play_client.bat` is available to run the already-built normal client. The normal Makefile now uses the same revision225 loop/render path as the qualified candidate.

## Existing working checkout

This checkout already has the fixes. No extraction or installation is needed here. If an older server is still running, use Ctrl+C in its own window, wait for shutdown, then start it again. Rebuilding the client does not replace loaded server code. Close an old client before rebuilding if Windows reports that its executable is locked.

## Applying this update to another copy of the same checkout

This is an overlay for the existing **2004scape C Rewrite** workspace, not a standalone game/server/cache distribution. Close that copy's client and stop its server normally. Back up that checkout, then copy the **contents of `workspace/`** into its rewrite root, preserving these relative paths. Keep its original cache, dependencies and player saves. Run the same two batch files above. The overlay includes native source so `test_client.bat` cannot rebuild stale client code over the included executable. It includes compiled server scripts so no manual script pack is required.

Required existing dependencies: the revision225 rewrite/server/content/cache checkout, its Bun/server dependencies, the configured Windows MSYS2 compiler for `test_client.bat`, and Windows audio support. No soundfont or MIDI runtime download is needed; music uses Windows' existing instrument bank. No account database, private key, .env file or save is included.

## Contents and checks

`workspace/` contains the current normal client/replay/SDL runtime, complete native implementation/build file, the eleven scoped server source fixes, three content script updates, current compiled server scripts, normal launchers and updated instructions. `source.zip` includes native build sources, registered tests/fixtures and the bundled SDL SDK. `evidence/` records the latest focused checks; see CHECKS.md and KNOWN-LIMITS.md.

Run `python VerifyPackage.py` in this package folder to verify every packaged file and source archive. After applying the overlay, `python VerifyPackage.py --workspace "C:/path/to/2004scape C Rewrite"` also compares the installed overlay files. It never starts a server or client.

To rebuild from the source archive, extract it into a new directory, enter its `client-c` folder using the existing compiler environment, and run `make bin/rs2client.exe bin/rs2replay.exe`. That is a source-build recipe. The new payload has verified identities, but the older October25 package's timestamp-normalized, mixed-compiler bit-identical clean rebuild was **not repeated** for this refresh; that old result is not relabelled as current evidence.

## October 27 changes

Three original-Java frame-parity corrections: the character-design model is no longer relit, the Wilderness "Level: N" skull and Arena indicator are drawn, and the camera pitch clamp starts at Java's 0. On this binary, an integrated new-account scenario (3099 frames) and a nine-location world tour (3849 frames) strictly replay and match the original Java client in every compared screen region. Console log messages now read RS2004-C (no visual or gameplay change). See evidence/current-parity-claude.

## Remaining listening check

Using your usual client and Windows speakers/headphones, confirm music starts, Off stops it, and enabled volume changes remain continuous and change loudness. Confirm sound effects play, Off silences them and restoring volume works. Change settings during an ordinary action and check that neither music nor effects stays stuck. Physical confirmation of the latest audio remains pending.

Older release ZIPs and their RunCandidate.ps1 instructions are superseded. Do not use them for the current fixes.
''',encoding='utf-8',newline='\n')
(package/'KNOWN-LIMITS.md').write_text('''# Current scope and limits

- Target: Windows, revision225 (May18,2004), bundled cache and server/content. Quest progress journals were introduced later and are correctly absent.
- Music uses continuous private Windows synthesis with original MIDI events and direct dB attenuation. It no longer rewrites CC7 or reopens the song for enabled-volume changes. Sound effects retain source PCM with playback gain. Physical listening remains pending. Exact historical external-host instrument tone and fade timing cannot be established from the Java client alone.
- Five stair placements have recovered routes using bundled map geometry and existing route patterns; exact historical landing tiles are not independently proven.
- All102 text-clue and6 map-clue displays remain unchanged. Oversized/missing clue text has defensive handling using existing message UI; this is not historically attested malformed-clue behavior.
- The retained synthetic renderer issue has been compared with Java. The native-only deferred-tile fatal check was already corrected. Later invalid synthetic topology is rejected by Java too. The populated camera regression now passes50 native draws. This is not exhaustive renderer or whole-world pixel equivalence.
- Input timing and the normal-build path are corrected. Latest focused controls and the registered suite pass. Earlier whole-scene, two-client and extended-session evidence retains its original scenarios/binary identities; it is not a fresh exhaustive run of every quest/location on this binary.
- Idle logout is enabled by the normal server launcher; combat may delay it. This does not make an AFK character invulnerable. Persistent server event logging is not configured by this update.
- This is an existing-workspace overlay. It does not contain a full server, original cache, compiler installation, account database, player saves, .env or private keys. Existing local account/login behavior is preserved by the normal launchers.
- Whole-frame Java parity on this binary covers two recorded scenarios (integrated new-account flow and a nine-location teleport tour). Wilderness combat, the Duel Arena, multi-player scenes and human play were not part of those recordings.
- File/archive integrity is freshly verified. No new claim of timestamp-normalized bit-identical clean-build reproduction is made for this package.
''',encoding='utf-8',newline='\n')
(package/'CHECKS.md').write_text('''# Evidence index

| Scope | Current evidence |
| --- | --- |
| Audio | D-JAVA-AUDIO-OUTPUT.md; real Windows private synth/held-note/cache playback and exact attenuation controls; source music/wave controls. Human listening pending. |
| Input and normal launcher build | D-INPUT-PHASE.md; native3, affected control families, registered suite and normal SDL startup. |
| Retained renderer fixture | D-RETAINED-RENDERER.md; Java/native topology comparison and50 populated camera draws. |
| Server chat diagnostics | D-CHAT-DIAGNOSTICS.md; actual handler routing and8033-script compilation. |
| Stair routes | D-STAIR-ROUTE-GAPS.md; compiled route/dialogue/collision checks and normal script pack. |
| Clue reading | D-CLUE-READ.md;108 existing displays unchanged, overflow/missing-input checks and normal script pack. |
| Current-binary frame parity | evidence/current-parity-claude; integrated (3099) and world tour (3849) frames match the original Java client; registered suite passed with 0 warnings. |
| Revision scope | D-QUEST-JOURNAL-SOURCES.md; historical correction excludes later quest progress journals. |

These reports explain the actual check boundaries. Earlier gate/whole-frame evidence is historical and is not promoted into a new full-game acceptance claim. Packaging verification checks delivery identities and workflow, not human audio or every gameplay action. Beyond the two October 27 parity recordings, no broad gameplay testing campaign was performed for this refresh.
''',encoding='utf-8',newline='\n')
(package/'LISTENING-STATUS.json').write_text(json.dumps({'status':'pending','applies_to':'current normal rs2client.exe','required':'Human music/effect start, Off, volume and continuity confirmation on Windows output device'},indent=2),encoding='utf-8')
(package/'build-manifest.json').write_text(json.dumps({'client_sha256':sha(overlay/'client-c/bin/rs2client.exe'),'replay_sha256':sha(overlay/'client-c/bin/rs2replay.exe'),'normal_object':'build/src/client/client225.o','source_zip_sha256':sha(package/'source.zip'),'source_files':len(sources),'server_files':server,'content_files':content,'compiled_script_count':8033,'clean_byte_identical_rebuild':'not repeated; do not inherit previous package claim'},indent=2),encoding='utf-8')
shutil.copyfile(root/'client-c/verification/completion/runs/2026-10-26-release-refresh-sol/VerifyPackage.py',package/'VerifyPackage.py')
manifest=[]
for p in sorted(package.rglob('*')):
    if p.is_file():manifest.append({'path':p.relative_to(package).as_posix(),'size':p.stat().st_size,'sha256':sha(p)})
(package/'package-files.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
print(json.dumps({'package':str(package),'payload_files':len(manifest),'source_files':len(sources)}))

# Archive, re-verify every member against the manifest, and write the sidecar.
archive=releases/(name+'.zip')
if archive.exists():raise SystemExit('Archive already exists; preserve it.')
files=sorted(p for p in package.rglob('*') if p.is_file())
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
    for p in files:z.write(p,p.relative_to(package).as_posix())
expected={v['path']:v['sha256'] for v in manifest}
with zipfile.ZipFile(archive) as z:
    assert z.testzip() is None
    names=z.namelist();assert len(names)==len(manifest)+1
    for n in names:
        if n!='package-files.json':assert hashlib.sha256(z.read(n)).hexdigest()==expected[n],n
side={'archive':archive.name,'sha256':sha(archive),'bytes':archive.stat().st_size,'members':len(names),
      'source_files':len(sources),'all_member_hashes_verified':True,
      'native_client_sha256':sha(overlay/'client-c/bin/rs2client.exe'),
      'native_replay_sha256':sha(overlay/'client-c/bin/rs2replay.exe'),
      'supersedes':'2026-10-26-current-225-sol.zip','physical_listening':'pending','clean_byte_identical_rebuild':'not repeated'}
(releases/(name+'.sha256.json')).write_text(json.dumps(side,indent=2),encoding='utf-8')
print(json.dumps(side,indent=2))
