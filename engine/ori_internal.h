/*
 * ori_internal.h - not part of the public boundary.
 *
 * Everything here is invisible to Swift. ori.h stays as it is.
 */

#ifndef ORI_INTERNAL_H
# define ORI_INTERNAL_H

# include "ori.h"

/* ------------------------------------------------------------------ *
 * Capacities.
 *
 * Fixed arrays, not realloc. Rationale: the engine is only trustworthy
 * to ~6-8 folds (requirements 6.3), so an unbounded sheet is a bound we
 * do not actually need, and a growable array is a class of bug we do not
 * need either. L lines produce at most L*(L-1)/2 points.
 *
 * ori_fold returns -1 when the line array is full - the same failure
 * value as "already exists" and "misses the sheet".  This is a DECISION
 * that is not yet in the requirements document.
 * ------------------------------------------------------------------ */
# define ORI_MAX_LINES 64
# define ORI_MAX_POINTS (ORI_MAX_LINES * (ORI_MAX_LINES - 1) / 2)

# define ORI_EDGE_COUNT 4 /* lines 0..3 are the sheet edges */

/* ------------------------------------------------------------------ *
 * Identity keys.
 *
 * origami.py decides identity by rounding to 6 decimals. We mirror that
 * with an integer key: llrint(v * 1e6). Integer comparison is exact, and
 * llrint maps -0.0 to 0, which kills the signed-zero trap for free.
 *
 * Caveat, recorded so it is not discovered later: v*1e6 is computed in
 * binary, so a value sitting within ~1 ulp of a bucket boundary could
 * land in a different bucket than Python's round(). Measured minimum
 * point spacing at fold 5 is 7.2e-2, five orders of magnitude from any
 * boundary, so this cannot bite inside the supported depth.
 * ------------------------------------------------------------------ */
typedef struct
{
	long long a, b, c;
}				ori_lkey;
typedef struct
{
	long long x, y;
}				ori_pkey;

ori_lkey		ori__lkey(ori_line l);
ori_pkey		ori__pkey(ori_pt p);
bool			ori__lkey_eq(ori_lkey u, ori_lkey v);
double			ori__dump_fix(double v);
bool			ori__pkey_eq(ori_pkey u, ori_pkey v);

/* ------------------------------------------------------------------ *
 * State
 * ------------------------------------------------------------------ */
struct			ori_state
{
	double w, h;
	ori_line	lines[ORI_MAX_LINES];
	int			nline;
	ori_pt		points[ORI_MAX_POINTS];
	int			npoint;
};

/* ------------------------------------------------------------------ *
 * Geometry primitives - implemented in ori_geom.c
 *
 * Python returns None for a degenerate result. We return false and leave
 * *out untouched. Callers must check the return value.
 * ------------------------------------------------------------------ */

/* Normalise a*x + b*y = c to a*a + b*b == 1 with the canonical sign
 * (a > 0, or |a| <= 1e-9 and b >= 0). False if (a,b) is degenerate,
 * i.e. hypot(a,b) < 1e-12.  Mirrors origami.canon. */
bool			ori__canon(double a, double b, double c, ori_line *out);

/* Intersection of two lines. False if they are parallel, i.e. the
 * determinant a1*b2 - a2*b1 has magnitude < 1e-10.  Mirrors
 * origami.intersect. */
bool			ori__intersect(ori_line l1, ori_line l2, ori_pt *out);

/* Line through p1 and p2. False if they coincide (< 1e-12 apart). */
bool			ori__line_through(ori_pt p1, ori_pt p2, ori_line *out);

/* Perpendicular bisector of p1 and p2. False if they coincide. */
bool			ori__perp_bisector(ori_pt p1, ori_pt p2, ori_line *out);

/* Is p inside the sheet, with a 1e-7 slack on every side? */
bool			ori__inside(const ori_state *s, ori_pt p);

#endif /* ORI_INTERNAL_H */
