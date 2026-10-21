"""Verify the isolated Java reference overlay.

Checks that reference/src/{deob/client,sign/signlink}.java differ from the
bundled originals only by the hunks recorded in reference/isolation.patch
(line endings ignored), and that every String constant in the compiled
reference/classes/deob/client.class equals the one in the original jar's
client.class apart from the hook-added strings.

Run from the rewrite root:  python client-c/verification/completion/tools/check_reference.py
"""
import difflib, os, subprocess, sys, tempfile, zipfile

ROOT = os.getcwd()
REF = os.path.join('client-c', 'verification', 'completion', 'reference')
JAR = os.path.join('javaclient', 'build', 'libs', 'rs2client.jar')
JDK = r'C:\Program Files\Eclipse Adoptium\jdk-8.0.504.1-hotspot\bin'
FILES = [('deob/client.java', 'deob.client'), ('sign/signlink.java', 'sign.signlink')]


def lines(path):
    text = open(path, 'rb').read().decode('utf-8').replace('\r\n', '\n')
    return [l + '\n' for l in text.split('\n')]


def changed(diff):
    return [l.rstrip('\n') for l in diff if l[:1] in '+-' and not l.startswith(('+++', '---'))]


def strings(classpath, name):
    out = subprocess.run([os.path.join(JDK, 'javap'), '-v', '-cp', classpath, name],
                         capture_output=True, check=True).stdout.decode('latin-1')
    return sorted(l.split('//', 1)[1].strip() for l in out.splitlines() if '= String ' in l)


def main():
    failures = []
    recorded = changed(open(os.path.join(REF, 'isolation.patch'), 'rb').read().decode('utf-8').splitlines())
    actual = []
    for rel, _ in FILES:
        original = os.path.join('javaclient', 'src', 'main', 'java', *rel.split('/'))
        isolated = os.path.join(REF, 'src', *rel.split('/'))
        actual += changed(difflib.unified_diff(lines(original), lines(isolated)))
    if actual != recorded:
        failures.append('isolated sources differ from original + isolation.patch')

    with tempfile.TemporaryDirectory() as tmp:
        with zipfile.ZipFile(JAR) as jar:
            jar.extract('deob/client.class', tmp)
        jar_strings = set(strings(tmp, 'deob.client'))
    ref_strings = set(strings(os.path.join(REF, 'classes'), 'deob.client'))
    added_by_hooks = {s for s in ref_strings - jar_strings}
    missing = jar_strings - ref_strings
    if missing:
        failures.append('compiled reference client lost original constants: ' +
                        '; '.join(sorted(s[:70] for s in missing)))
    print('hook-only constants:', len(added_by_hooks))
    for f in failures:
        print('FAIL', f)
    print('PASS reference overlay' if not failures else 'reference overlay check FAILED')
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
