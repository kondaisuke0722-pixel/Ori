"""
verify.py - validations and measurements for origami.py

Run this after any change to the engine (and after each step of the C port,
diffing State.dump() against ori_dump()).

    python3 verify.py            # validations + fold-6 counts  (~1 min)
    python3 verify.py spacing    # point-spacing table only     (fast)
"""
import math
import sys
from itertools import combinations

from origami import (State, canon, perp_bisector, line_through, lkey,
                     axiom6, candidates, MAX_SOLUTIONS)

H = math.sqrt(2)                     # 1:sqrt(2) sheet, nose at the top


# --------------------------------------------------------------------------
def beloch():
    """
    Beloch's fold for x^3 = 2.  Folding (-1,0) onto x=1 while folding
    (0,-2) onto y=2 gives a crease whose slope is a cube root of 2.
    x^3 - 2 has exactly ONE real root, so exactly one crease must appear.
    """
    st = State(10.0, 10.0)
    st.points = [(-1.0, 0.0), (0.0, -2.0)]
    st.lines = [canon(1, 0, 1.0), canon(0, 1, 2.0)]
    sols = axiom6(st, 0, 0, 1, 1)
    print("[1] Beloch fold, x^3 = 2")
    ok = False
    for s in sols:
        slope = -s[0] / s[1] if abs(s[1]) > 1e-12 else float("inf")
        print(f"      crease slope = {slope:+.9f}")
        if abs(abs(slope) - 2 ** (-1 / 3)) < 1e-7:
            ok = True
    print(f"      expected |slope| = {2 ** (-1/3):.9f}   found {len(sols)} real solution(s)")
    print(f"      -> {'PASS' if ok and len(sols) == 1 else 'FAIL'}\n")


def invariants():
    """cheap structural invariants that catch the bugs that fail silently"""
    st = build(5)
    print("[2] invariants")
    print(f"      state self-check            : {'PASS' if st.check() else 'FAIL'}")

    worst = 0
    nP, nL = len(st.points), len(st.lines)
    for a in range(nP):
        for la in range(nL):
            for b in range(nP):
                for lb in range(nL):
                    worst = max(worst, len(axiom6(st, a, la, b, lb)))
    print(f"      max O6 solutions (must be <= {MAX_SOLUTIONS}) : {worst}"
          f"   -> {'PASS' if worst <= MAX_SOLUTIONS else 'FAIL'}")

    # double reflection is the identity
    from origami import reflect_pt
    bad = max(math.hypot(*(x - y for x, y in
                           zip(reflect_pt(reflect_pt(p, l), l), p)))
              for p in st.points for l in st.lines)
    print(f"      max double-reflection drift : {bad:.2e}"
          f"   -> {'PASS' if bad < 1e-9 else 'FAIL'}\n")


# --------------------------------------------------------------------------
def build(nfolds):
    """the generic (asymmetric) 5-fold sequence used for all measurements"""
    st = State(1.0, H)
    seq = [perp_bisector((0, 0), (0, H)),
           line_through((0, 0), (1, H)),
           perp_bisector((0, H), (1, 0)),
           canon(0, 1, H * 0.25),
           perp_bisector((0, 0), (1, H / 2))]
    for s in seq[:nfolds]:
        st.fold(s)
    return st


def counts():
    st = build(5)
    print(f"[3] candidate folds at fold 6   (P={len(st.points)}, L={len(st.lines)})")
    d = candidates(st)
    for k in [f"O{i}" for i in range(1, 8)]:
        print(f"      {k}: {len(d[k]):>6}")
    allk = set().union(*d.values())
    no6 = set().union(*[v.keys() for k, v in d.items() if k != "O6"])
    no56 = set().union(*[v.keys() for k, v in d.items() if k not in ("O5", "O6")])
    print(f"      total distinct : {len(allk):>6}   (expected ~7470)")
    print(f"      without O6     : {len(no6):>6}   (expected ~1673)")
    print(f"      without O5,O6  : {len(no56):>6}   (expected ~264)")
    print(f"      O6 share       : {len(d['O6']) / len(allk) * 100:>5.1f}%\n")


def spacing(paper_pt=350.0):
    """
    How close do points get, in screen points, with the sheet drawn
    paper_pt wide?  44pt is Apple's minimum touch target.
    """
    print(f"[4] point spacing (sheet drawn {paper_pt:.0f}pt wide)")
    import random
    random.seed(7)
    st = build(5)
    for step in range(5, 15):
        ds = sorted(math.hypot(a[0] - b[0], a[1] - b[1])
                    for a, b in combinations(st.points, 2))
        f = lambda v: v * paper_pt
        print(f"      fold {step:>2} | P={len(st.points):>3} | "
              f"min {f(ds[0]):6.1f}pt | 5%ile {f(ds[len(ds)//20]):6.1f}pt | "
              f"<44pt {sum(1 for v in ds if f(v) < 44):>4}/{len(ds)}")
        d = candidates(st, min_chord=0.15, skip=("O5", "O6"))
        pool = [v for sub in d.values() for v in sub.values()]
        if not pool:
            break
        st.fold(random.choice(pool))
    print()


# --------------------------------------------------------------------------
if __name__ == "__main__":
    if "spacing" in sys.argv:
        spacing()
    else:
        beloch()
        invariants()
        counts()
        spacing()
