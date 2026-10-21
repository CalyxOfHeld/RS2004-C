"""Package only in-project sources, runtime and explicit verification evidence.

Run from client-c after the latest full suite and actual-client verification.

Does not launch servers, modify saves, or regenerate the baseline.

"""

from pathlib import Path

import hashlib,json,re,zipfile,shutil

c=Path.cwd().resolve();rw=c.parent;project=rw.parent

assert c.name=="client-c" and rw.name=="2004scape C Rewrite" and project.name=="2004scape-LostCityRS"

r=c/"verification/completion/runs/2026-10-20-foundation";root=c/"verification/completion"
f=root/"runs/2026-10-20-flicker"
g=root/"runs/2026-10-20-explicit-login"
h=root/"runs/2026-10-20-default-autologin"
i=root/"runs/2026-10-20-server-preflight"
j=root/"runs/2026-10-20-npc-minimap"
k=root/"runs/2026-10-20-audio-dialogue"

audio_dialogue=json.loads((k/"results.json").read_text())
assert audio_dialogue["tests_passed"]==35 and audio_dialogue["compiler_warnings"]==0
assert "warning:" not in (k/"build.log").read_text() and "error:" not in (k/"build.log").read_text()
assert "ALL AUDIO SYNTHESIZER & MIDI TESTS PASSED" in (k/"regression.log").read_text()
assert "ALL PLAYER HEAD & DIALOGUE TESTS PASSED" in (k/"regression.log").read_text()
assert "ALL BANK & INTERFACE ENHANCEMENT TESTS PASSED" in (k/"regression.log").read_text()

npc_minimap=json.loads((j/"results.json").read_text())
assert npc_minimap["cache_definitions"]==1017 and npc_minimap["hidden"]==84 and npc_minimap["live_connected"]
assert "ALL MINIMAP & COMPASS TESTS PASSED" in (j/"cache-regression.log").read_text()
assert "warning:" not in (j/"build.log").read_text() and "error:" not in (j/"build.log").read_text()

server_preflight=json.loads((i/"results.json").read_text())
assert len(server_preflight)==4 and all(check["result"]=="PASS" for check in server_preflight)

automatic_login=json.loads((h/"results.json").read_text())
assert automatic_login["play_script"]["live"] and automatic_login["test_batch_and_play_batch"]["live"]
assert "live=1" in (h/"default-live.log").read_text() and "live=1" in (h/"test-default-live.log").read_text()

startup_checks=json.loads((g/"results.json").read_text())
assert len(startup_checks)==7 and all(check["result"]=="PASS" for check in startup_checks)
startup_build=(g/"build.log").read_text()
assert "warning:" not in startup_build and "error:" not in startup_build

assert (rw/"HANDOFF.md").read_bytes()==(project/"Server-main/HANDOFF.md").read_bytes()

log=(f/"full-suite.log").read_text()

assert "ALL TESTS PASSED" in log and "warning:" not in log and "error:" not in log

build=(f/"fixed-build.log").read_text();assert "warning:" not in build and "error:" not in build

assert "live=1" in (r/"current-reference-view.log").read_text()
assert "live=1" in (r/"manual-test-launch.log").read_text()
assert "warning:" not in (r/"manual-test-launch.log").read_text()

assert "target=1531" in (r/"closed-door.log").read_text()
assert "live=1" in (f/"fixed-reset-walk.log").read_text()
assert "Assertion failed" in (f/"tester/baseline-reproduction-run.log").read_text()
assert json.loads((f/"tester/baseline-reproduction-result.json").read_text())["compile_exit"]==0
assert "PASS:" in (f/"tester/current-run.log").read_text()

files=[c/"bin/rs2client.exe",c/"bin/SDL2.dll",rw/"play_client.bat",rw/"play_client.ps1",rw/"test_client.bat",rw/"start_local_server.bat",rw/"check_local_server.ps1",rw/"MANUAL-TESTING.md",rw/"RENDER-FLICKER-FIX.md",rw/"HANDOFF.md",rw/"AUTONOMOUS_PLAN.md",rw/"FUNCTIONALITY-CHECKPOINT.md",c/"Makefile"]

for folder in ["src","include","platform","tests"]:

 files.extend(p for p in (c/folder).rglob("*") if p.is_file() and p.suffix in [".c",".h",".ps1",".bin"])

for name in ["progress.md","capabilities.md","capabilities-current.md","scenarios.md"]:files.append(root/name)

for p in (root/"reference/src").rglob("*.java"):files.append(p)

files.extend([root/"reference/isolation.patch",root/"reference/fixture-manifest.json",Path(__file__).resolve(),root/"tools/run_observed_server.ts"])

selected=["manual-launch-build.log","manual-launch.log","manual-launch.bmp.png","manual-test-launch.log","manual-test-launch.bmp.png","manual-network-failure.log","manual-transport-build.log","manual-transport.log","manual-server.log","manual-server-error.log","zone-release-tests.log","current-client-build.log","zone-regressions.log","collision-reference.log","world-transport.log","entity-regressions.log","trace-design.log","equipped-dagger.log","style-sword.log","unequipped.log","relog-unequipped.log","walk-authoritative.log","walk-verification.json","reference-scheduled.log","comparison.html","source-review.json","server-stop.txt","observed-server.log","observed-server-next.log","world-server.log","java-composed-client.png","java-viewport.png","java-mapback.png","java-sidebar.png","current-reference-view.log","current-reference-view.bmp.png","current-comparison-alignment.json","cook-talk.log","cook-talk.bmp.png","cook-continue.log","cook-continue.bmp.png","cook-choice.log","cook-choice.bmp.png","door-menu.log","door-menu.bmp.png","open-door.log","open-door.bmp.png","closed-door.log","closed-door.bmp.png"]

for name in selected:

 p=r/name;assert p.exists(),name;files.append(p)

files.extend(p for p in r.glob("*.input") if p.is_file())

files.extend(p for p in (r/"current-views").rglob("*") if p.is_file() and p.suffix in [".png",".txt",".log"])

files.extend(p for p in f.rglob("*") if p.is_file() and p.suffix in [".c",".png",".log",".json",".input",".txt",".html",".diff",".md"] and p.name!="release-manifest.json")
files.extend(p for p in g.rglob("*") if p.is_file() and p.suffix in [".py",".png",".log",".json",".diff",".md"])
files.extend(p for p in h.rglob("*") if p.is_file() and p.suffix in [".png",".log",".json",".diff",".md"])
files.extend(p for p in i.rglob("*") if p.is_file() and p.suffix in [".log",".json",".diff",".md"])
files.extend(p for p in j.rglob("*") if p.is_file() and p.suffix in [".png",".log",".json",".diff",".md"])
files.extend(p for p in k.rglob("*") if p.is_file() and p.suffix in [".png",".bmp",".log",".json",".diff",".md"])

files=sorted(set(files))

hashes=[{"path":p.relative_to(rw).as_posix(),"sha256":hashlib.sha256(p.read_bytes()).hexdigest(),"bytes":p.stat().st_size} for p in files]

manifest={"milestone":"Functional/authentic native client checkpoint — overall roadmap active/incomplete","native_executable_sha256":hashlib.sha256((c/"bin/rs2client.exe").read_bytes()).hexdigest(),"manual_startup":{"port":43595,"live_wrappers":["play_client.bat","test_client.bat"],"latest_capture":"manual-test-launch.bmp.png","closed_port_failure_exit":1,"transport_checks":"rebuilt and passed","full_suite":"historical manual-startup evidence; see current flicker full suite"},"flicker_validation":{"scope":"Shared model priorities/composite priority propagation and precise planar scene depth; no asset edits or model-ID exceptions", "regression":"preserved pre-fix actual-cache detail-mask failure; current pass in 28 yaw/shift views", "live":"40 actual SDL capture frames during two click-to-walk routes; 15 camera positions", "comparison":"Same world camera native before/after; original Java full parity not claimed", "full_suite":"Fresh full-suite.log, 33 programs including new independent regression", "assets":"Original revision-225 models, geometry and decoration retained", "supplementary_cache":"models1110/1134, 80 views, 22383 coplanar pixel samples; old source fails, current passes"},"tests":{"programs_exited_successfully":len(re.findall(r"^\s*@\./bin/",(c/"Makefile").read_text(),re.M)),"optional_standalone_live_login":"SKIP without disposable credentials; CRC checks run","compiler_warnings":0,"oracle":"33408 original Java CollisionMap reach decisions plus wall/footprint flags match","live":"Actual design/item/style/walk checks plus Cook dialogue/Continue/choice and ordinary door Open/Close, with production handler acceptance and received client state","visual":"Historical native/Java foundation comparison plus current native before/after and walking sequence. Settings/ticks differ; no exact parity claim"},"files":hashes}

manifest["explicit_account_startup"]={"verification":startup_checks,"scope":"Actual executable startup and offline SDL rendering; supplied credentials tested against a temporary loopback listener with no login backend. No new live gameplay login or full-suite rerun.","evidence":"client-c/verification/completion/runs/2026-10-20-explicit-login/"}
manifest["automatic_test_login"]={"verification":automatic_login,"evidence":"client-c/verification/completion/runs/2026-10-20-default-autologin/","scope":"Both play and compile/test launch paths automatically logged in to the existing local server with the existing default account; real account state and SDL captures. No credential prompt, offline mode or account reset."}
manifest["server_port_preflight"]={"verification":server_preflight,"scope":"Real temporary socket listeners for free/all-occupied/partial-conflict cases; actual start_local_server.bat against the already-running production server skipped a duplicate. No running server restarted.","evidence":"client-c/verification/completion/runs/2026-10-20-server-preflight/"}
manifest["npc_minimap_visibility"]={"verification":npc_minimap,"scope":"Original revision-225 client.drawMinimap and NpcEntity.isVisible conditions. All 1017 cache definitions exercised with original mapdot artwork. Actual live account capture; default account busy, existing user clients left open.","evidence":"client-c/verification/completion/runs/2026-10-20-npc-minimap/"}
manifest["audio_and_interactive_dialogue"]={"verification":audio_dialogue,"scope":"Authentic revision-225 software wave synth (22,050 Hz 8-bit unsigned PCM), 50Hz client tick sound delay queue, native Windows MCI MIDI engine, dynamic player head composite model rasterization, item dragging (threshold, cycles, alpha 128 floating icon, local swap), quantity count dialogs, and recursive scrollbars.","evidence":"client-c/verification/completion/runs/2026-10-20-audio-dialogue/"}
(r/"release-manifest.json").write_text(json.dumps(manifest,indent=2));files.append(r/"release-manifest.json")

package=rw/"functionality-authenticity-checkpoint.zip"

if package.exists():

 with zipfile.ZipFile(package) as old:

  exe_hash=hashlib.sha256(old.read("client-c/bin/rs2client.exe")).hexdigest()

 archive=root/"releases"/f"checkpoint-{exe_hash[:12]}.zip";archive.parent.mkdir(exist_ok=True)

 if not archive.exists():shutil.copy2(package,archive)

temp=rw/"functionality-authenticity-checkpoint.pending.zip"

with zipfile.ZipFile(temp,"w",zipfile.ZIP_DEFLATED,compresslevel=6) as z:

 for p in files:z.write(p,p.relative_to(rw).as_posix())

with zipfile.ZipFile(temp) as z:

 assert z.testzip() is None

 assert hashlib.sha256(z.read("client-c/bin/rs2client.exe")).hexdigest()==manifest["native_executable_sha256"]

temp.replace(package)

package_hash=hashlib.sha256(package.read_bytes()).hexdigest().upper()

(rw/"functionality-authenticity-checkpoint.sha256").write_text(package_hash+"  "+package.name+"\n")

print(json.dumps({"package":str(package),"bytes":package.stat().st_size,"entries":len(files),"sha256":package_hash,"native_sha256":manifest["native_executable_sha256"],"test_programs":manifest["tests"]["programs_exited_successfully"],"handoffs_identical":True},indent=2))

