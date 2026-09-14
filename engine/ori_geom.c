/*
 * ori_geom.c - THIS IS THE FILE YOU WRITE.
 *
 * Everything else compiles and is finished. Fill in the bodies below in
 * the order they appear; each stage of ./test_ori turns green as you go:
 *
 *     T0  needs  ori__canon, ori__intersect, ori__inside
 *     T1  needs  the above + ori__line_through, ori__perp_bisector, ori_chord
 *     T2  needs  the above + ori_axiom1..4  (and ori_axiom7 if you do it)
 *     T3  needs  everything in T2
 *     T4  needs  everything in T2
 *
 *     make check      builds, runs, and diffs against origami.py
 *     make check S=0  just stage T0
 *
 * The reference for every function is origami.py. Two things there do NOT
 * carry over and you have to add them yourself:
 *
 *   - Python raises IndexError on a bad index and silently wraps negative
 *     ones. Requirements 5.1 says an out-of-range index returns 0 and does
 *     not crash. Every axiom needs its own bounds check. The diff against
 *     Python cannot catch a missing one; T2 tests it explicitly.
 *
 *   - Python returns None; here you return false and leave *out alone.
 */

#include "ori_internal.h"

#include <math.h>

/* ================================================================== */
/* primitives                                                         */
/* ================================================================== */

/* origami.canon
 *
 *   n = hypot(a, b);  if (n < 1e-12) return false;
 *   divide a, b, c by n
 *   if (a < -1e-9 || (fabs(a) <= 1e-9 && b < 0))  negate all three
 *
 * Watch the second condition: it is NOT "a == 0 && b < 0". A line whose
 * normal is a hair off vertical must still be flipped, or two spellings
 * of the same line survive side by side.
 */
bool ori__canon(double a, double b, double c, ori_line *out)
{
   double n = hypot(a, b);      /* 矢印の長さを測る */
    if (n < 1e-12) return false; /* 長さがなければ直線ではない */

    out->a = a / n;              /* 3つとも、その長さで割る */
    out->b = b / n;              /* → 矢印の長さが 1 になる */
    out->c = c / n;              /* → 直線は変わらない */
    if (out->a < -1e-9 || (fabs(out->a) <= 1e-9 && out->b < 0))
    {
        out->a = -out->a;
        out->b = -out->b;
        out->c = -out->c;
    }
    return true;
}



/* origami.intersect
 *
 *   det = l1.a*l2.b - l2.a*l1.b;   if (fabs(det) < 1e-10) return false;
 *   x = (l1.c*l2.b - l2.c*l1.b) / det
 *   y = (l1.a*l2.c - l2.a*l1.c) / det
 *
 * Keep the expressions in exactly this shape. Reordering the subtractions
 * changes the last bits, and -0.0 appears or disappears depending on the
 * sign of det.
 */
bool ori__intersect(ori_line l1, ori_line l2, ori_pt *out)
{
    double det = l1.a * l2.b - l1.b* l2.a;
    if (fabs(det) < 1e-10)
        return false;
    out->x = (l2.b * l1.c - l1.b * l2.c) / det;
    out->y = (l1.a * l2.c - l2.a * l1.c) / det;
    return true;
}



/* origami.line_through - the line through p1 and p2.
 * Direction d = p2 - p1; the normal is (-d.y, d.x). Feed it to ori__canon.
 * False if |d| < 1e-12. */
bool ori__line_through(ori_pt p1, ori_pt p2, ori_line *out)
{
    double dx = p2.x - p1.x;
    double dy = p2.y - p1.y;
    if (hypot(dx,dy) < 1e-12)
        return false;
    return ori__canon(-dy, dx, -dy * p1.x + dx + p1.y);
}


// def line_through(p1, p2):
//     dx, dy = p2[0] - p1[0], p2[1] - p1[1]
//     if math.hypot(dx, dy) < 1e-12:
//         return None
//     return canon(-dy, dx, -dy * p1[0] + dx * p1[1])
/* origami.perp_bisector - normal is d = p2 - p1, and the line passes
 * through the midpoint. False if |d| < 1e-12. */
bool ori__perp_bisector(ori_pt p1, ori_pt p2, ori_line *out)
{
    (void)p1; (void)p2; (void)out;
    return false;   /* TODO */
}

/* origami.State.inside, tol = 1e-7 on every side. */
bool ori__inside(const ori_state *s, ori_pt p)
{
    const double tol = 1e-7;
    return -tol <= p.x && p.x <= s->w + tol && -tol <= p.y && p.y <= s->h + tol;
}






/* ori_chord - length of the part of `l` inside the sheet, 0 if it misses.
 *
 * origami.State.chord intersects l with each of the four edges, keeps the
 * hits that are inside(), and returns the largest distance between any two
 * of them - 0 if fewer than two survive.
 *
 * Taking the MAX over all pairs, not just the first two, is deliberate: a
 * line through a corner hits three or four edges, and two of those hits
 * coincide. Do not shortcut it.
 */
double ori_chord(const ori_state *s, ori_line l)
{
    (void)s; (void)l;
    return 0.0;     /* TODO */
}

/* ================================================================== */
/* axioms                                                             */
/*                                                                    */
/* Each writes solutions to out[] and returns how many it wrote.       */
/* Return 0 for a bad index. Never write past the documented size.     */
/* ================================================================== */

/* O1: the line through p1 and p2.   out[1] */
int ori_axiom1(const ori_state *s, int p1, int p2, ori_line *out)
{
    (void)s; (void)p1; (void)p2; (void)out;
    return 0;       /* TODO */
}

/* O2: place p1 onto p2 - the perpendicular bisector.   out[1] */
int ori_axiom2(const ori_state *s, int p1, int p2, ori_line *out)
{
    (void)s; (void)p1; (void)p2; (void)out;
    return 0;       /* TODO */
}

/* O3: place l1 onto l2 - the two angle bisectors.   out[2]
 *
 *   for sign in (+1, -1):
 *       canon(a1 + sign*a2, b1 + sign*b2, c1 + sign*c2)
 *
 * Both normals are already unit vectors, which is why plain addition
 * gives the bisector. When the lines are parallel one of the two is
 * degenerate and ori__canon rejects it, leaving a single solution.
 *
 * Keep origami.py's duplicate guard (skip a solution whose ori__lkey
 * matches one already written). It never fires for non-parallel lines -
 * the two bisectors are perpendicular to each other - but it is cheap.
 *
 * Note l1 == l2 returns 1 solution which is that same line. That is what
 * the reference does. ori_fold will reject it; see the grey-out rule.
 */
int ori_axiom3(const ori_state *s, int l1, int l2, ori_line *out)
{
    (void)s; (void)l1; (void)l2; (void)out;
    return 0;       /* TODO */
}

/* O4: through p, perpendicular to l.   out[1]
 * l has normal (a, b); the perpendicular line has normal (-b, a) and
 * passes through p. */
int ori_axiom4(const ori_state *s, int p, int l, ori_line *out)
{
    (void)s; (void)p; (void)l; (void)out;
    return 0;       /* TODO */
}



/* O7: place p1 onto l1, crease perpendicular to l2.   out[1]
 *
 * Closed form, no root finding - it belongs with O1..O4 even though the
 * requirements document lists it with O5/O6. Doing it now is what makes
 * the 264 acceptance figure reachable at this stage. See origami.axiom7.
 * Leave it as a stub if you would rather do it after T3; T4 prints the
 * O1-O4-only target (205) alongside.
 */
int ori_axiom7(const ori_state *s, int p1, int l1, int l2, ori_line *out)
{
    (void)s; (void)p1; (void)l1; (void)l2; (void)out;
    return 0;       /* TODO (optional this round) */
}

/* ================================================================== */
/* not this round - O5 and O6 (requirements 12.5)                      */
/* ================================================================== */

int ori_axiom5(const ori_state *s, int p1, int l1, int p2, ori_line *out)
{
    (void)s; (void)p1; (void)l1; (void)p2; (void)out;
    return 0;
}

int ori_axiom6(const ori_state *s, int p1, int l1, int p2, int l2,
               ori_line *out)
{
    (void)s; (void)p1; (void)l1; (void)p2; (void)l2; (void)out;
    return 0;
}
