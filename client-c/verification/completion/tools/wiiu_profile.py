"""Summarise a Wii U samples.bin (platform/sdl2_main.c wiiu_profile_*).

usage: python wiiu_profile.py samples.bin rs2004c.elf [top]

samples.bin: magic 'RPRF', runtime address of wiiu_profile_reference, count,
then count (pc, lr) big-endian pairs. The ELF must be the exact build that ran.
Prints the hottest functions by PC (self time) and, for the hottest leaf
functions, the callers seen in LR.
"""
import bisect
import collections
import struct
import subprocess
import sys

NM = r"C:/devkitPro/devkitPPC/bin/powerpc-eabi-nm"


def symbols(elf):
    out = subprocess.run([NM, "-n", "--defined-only", elf], capture_output=True, text=True, check=True).stdout
    addrs, names = [], []
    for line in out.splitlines():
        parts = line.split()
        if len(parts) == 3 and parts[1] in "tTwW":
            addrs.append(int(parts[0], 16))
            names.append(parts[2])
    return addrs, names


def main():
    samples_path, elf = sys.argv[1], sys.argv[2]
    top = int(sys.argv[3]) if len(sys.argv) > 3 else 40
    data = open(samples_path, "rb").read()
    magic, reference, count = struct.unpack(">III", data[:12])
    if magic != 0x52505246:
        sys.exit("not a samples.bin")
    pairs = struct.iter_unpack(">II", data[12:12 + 8 * count])
    addrs, names = symbols(elf)
    offset = reference - addrs[names.index("wiiu_profile_reference")]

    def name(address):
        a = address - offset
        i = bisect.bisect_right(addrs, a) - 1
        if i < 0 or a - addrs[i] > 0x100000:
            return "[outside %08x]" % address
        return names[i]

    self_counts = collections.Counter()
    callers = collections.defaultdict(collections.Counter)
    for pc, lr in pairs:
        fn = name(pc)
        self_counts[fn] += 1
        callers[fn][name(lr)] += 1
    print("samples %d (1 kHz), load offset %+#x" % (count, offset))
    print("%6s %6s  function" % ("count", "%"))
    for fn, n in self_counts.most_common(top):
        print("%6d %5.1f%%  %s" % (n, 100.0 * n / count, fn))
    print("\ncallers (by LR) of the top 12:")
    for fn, n in self_counts.most_common(12):
        seen = ", ".join("%s %d" % (c, k) for c, k in callers[fn].most_common(5) if c != fn)
        print("  %s: %s" % (fn, seen or "(LR within itself)"))


if __name__ == "__main__":
    main()
