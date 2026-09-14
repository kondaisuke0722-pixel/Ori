"""
origami.py - reference implementation of the Huzita-Hatori axiom engine.

Purpose: this is the ORACLE for the C port. It is deliberately written to
mirror the structure of ori.h, so that C output can be diffed against it.

Model ("crease construction", the formalism the 7 axioms are defined in):
  - the sheet is a W x H rectangle with a corner at the origin
  - state = a list of LINES (4 edges + creases) and the POINTS that are
    their pairwise intersections inside the sheet
  - a fold adds one line, then new intersection points appear

KNOWN LIMITATION (do not paper over this in the C port):
  Identity of lines and points is decided by rounding to ROUND decimals.
  That is a hack, not a solution. Measured point spacing collapses to about
  3e-4 of the sheet width by fold 13, which is the same order as the rounding
  tolerance. This engine is only trustworthy to roughly 6-8 folds.
"""
import math
from itertools import combinations

# import numpy as np

ROUND = 6                    # decimals used for identity of lines/points
MAX_SOLUTIONS = 3            # O6 solves a cubic


# --------------------------------------------------------------------------
# geometry.  line = (a, b, c) with a*x + b*y = c, a^2 + b^2 = 1, canonical sign
# --------------------------------------------------------------------------

def canon(a, b, c):
    n = math.hypot(a, b)
    if n < 1e-12:
        return None
    a, b, c = a / n, b / n, c / n
    if a < -1e-9 or (abs(a) <= 1e-9 and b < 0):
        a, b, c = -a, -b, -c
    return (a, b, c)


def lkey(l):
    return (round(l[0], ROUND), round(l[1], ROUND), round(l[2], ROUND))


def pkey(p):
    return (round(p[0], ROUND), round(p[1], ROUND))


def line_through(p1, p2):
    dx, dy = p2[0] - p1[0], p2[1] - p1[1]
    if math.hypot(dx, dy) < 1e-12:
        return None
    return canon(-dy, dx, -dy * p1[0] + dx * p1[1])


def perp_bisector(p1, p2):
    dx, dy = p2[0] - p1[0], p2[1] - p1[1]
    if math.hypot(dx, dy) < 1e-12:
        return None
    return canon(dx, dy, dx * (p1[0] + p2[0]) / 2 + dy * (p1[1] + p2[1]) / 2)


def reflect_pt(p, l):
    a, b, c = l
    d = a * p[0] + b * p[1] - c
    return (p[0] - 2 * d * a, p[1] - 2 * d * b)


def signed_dist(p, l):
    return l[0] * p[0] + l[1] * p[1] - l[2]


def intersect(l1, l2):
    det = l1[0] * l2[1] - l2[0] * l1[1]
    if abs(det) < 1e-10:
        return None
    return ((l1[2] * l2[1] - l2[2] * l1[1]) / det,
            (l1[0] * l2[2] - l2[0] * l1[2]) / det)


# --------------------------------------------------------------------------
# state
# --------------------------------------------------------------------------

class State:
    def __init__(self, w=1.0, h=1.0):
        self.w, self.h = w, h
        self.lines = [canon(0, 1, 0), canon(0, 1, h),
                      canon(1, 0, 0), canon(1, 0, w)]
        self._rebuild()

    # -- inspection ------------------------------------------------------
    def inside(self, p, tol=1e-7):
        return -tol <= p[0] <= self.w + tol and -tol <= p[1] <= self.h + tol

    def chord(self, l):
        """length of the part of l inside the sheet; 0 if it misses"""
        edges = [canon(0, 1, 0), canon(0, 1, self.h),
                 canon(1, 0, 0), canon(1, 0, self.w)]
        pts = [q for e in edges
               if (q := intersect(l, e)) is not None and self.inside(q)]
        if len(pts) < 2:
            return 0.0
        return max(math.hypot(a[0] - b[0], a[1] - b[1])
                   for a, b in combinations(pts, 2))

    def nearest_point(self, x, y):
        if not self.points:
            return -1, float('inf')
        ds = [math.hypot(p[0] - x, p[1] - y) for p in self.points]
        i = min(range(len(ds)), key=ds.__getitem__)
        return i, ds[i]

    # -- mutation --------------------------------------------------------
    def _rebuild(self):
        pts = {}
        for l1, l2 in combinations(self.lines, 2):
            q = intersect(l1, l2)
            if q is not None and self.inside(q):
                pts.setdefault(pkey(q), q)
        self.points = list(pts.values())

    def fold(self, crease):
        """commit a crease; returns its line index, or -1 if rejected"""
        if crease is None or self.chord(crease) <= 1e-9:
            return -1
        if lkey(crease) in {lkey(l) for l in self.lines}:
            return -1
        self.lines.append(crease)
        self._rebuild()
        return len(self.lines) - 1

    # -- testing hook, mirrors ori_dump ----------------------------------
    def dump(self):
        ls = "\n".join(f"L {i} {l[0]:+.9f} {l[1]:+.9f} {l[2]:+.9f}"
                       for i, l in enumerate(self.lines))
        ps = "\n".join(f"P {i} {p[0]:+.9f} {p[1]:+.9f}"
                       for i, p in enumerate(sorted(self.points)))
        return f"W {self.w:.9f} H {self.h:.9f}\n{ls}\n{ps}\n"

    def check(self):
        for l in self.lines:
            if abs(math.hypot(l[0], l[1]) - 1.0) > 1e-9:
                return False
            if l[0] < -1e-9 or (abs(l[0]) <= 1e-9 and l[1] < 0):
                return False
        if len({lkey(l) for l in self.lines}) != len(self.lines):
            return False
        if len({pkey(p) for p in self.points}) != len(self.points):
            return False
        return all(self.inside(p) for p in self.points)


# --------------------------------------------------------------------------
# the seven axioms.  each returns a list of creases (possibly empty).
# arguments are INDICES into st.points / st.lines, as in ori.h
# --------------------------------------------------------------------------

def axiom1(st, p1, p2):
    l = line_through(st.points[p1], st.points[p2])
    return [l] if l else []


def axiom2(st, p1, p2):
    l = perp_bisector(st.points[p1], st.points[p2])
    return [l] if l else []


def axiom3(st, l1, l2):
    a1, b1, c1 = st.lines[l1]
    a2, b2, c2 = st.lines[l2]
    out = []
    for s in (+1, -1):
        l = canon(a1 + s * a2, b1 + s * b2, c1 + s * c2)
        if l and lkey(l) not in {lkey(x) for x in out}:
            out.append(l)
    return out


def axiom4(st, p, l):
    px, py = st.points[p]
    a, b, _ = st.lines[l]
    r = canon(-b, a, -b * px + a * py)
    return [r] if r else []


def axiom5(st, p1, l1, p2):
    P1, P2 = st.points[p1], st.points[p2]
    a, b, c = st.lines[l1]
    r = math.hypot(P1[0] - P2[0], P1[1] - P2[1])
    if r < 1e-9:
        return []
    d = a * P2[0] + b * P2[1] - c
    if abs(d) > r + 1e-12:
        return []
    foot = (P2[0] - d * a, P2[1] - d * b)
    t = math.sqrt(max(0.0, r * r - d * d))
    out, seen = [], set()
    for s in (+1, -1):
        img = (foot[0] - s * t * b, foot[1] + s * t * a)
        cr = perp_bisector(P1, img)
        if cr and lkey(cr) not in seen:
            seen.add(lkey(cr))
            out.append(cr)
    return out


def axiom6(st, p1, l1, p2, l2, samples=3000):
    """
    Place p1 onto l1 AND p2 onto l2.  Up to 3 creases (a cubic).

    Method: parametrise the image of p1 along l1 by arclength t, measured
    FROM THE FOOT OF THE PERPENDICULAR FROM p1 -- not from the origin, which
    was the bug in the first version.  The crease is then the perpendicular
    bisector of p1 and its image, and we root-find on the residual
    "how far is the image of p2 from l2".

    Bound on t: the crease must cross the sheet, so dist(p1, crease) <= diag,
    hence |p1' - p1| <= 2*diag.  This makes the search range finite.
    """
    P1, P2 = st.points[p1], st.points[p2]
    L1, L2 = st.lines[l1], st.lines[l2]

    # degenerate: the two constraints are the same one -> infinitely many
    if lkey(L1) == lkey(L2) and pkey(P1) == pkey(P2):
        return []

    a, b, c = L1
    d = a * P1[0] + b * P1[1] - c
    foot = (P1[0] - d * a, P1[1] - d * b)
    dx, dy = -b, a
    lim = 2 * math.hypot(st.w, st.h) + 1e-6
    if abs(d) > lim:
        return []
    span = math.sqrt(max(0.0, lim * lim - d * d))
    if span < 1e-9:
        return []

    t = np.linspace(-span, span, samples)
    ix, iy = foot[0] + t * dx, foot[1] + t * dy
    ux, uy = ix - P1[0], iy - P1[1]
    nrm = np.hypot(ux, uy)
    ok = nrm > 1e-7
    safe = np.where(ok, nrm, 1.0)
    nx, ny = ux / safe, uy / safe
    cc = nx * (P1[0] + ix) / 2 + ny * (P1[1] + iy) / 2
    dd = nx * P2[0] + ny * P2[1] - cc
    rx, ry = P2[0] - 2 * dd * nx, P2[1] - 2 * dd * ny
    res = np.where(ok, L2[0] * rx + L2[1] * ry - L2[2], np.nan)

    good = res[~np.isnan(res)]
    if good.size == 0 or np.max(np.abs(good)) < 1e-7:
        return []                      # residual identically zero -> degenerate

    def at(tv):
        q = (foot[0] + tv * dx, foot[1] + tv * dy)
        cr = perp_bisector(P1, q)
        if cr is None:
            return None, None
        return signed_dist(reflect_pt(P2, cr), L2), cr

    out, seen = [], set()
    for i in range(samples - 1):
        v0, v1 = res[i], res[i + 1]
        if np.isnan(v0) or np.isnan(v1) or v0 * v1 >= 0:
            continue
        lo, hi, flo = t[i], t[i + 1], v0
        for _ in range(80):                       # bisection
            mid = (lo + hi) / 2
            fm, _ = at(mid)
            if fm is None:
                break
            if flo * fm <= 0:
                hi = mid
            else:
                lo, flo = mid, fm
        fv, cr = at((lo + hi) / 2)
        if cr and fv is not None and abs(fv) < 1e-7 and lkey(cr) not in seen:
            seen.add(lkey(cr))
            out.append(cr)
    return out[:MAX_SOLUTIONS]


def axiom7(st, p1, l1, l2):
    P1 = st.points[p1]
    a1, b1, c1 = st.lines[l1]
    a2, b2, _ = st.lines[l2]
    n = canon(-b2, a2, 0)
    if n is None:
        return []
    na, nb, _ = n
    denom = -2 * (a1 * na + b1 * nb)
    if abs(denom) < 1e-10:
        return []
    k = (c1 - a1 * P1[0] - b1 * P1[1]) / denom
    r = canon(na, nb, na * P1[0] + nb * P1[1] - k)
    return [r] if r else []


# --------------------------------------------------------------------------
# candidate enumeration (for counting only - the UI never calls this)
# --------------------------------------------------------------------------

def candidates(st, min_chord=1e-6, skip=()):
    """dict: axiom name -> {line key: line}, excluding lines already present"""
    out = {f"O{i}": {} for i in range(1, 8)}
    have = {lkey(l) for l in st.lines}
    nP, nL = len(st.points), len(st.lines)

    def add(name, ls):
        for l in ls:
            if l is None:
                continue
            k = lkey(l)
            if k in have or st.chord(l) <= min_chord:
                continue
            out[name][k] = l

    if "O1" not in skip:
        for i, j in combinations(range(nP), 2):
            add("O1", axiom1(st, i, j))
    if "O2" not in skip:
        for i, j in combinations(range(nP), 2):
            add("O2", axiom2(st, i, j))
    if "O3" not in skip:
        for i, j in combinations(range(nL), 2):
            add("O3", axiom3(st, i, j))
    if "O4" not in skip:
        for p in range(nP):
            for l in range(nL):
                add("O4", axiom4(st, p, l))
    if "O5" not in skip:
        for a in range(nP):
            for l in range(nL):
                for b in range(nP):
                    add("O5", axiom5(st, a, l, b))
    if "O6" not in skip:
        pl = [(p, l) for p in range(nP) for l in range(nL)]
        for (a, la), (b, lb) in combinations(pl, 2):
            add("O6", axiom6(st, a, la, b, lb))
    if "O7" not in skip:
        for p in range(nP):
            for x in range(nL):
                for y in range(nL):
                    add("O7", axiom7(st, p, x, y))
    return out
