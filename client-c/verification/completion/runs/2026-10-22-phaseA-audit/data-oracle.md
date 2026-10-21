# Decoder oracle (Phase A, data layer) — 2026-10-22

## What it compares

**`reference/src/DataOracle.java`** runs the unchanged revision-225 decoders on the bundled cache (`engine/data/pack/client`):
- The original jar is first on the classpath.
- The unpack order is exactly client.load's (`client.java:6438-6459`).
- Math.random() is seeded with 225 via `Math$RandomNumberGeneratorHolder`.
- It writes one `type id field=value` line for every instance field of:
  - every FloType, IdkType, SpotAnimType, VarpType, ObjType, LocType, NpcType, AnimBase, AnimFrame and SeqType;
  - every Model (`new Model(false, id)`) for each listed metadata id;
  - the four PixFonts;
  - every Component.
- Sprites are fingerprinted by size, crop and the SHA-256 of their pixels.

**`tests/test_data_reference.c`** writes the same lines from the native structs, with `math_random_seed(225)`.

**`tools/compare_data_reference.py`** compares the two dumps field by field.

**`tools/data_reference_digest.py`** reduces the Java dump to `tests/fixtures/data-reference.digest`: 27,984 per-object SHA-256 digests plus counts. The file is untracked and hashed in `tests/fixtures.sha256`.

**`test_data_reference --check`** (part of `make test`):
- recomputes the digests from the native decoders and reports each mismatching id;
- was shown to fail on a single corrupted digest (obj 1277);
- takes about 1.7 s.

## Regenerate

```
java -Djava.awt.headless=true -cp "javaclient/build/libs/rs2client.jar;client-c/verification/completion/reference/classes" DataOracle engine/data/pack/client %TEMP%/data/java.txt
python client-c/verification/completion/tools/data_reference_digest.py %TEMP%/data/java.txt client-c/tests/fixtures/data-reference.digest
client-c/bin/test_data_reference.exe engine/data/pack/client %TEMP%/data/native.txt 3554
python client-c/verification/completion/tools/compare_data_reference.py %TEMP%/data/java.txt %TEMP%/data/native.txt
```

The Java dump is 35.6 MB, sha256 `1cf3c7c0634e60ec…`. The native dump's sha256 is `dda58498b3b749c5…`. They differ only in the excluded fields below.

## First run: findings (Java is the source for every fix)

| # | Defect | Java | Fix |
|---|---|---|---|
| D1 | **seq delay**: 952 of 1,027 seqs kept 0 where Java resolves it. | `SeqType.decode`: delay 0 becomes `AnimFrame.instances[frame].delay`, then 1. | `seq_decode` resolves it at decode time. `sdl2_main.c` now unpacks AnimBase/AnimFrame before SeqType, as client.load does. `seq_get_delay` is a plain `delay[frame]` read like every Java consumer. |
| D2 | **flo hsl**: 60 of 78 differ. Native used `rand()/RAND_MAX`, a different generator that can return 1.0. | `Math.random()`, which is `java.util.Random.nextDouble()` on one shared instance. | New `core/jrandom.c`: a port of `java.util.Random` plus a shared `math_random()`. All four `rand()` sites (FloType.setColor, LocEntity random start, login ISAAC seed halves `(int)(Math.random()*9.9999999E7)`) now use it. Before this, native never called `srand`, so every session sent the same "random" seed halves. |
| D3 | **Empty models**: ids 596, 2214 and 2215 (0 vertices, 0 faces) loaded as NULL. They are used by obj 2529 and locs 83–85 and 771–776. | `Model.unpack` creates metadata for every id listed in `ob_head.dat`. | `ModelMeta.present`. `model_load` returns NULL only for unlisted ids. |
| D4 | **VarpType** absent: 295 varps, so `clientcode` (read by `updateVarp`, `client.java:4486`) was unavailable. | `VarpType.unpack/decode`. | New `config/varp_type.c`. It is decoded and verified, but not yet consumed (see N2). |
| D5 | **Component model presence**: native stored `model_id = 0` both for "none" and for model 0. | `model` is null unless the type-6 byte is nonzero. | -1 means none. See N1. |
| — | Oracle and dumper artifacts fixed along the way: walkmerge terminator length; type-7 `iops`; spotanim `seq` link (the oracle first unpacked SpotAnim before Seq). | | |

After the fixes, the comparator reports 0 failing checks, and all compared fields of all 27,984 entries are equal (`data-oracle-compare.txt`).

## Excluded fields (Java-only, confirmed default or covered)

- **model:** `labelFaces`, `labelVertices`, `min/max X/Y/Z`, `pickable`, `radius`, `vertexNormal(Original)`. Java fills these later in `createLabelReferences`, `calculateNormals` and the bounds code; all are null or 0 after `new Model(false,id)`. They need their own oracle in Phase C.
- **component:** `seqCycle` and `seqFrame` (0 after decode).
- **font:** `random` (the font's RNG object).
- **spotanim:** `index` (equals the id) and `seq` (native keeps the same link as `base`).

## New gaps found (not decoder bugs; queued for Phase D)

- **N1. Interface models never load from the cache.**
  - Java's `Component.decode` calls `getModel(id)`, so the 745 type-6 components with `model` and the 83 with `activeModel` have a Model.
  - Native draws `com->model` only when a packet sets it, and draws it raw. Java's `getModel(IIZ)` copies, transforms and runs `calculateNormals(64,768,-50,-10,-50)`.
  - To be ported together with drawInterface type 6 and the Phase C renderer.
- **N2. There is no native `updateVarp`** (`client.java:4484`).
  - VARP_SMALL/LARGE only store the value, so client codes such as brightness (clientcode 1 → `Pix3D.setBrightness`) and the music and sound settings never apply.
  - `varp_type.c` now provides the `clientcode` table it needs.
- **N3. `varp_init` invents initial state** (`src/ui/varp.c:24-36`): hitpoints 10/10, energy 100, combat level 3, and varps 20–25 = -1 ("all music tracks unlocked").
  - The Java client's fields start at 0 until the server sends them.
  - To be removed, or confined to the offline mode (F5), in Phase D.

## Live smoke (user's server, PID 2352, port 43595, left running)

- New disposable account `claudesm22`, `live-smoke/`.
- Saves were snapshotted first (`live-smoke/saves-before/`, 38 files).
- Result: boot, then login reply 18 (the server accepted the ISAAC stream built from the new `math_random` seed halves), then REBUILD_NORMAL. The character-design interface renders with the player animating (seq 808, frame 5). Exit 0.
- The capture frame ends the run, so the scripted logout did not run. Logout is unchanged since the logout-title checkpoint.
- Previous installed build `0510EADD…` is kept as `bin/rs2client.lastgood.exe`.
