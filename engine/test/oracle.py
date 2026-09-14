"""
oracle.py - prints the same transcript as ./test_ori, from origami.py.

    python3 oracle.py       all stages
    python3 oracle.py 2     stage T2 only

origami.py is NOT modified. Two adjustments are made here instead, and
both are decisions that belong in the requirements document:

  1. Signed zero. intersect() produces -0.0 for the corner at the origin,
     so State.dump() emits a mix of -0.000000000 and +0.000000000
     depending on the sign of a determinant. f9() below prints anything
     that would round to zero as +0.000000000. ori_dump() does the same.
     Recommended: apply the same rule inside State.dump().

  2. Out-of-range indices. Requirements 5.1 says an axiom returns 0.
     Python raises IndexError and wraps negative indices silently, so the
     reference cannot answer these. The expected values are hardcoded.
"""
import math
import sys
from itertools import combinations

import origami
from origami import (State, canon, intersect, line_through, perp_bisector,
                     lkey, axiom1, axiom2, axiom3, axiom4, axiom7)

SQRT2 = math.sqrt(2.0)


def f9(v):
    return f"{0.0 if abs(v) < 5e-10 else v:+.9f}"


def pl(tag, sols):
    parts = "".join(f" | {f9(l[0])} {f9(l[1])} {f9(l[2])}" for l in sols)
    print(f"{tag} -> {len(sols)}{parts}")


def dump_state(st):
    ls = "\n".join(f"L {i} {f9(l[0])} {f9(l[1])} {f9(l[2])}"
                   for i, l in enumerate(st.lines))
    ps = "\n".join(f"P {i} {f9(p[0])} {f9(p[1])}"
                   for i, p in enumerate(sorted(st.points)))
    print(f"W {st.w:.9f} H {st.h:.9f}")
    print(ls)
    print(ps)
    print(f"check {1 if st.check() else 0}")
    print(f"fold_count {len(st.lines) - 4}")


def undo(st):
    if len(st.lines) <= 4:
        return 0
    st.lines.pop()
    st._rebuild()
    return 1


# ----------------------------------------------------------------- T0
def t0():
    print("### T0 new(1, sqrt2)")
    st = State(1.0, SQRT2)
    dump_state(st)
    print(f"undo {undo(st)}")
    print()


# ----------------------------------------------------------------- T1
CANON = [(0, 1, 0), (0, -1, 0), (1, 0, 0), (-1, 0, -1),
         (3, 4, 5), (-3, -4, -5), (0, 0, 1), (1e-13, 1e-13, 1),
         (1e-10, -1, 0), (-0.0, 1, 0), (2, 0, 6)]
PAIRS = [((0, 0), (1, 0)), ((1, 0), (0, 0)), ((0, 0), (0, 1)),
         ((0, 0), (1, 1)), ((0.25, 0.5), (0.25, 0.5)), ((1, 2), (3, 4))]
INSIDE = [(0.5, 0.5), (-1e-8, 0.5), (-1e-6, 0.5),
          (1.0, SQRT2), (1.5, 0.5), (0.5, SQRT2 + 1e-8)]
CHORD = [(0, 1, SQRT2 / 2), (1, 0, 0.5), (-SQRT2, 1, 0), (0, 1, -1), (1, 1, 0)]


def t1():
    print("### T1 primitives")
    for i, (a, b, c) in enumerate(CANON):
        l = canon(a, b, c)
        pl(f"canon {i:02d}", [l] if l else [])

    st = State(1.0, SQRT2)
    for i, j in combinations(range(len(st.lines)), 2):
        q = intersect(st.lines[i], st.lines[j])
        tail = f" | {f9(q[0])} {f9(q[1])}" if q else ""
        print(f"intersect {i} {j} -> {1 if q else 0}{tail}")

    for i, (a, b) in enumerate(PAIRS):
        l = line_through(a, b)
        pl(f"through {i:02d}", [l] if l else [])
        l = perp_bisector(a, b)
        pl(f"bisector {i:02d}", [l] if l else [])

    for i, p in enumerate(INSIDE):
        print(f"inside {i:02d} -> {1 if st.inside(p) else 0}")

    for i, (a, b, c) in enumerate(CHORD):
        l = canon(a, b, c)
        if l is None:
            print(f"chord {i:02d} -> degenerate")
        else:
            print(f"chord {i:02d} -> {f9(st.chord(l))}")

    for i, (x, y) in enumerate([(0.9, 0.1), (0.5, 0.5), (0.9, 1.2)]):
        k, d = st.nearest_point(x, y)
        print(f"nearest {i} -> {k} {f9(d)}")
    print()


# ----------------------------------------------------------------- T2
def t2():
    print("### T2 axioms on the initial state")
    st = State(1.0, SQRT2)
    nP, nL = len(st.points), len(st.lines)

    for i, j in combinations(range(nP), 2):
        pl(f"O1 {i} {j}", axiom1(st, i, j))
    for i, j in combinations(range(nP), 2):
        pl(f"O2 {i} {j}", axiom2(st, i, j))
    for i in range(nL):
        for j in range(i, nL):
            pl(f"O3 {i} {j}", axiom3(st, i, j))
    for p in range(nP):
        for l in range(nL):
            pl(f"O4 {p} {l}", axiom4(st, p, l))
    for p in range(nP):
        for x in range(nL):
            for y in range(nL):
                pl(f"O7 {p} {x} {y}", axiom7(st, p, x, y))

    for tag in ["O1 -1 0", "O1 0 99", "O2 -1 -1", "O3 0 -5",
                "O4 99 0", "O4 0 99", "O7 0 -1 0"]:
        print(f"oor {tag} -> 0")
    print()


# ----------------------------------------------------------------- T3/T4
def pidx(st, x, y):
    i, d = st.nearest_point(x, y)
    if i < 0 or d > 1e-9:
        print(f"!! no point at {f9(x)} {f9(y)} (nearest {i} at {d:.3e})")
    return i


def step(st, k):
    H = SQRT2
    if k == 1: return "O2", axiom2(st, pidx(st, 0, 0), pidx(st, 0, H))
    if k == 2: return "O1", axiom1(st, pidx(st, 0, 0), pidx(st, 1, H))
    if k == 3: return "O2", axiom2(st, pidx(st, 0, H), pidx(st, 1, 0))
    if k == 4: return "O3", axiom3(st, 0, 4)
    if k == 5: return "O2", axiom2(st, pidx(st, 0, 0), pidx(st, 1, H / 2))
    return "??", []


def t3():
    print("### T3 the five-fold reference sequence")
    st = State(1.0, SQRT2)
    for k in range(1, 6):
        name, sols = step(st, k)
        if not sols:
            print(f"fold {k}: {name} returned 0 solutions, stopping")
            break
        idx = st.fold(sols[0])
        print(f"fold {k} {name} -> index {idx}  L {len(st.lines)}  "
              f"P {len(st.points)}  check {1 if st.check() else 0}")
    print("--")
    dump_state(st)
    print(f"undo {undo(st)}")
    print(f"after undo  L {len(st.lines)}  P {len(st.points)}  "
          f"fold_count {len(st.lines) - 4}  check {1 if st.check() else 0}")
    print()


def t4_one(st, depth):
    have = {lkey(l) for l in st.lines}
    per = [dict() for _ in range(5)]
    uni = {}
    nP, nL = len(st.points), len(st.lines)

    def take(slot, sols):
        for l in sols:
            if l is None:
                continue
            k = lkey(l)
            if k in have or st.chord(l) <= 1e-6:
                continue
            per[slot][k] = 1
            uni[k] = 1

    for i, j in combinations(range(nP), 2):
        take(0, axiom1(st, i, j))
        take(1, axiom2(st, i, j))
    for i, j in combinations(range(nL), 2):
        take(2, axiom3(st, i, j))
    for p in range(nP):
        for l in range(nL):
            take(3, axiom4(st, p, l))
    for p in range(nP):
        for x in range(nL):
            for y in range(nL):
                take(4, axiom7(st, p, x, y))

    print(f"depth {depth} P {nP} L {nL} | "
          f"O1 {len(per[0])} O2 {len(per[1])} O3 {len(per[2])} "
          f"O4 {len(per[3])} O7 {len(per[4])} | union {len(uni)}")


def t4():
    print("### T4 closed-form candidate counts")
    st = State(1.0, SQRT2)
    t4_one(st, 0)
    for k in range(1, 6):
        _, sols = step(st, k)
        if not sols or st.fold(sols[0]) < 0:
            print(f"depth {k}: could not build")
            break
        t4_one(st, k)
    print()


if __name__ == "__main__":
    only = int(sys.argv[1]) if len(sys.argv) > 1 else -1
    for i, f in enumerate([t0, t1, t2, t3, t4]):
        if only < 0 or only == i:
            f()
