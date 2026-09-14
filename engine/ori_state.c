/*
 * ori_state.c - bookkeeping. No geometry decisions are taken here.
 *
 * Every function in this file is finished. If a test fails, the bug is
 * in ori_geom.c, not here - with one exception: rebuild() decides WHICH
 * duplicate point survives, and that choice has to match origami.py.
 * It keeps the FIRST one found, scanning line pairs as (i, j) with j > i,
 * which is exactly what itertools.combinations does. Do not "optimise"
 * that loop order.
 */

#include "ori_internal.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* identity keys                                                      */
/* ------------------------------------------------------------------ */

static long long key1(double v)
{
    /* llrint honours the current rounding mode, which defaults to
     * round-half-to-even - the same tie rule Python's round() uses. */
    return llrint(v * 1e6);
}

ori_lkey ori__lkey(ori_line l)
{
    ori_lkey k = { key1(l.a), key1(l.b), key1(l.c) };
    return k;
}

ori_pkey ori__pkey(ori_pt p)
{
    ori_pkey k = { key1(p.x), key1(p.y) };
    return k;
}

bool ori__lkey_eq(ori_lkey u, ori_lkey v)
{
    return u.a == v.a && u.b == v.b && u.c == v.c;
}

bool ori__pkey_eq(ori_pkey u, ori_pkey v)
{
    return u.x == v.x && u.y == v.y;
}

/* ------------------------------------------------------------------ */
/* construction                                                       */
/* ------------------------------------------------------------------ */

static void rebuild(ori_state *s)
{
    s->npoint = 0;
    for (int i = 0; i < s->nline; i++) {
        for (int j = i + 1; j < s->nline; j++) {
            ori_pt q;
            if (!ori__intersect(s->lines[i], s->lines[j], &q))
                continue;
            if (!ori__inside(s, q))
                continue;

            ori_pkey k = ori__pkey(q);
            bool seen = false;
            for (int m = 0; m < s->npoint; m++) {
                if (ori__pkey_eq(ori__pkey(s->points[m]), k)) {
                    seen = true;
                    break;
                }
            }
            if (!seen && s->npoint < ORI_MAX_POINTS)
                s->points[s->npoint++] = q;
        }
    }
}

ori_state *ori_new(double w, double h)
{
    if (!(w > 0.0) || !(h > 0.0))
        return NULL;

    ori_state *s = calloc(1, sizeof *s);
    if (!s)
        return NULL;

    s->w = w;
    s->h = h;

    /* Same order as origami.State.__init__: y=0, y=h, x=0, x=w.
     * The order fixes the line indices the tests refer to. */
    bool ok = true;
    ok &= ori__canon(0.0, 1.0, 0.0, &s->lines[0]);
    ok &= ori__canon(0.0, 1.0, h,   &s->lines[1]);
    ok &= ori__canon(1.0, 0.0, 0.0, &s->lines[2]);
    ok &= ori__canon(1.0, 0.0, w,   &s->lines[3]);
    if (!ok) {                       /* ori__canon not written yet */
        free(s);
        return NULL;
    }
    s->nline = ORI_EDGE_COUNT;
    rebuild(s);
    return s;
}

void ori_free(ori_state *s)
{
    free(s);
}

/* ------------------------------------------------------------------ */
/* inspection                                                         */
/* ------------------------------------------------------------------ */

int ori_point_count(const ori_state *s)
{
    return s ? s->npoint : 0;
}

ori_pt ori_point(const ori_state *s, int i)
{
    ori_pt zero = { 0.0, 0.0 };
    if (!s || i < 0 || i >= s->npoint)
        return zero;
    return s->points[i];
}

int ori_line_count(const ori_state *s)
{
    return s ? s->nline : 0;
}

ori_line ori_line_at(const ori_state *s, int i)
{
    ori_line zero = { 0.0, 0.0, 0.0 };
    if (!s || i < 0 || i >= s->nline)
        return zero;
    return s->lines[i];
}

int ori_nearest_point(const ori_state *s, double x, double y, double *out_dist)
{
    if (!s || s->npoint == 0) {
        if (out_dist)
            *out_dist = INFINITY;
        return -1;
    }
    int    best = 0;
    double bd   = INFINITY;
    for (int i = 0; i < s->npoint; i++) {
        double d = hypot(s->points[i].x - x, s->points[i].y - y);
        if (d < bd) {            /* strict: first minimum wins, as in Python */
            bd   = d;
            best = i;
        }
    }
    if (out_dist)
        *out_dist = bd;
    return best;
}

/* ------------------------------------------------------------------ */
/* mutation                                                           */
/* ------------------------------------------------------------------ */

int ori_fold(ori_state *s, ori_line crease)
{
    if (!s)
        return -1;
    if (s->nline >= ORI_MAX_LINES)
        return -1;
    if (ori_chord(s, crease) <= 1e-9)
        return -1;

    ori_lkey k = ori__lkey(crease);
    for (int i = 0; i < s->nline; i++)
        if (ori__lkey_eq(ori__lkey(s->lines[i]), k))
            return -1;

    s->lines[s->nline++] = crease;
    rebuild(s);
    return s->nline - 1;
}

bool ori_undo(ori_state *s)
{
    if (!s || s->nline <= ORI_EDGE_COUNT)
        return false;
    s->nline--;
    rebuild(s);
    return true;
}

int ori_fold_count(const ori_state *s)
{
    return s ? s->nline - ORI_EDGE_COUNT : 0;
}

/* ------------------------------------------------------------------ */
/* testing hooks                                                      */
/* ------------------------------------------------------------------ */

bool ori_check(const ori_state *s)
{
    if (!s)
        return false;

    for (int i = 0; i < s->nline; i++) {
        ori_line l = s->lines[i];
        if (fabs(hypot(l.a, l.b) - 1.0) > 1e-9)
            return false;
        if (l.a < -1e-9 || (fabs(l.a) <= 1e-9 && l.b < 0.0))
            return false;
    }
    for (int i = 0; i < s->nline; i++)
        for (int j = i + 1; j < s->nline; j++)
            if (ori__lkey_eq(ori__lkey(s->lines[i]), ori__lkey(s->lines[j])))
                return false;
    for (int i = 0; i < s->npoint; i++)
        for (int j = i + 1; j < s->npoint; j++)
            if (ori__pkey_eq(ori__pkey(s->points[i]), ori__pkey(s->points[j])))
                return false;
    for (int i = 0; i < s->npoint; i++)
        if (!ori__inside(s, s->points[i]))
            return false;
    return true;
}

/* --- dump ---------------------------------------------------------- */

/* Anything that would print as -0.000000000 prints as +0.000000000.
 * Without this the very first diff against origami.py is full of sign
 * noise: intersect() produces -0.0 for the corner at the origin.
 * Apply the same rule to State.dump() in origami.py. */
double ori__dump_fix(double v)
{
    return (fabs(v) < 5e-10) ? 0.0 : v;
}

static int cmp_pt(const void *pa, const void *pb)
{
    const ori_pt *a = pa, *b = pb;
    if (a->x < b->x) return -1;
    if (a->x > b->x) return  1;
    if (a->y < b->y) return -1;
    if (a->y > b->y) return  1;
    return 0;
}

/* append to buf, tracking how many bytes the full dump would need */
static void emit(char *buf, int cap, int *need, const char *fmt, ...)
{
    va_list ap;
    char    tmp[128];

    va_start(ap, fmt);
    int n = vsnprintf(tmp, sizeof tmp, fmt, ap);
    va_end(ap);
    if (n < 0)
        return;

    if (buf && cap > 0 && *need < cap - 1) {
        int room = cap - 1 - *need;
        int copy = (n < room) ? n : room;
        memcpy(buf + *need, tmp, (size_t)copy);
        buf[*need + copy] = '\0';
    }
    *need += n;
}

int ori_dump(const ori_state *s, char *buf, int cap)
{
    if (buf && cap > 0)
        buf[0] = '\0';
    if (!s)
        return 0;

    int need = 0;
    emit(buf, cap, &need, "W %.9f H %.9f\n", s->w, s->h);
    for (int i = 0; i < s->nline; i++)
        emit(buf, cap, &need, "L %d %+.9f %+.9f %+.9f\n", i,
             ori__dump_fix(s->lines[i].a),
             ori__dump_fix(s->lines[i].b),
             ori__dump_fix(s->lines[i].c));

    ori_pt sorted[ORI_MAX_POINTS];
    memcpy(sorted, s->points, (size_t)s->npoint * sizeof(ori_pt));
    qsort(sorted, (size_t)s->npoint, sizeof(ori_pt), cmp_pt);
    for (int i = 0; i < s->npoint; i++)
        emit(buf, cap, &need, "P %d %+.9f %+.9f\n", i,
             ori__dump_fix(sorted[i].x), ori__dump_fix(sorted[i].y));

    return need + 1;      /* including the NUL */
}
