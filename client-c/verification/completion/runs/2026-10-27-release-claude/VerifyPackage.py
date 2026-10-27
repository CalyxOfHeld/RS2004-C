"""Read-only integrity/workflow verification; no process or installation action."""
from pathlib import Path,PurePosixPath
import argparse,hashlib,json,struct,zipfile
p=argparse.ArgumentParser();p.add_argument('--workspace',type=Path);args=p.parse_args()
root=Path(__file__).resolve().parent
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
manifest=json.loads((root/'package-files.json').read_text(encoding='utf-8'))
expected={v['path'] for v in manifest}|{'package-files.json'}
actual={p.relative_to(root).as_posix() for p in root.rglob('*') if p.is_file()}
assert actual==expected,('Unexpected/missing files',actual^expected)
for v in manifest:
    path=PurePosixPath(v['path']);assert not path.is_absolute() and '..' not in path.parts
    f=root/v['path'];assert f.stat().st_size==v['size'] and sha(f)==v['sha256'],v['path']
    assert f.suffix.lower() not in {'.env','.pem','.key','.sav','.sqlite'},v['path']
    if args.workspace and v['path'].startswith('workspace/'):
        installed=args.workspace/v['path'][10:];assert installed.is_file() and sha(installed)==v['sha256'],str(installed)
build=json.loads((root/'build-manifest.json').read_text(encoding='utf-8'))
assert sha(root/'source.zip')==build['source_zip_sha256']
with zipfile.ZipFile(root/'source.zip') as z:
    assert z.testzip() is None
    for name in z.namelist():
        p=PurePosixPath(name);assert not p.is_absolute() and '..' not in p.parts
        assert not any(x in p.parts for x in ['node_modules','.git','saves'])
    for f in (root/'workspace/client-c').rglob('*'):
        if f.is_file() and 'bin' not in f.relative_to(root/'workspace/client-c').parts:
            assert z.read(f.relative_to(root/'workspace').as_posix())==f.read_bytes(),str(f)
workspace=root/'workspace'
assert 'make bin/rs2client.exe' in (workspace/'test_client.bat').read_text()
assert 'CLIENT_OBJS = build/src/client/client225.o' in (workspace/'client-c/Makefile').read_text()
assert 'set "NODE_DEBUG=false"' in (workspace/'start_local_server.bat').read_text()
assert not (root/'RunCandidate.ps1').exists()
assert sha(workspace/'client-c/bin/rs2client.exe')==build['client_sha256']
idx=(workspace/'engine/data/pack/server/script.idx').read_bytes()
assert sum(n>0 for (n,) in struct.iter_unpack('>I',idx[4:]))==8033
print(json.dumps({'status':'PASS','files':len(manifest),'usual_workflow':'start_local_server.bat -> test_client.bat','workspace_compared':bool(args.workspace),'physical_listening':'pending','client_sha256':build['client_sha256']},indent=2))
