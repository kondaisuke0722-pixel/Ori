/*
 * test_ori.c - prints a deterministic transcript. oracle.py prints the
 * same transcript from origami.py. `make check` diffs the two.
 *
 *     ./test_ori          all stages
 *     ./test_ori 2        stage T2 only
 *
 * Nothing here belongs in the shipped library. The candidate enumeration
 * in T4 mirrors origami.candidates(), which the UI never calls.
 */

#include "ori_internal.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SQRT2 1.4142135623730951

static double fx(double v) { return ori__dump_fix(v); }

static void pl(const char *tag, int n, const ori_line *out)
{
    printf("%s -> %d", tag, n);
    for (int i = 0; i < n; i++)
        printf(" | %+.9f %+.9f %+.9f", fx(out[i].a), fx(out[i].b), fx(out[i].c));
    printf("\n");
}

static void dump_state(const ori_state *s)
{
    int   need = ori_dump(s, NULL, 0);
    char *buf  = malloc((size_t)need);
    ori_dump(s, buf, need);
    fputs(buf, stdout);
    free(buf);
    printf("check %d\nfold_count %d\n", ori_check(s) ? 1 : 0, ori_fold_count(s));
}

/* ------------------------------------------------------------------ */
/* T0                                                                 */
/* ------------------------------------------------------------------ */
static void t0(void)
{
    printf("### T0 new(1, sqrt2)\n");
    ori_state *s = ori_new(1.0, SQRT2);
    if (!s) { printf("ori_new returned NULL\n"); return; }
    dump_state(s);
    printf("undo %d\n", ori_undo(s) ? 1 : 0);
    ori_free(s);
    printf("\n");
}

/* ------------------------------------------------------------------ */
/* T1                                                                 */
/* ------------------------------------------------------------------ */
static const double CANON[][3] = {
    { 0, 1, 0 }, { 0, -1, 0 }, { 1, 0, 0 }, { -1, 0, -1 },
    { 3, 4, 5 }, { -3, -4, -5 }, { 0, 0, 1 }, { 1e-13, 1e-13, 1 },
    { 1e-10, -1, 0 }, { -0.0, 1, 0 }, { 2, 0, 6 },
};
static const double PAIRS[][4] = {           /* p1.x p1.y p2.x p2.y */
    { 0, 0, 1, 0 }, { 1, 0, 0, 0 }, { 0, 0, 0, 1 }, { 0, 0, 1, 1 },
    { 0.25, 0.5, 0.25, 0.5 }, { 1, 2, 3, 4 },
};
static const double INSIDE[][2] = {
    { 0.5, 0.5 }, { -1e-8, 0.5 }, { -1e-6, 0.5 },
    { 1.0, SQRT2 }, { 1.5, 0.5 }, { 0.5, SQRT2 + 1e-8 },
};
static const double CHORD[][3] = {           /* raw a, b, c for ori__canon */
    { 0, 1, SQRT2 / 2 }, { 1, 0, 0.5 }, { -SQRT2, 1, 0 },
    { 0, 1, -1 }, { 1, 1, 0 },
};

static void t1(void)
{
    printf("### T1 primitives\n");

    for (size_t i = 0; i < sizeof CANON / sizeof CANON[0]; i++) {
        ori_line l;
        char tag[48];
        snprintf(tag, sizeof tag, "canon %02zu", i);
        bool ok = ori__canon(CANON[i][0], CANON[i][1], CANON[i][2], &l);
        pl(tag, ok ? 1 : 0, &l);
    }

    ori_state *s = ori_new(1.0, SQRT2);
    if (!s) { printf("ori_new returned NULL\n\n"); return; }

    for (int i = 0; i < ori_line_count(s); i++)
        for (int j = i + 1; j < ori_line_count(s); j++) {
            ori_pt q;
            bool ok = ori__intersect(ori_line_at(s, i), ori_line_at(s, j), &q);
            printf("intersect %d %d -> %d", i, j, ok ? 1 : 0);
            if (ok) printf(" | %+.9f %+.9f", fx(q.x), fx(q.y));
            printf("\n");
        }

    for (size_t i = 0; i < sizeof PAIRS / sizeof PAIRS[0]; i++) {
        ori_pt a = { PAIRS[i][0], PAIRS[i][1] };
        ori_pt b = { PAIRS[i][2], PAIRS[i][3] };
        ori_line l;
        char tag[48];
        snprintf(tag, sizeof tag, "through %02zu", i);
        pl(tag, ori__line_through(a, b, &l) ? 1 : 0, &l);
        snprintf(tag, sizeof tag, "bisector %02zu", i);
        pl(tag, ori__perp_bisector(a, b, &l) ? 1 : 0, &l);
    }

    for (size_t i = 0; i < sizeof INSIDE / sizeof INSIDE[0]; i++) {
        ori_pt p = { INSIDE[i][0], INSIDE[i][1] };
        printf("inside %02zu -> %d\n", i, ori__inside(s, p) ? 1 : 0);
    }

    for (size_t i = 0; i < sizeof CHORD / sizeof CHORD[0]; i++) {
        ori_line l;
        if (!ori__canon(CHORD[i][0], CHORD[i][1], CHORD[i][2], &l)) {
            printf("chord %02zu -> degenerate\n", i);
            continue;
        }
        printf("chord %02zu -> %+.9f\n", i, fx(ori_chord(s, l)));
    }

    /* query 1 is a deliberate exact tie between points 0 and 1 - the
     * two distances are bit-identical, so it only comes out as 0 if the
     * comparison is strict `<`. A `<=` there silently returns the LAST
     * minimum and origami.py returns the first. */
    static const double NEAR[][2] = { { 0.9, 0.1 }, { 0.5, 0.5 }, { 0.9, 1.2 } };
    for (size_t i = 0; i < sizeof NEAR / sizeof NEAR[0]; i++) {
        double d;
        int    k = ori_nearest_point(s, NEAR[i][0], NEAR[i][1], &d);
        printf("nearest %zu -> %d %+.9f\n", i, k, fx(d));
    }

    ori_free(s);
    printf("\n");
}

/* ------------------------------------------------------------------ */
/* T2                                                                 */
/* ------------------------------------------------------------------ */
static void t2(void)
{
    printf("### T2 axioms on the initial state\n");
    ori_state *s = ori_new(1.0, SQRT2);
    if (!s) { printf("ori_new returned NULL\n\n"); return; }

    int      nP = ori_point_count(s), nL = ori_line_count(s);
    ori_line out[ORI_MAX_SOLUTIONS];
    char tag[48];

    for (int i = 0; i < nP; i++)
        for (int j = i + 1; j < nP; j++) {
            snprintf(tag, sizeof tag, "O1 %d %d", i, j);
            pl(tag, ori_axiom1(s, i, j, out), out);
        }
    for (int i = 0; i < nP; i++)
        for (int j = i + 1; j < nP; j++) {
            snprintf(tag, sizeof tag, "O2 %d %d", i, j);
            pl(tag, ori_axiom2(s, i, j, out), out);
        }
    for (int i = 0; i < nL; i++)
        for (int j = i; j < nL; j++) {          /* includes l1 == l2 */
            snprintf(tag, sizeof tag, "O3 %d %d", i, j);
            pl(tag, ori_axiom3(s, i, j, out), out);
        }
    for (int p = 0; p < nP; p++)
        for (int l = 0; l < nL; l++) {
            snprintf(tag, sizeof tag, "O4 %d %d", p, l);
            pl(tag, ori_axiom4(s, p, l, out), out);
        }
    for (int p = 0; p < nP; p++)
        for (int x = 0; x < nL; x++)
            for (int y = 0; y < nL; y++) {
                snprintf(tag, sizeof tag, "O7 %d %d %d", p, x, y);
                pl(tag, ori_axiom7(s, p, x, y, out), out);
            }

    /* requirements 5.1: out of range returns 0, never crashes.
     * origami.py does NOT do this - Python wraps negatives silently -
     * so the diff cannot catch a missing check. These lines can. */
    printf("oor O1 -1 0 -> %d\n", ori_axiom1(s, -1, 0, out));
    printf("oor O1 0 99 -> %d\n", ori_axiom1(s, 0, 99, out));
    printf("oor O2 -1 -1 -> %d\n", ori_axiom2(s, -1, -1, out));
    printf("oor O3 0 -5 -> %d\n", ori_axiom3(s, 0, -5, out));
    printf("oor O4 99 0 -> %d\n", ori_axiom4(s, 99, 0, out));
    printf("oor O4 0 99 -> %d\n", ori_axiom4(s, 0, 99, out));
    printf("oor O7 0 -1 0 -> %d\n", ori_axiom7(s, 0, -1, 0, out));

    ori_free(s);
    printf("\n");
}

/* ------------------------------------------------------------------ */
/* T3 - verify.build(5), driven through the axioms                     */
/* ------------------------------------------------------------------ */
static int pidx(const ori_state *s, double x, double y)
{
    double d;
    int    i = ori_nearest_point(s, x, y, &d);
    if (i < 0 || d > 1e-9) {
        printf("!! no point at %+.9f %+.9f (nearest %d at %.3e)\n", x, y, i, d);
        return -1;
    }
    return i;
}

static void t3(void)
{
    printf("### T3 the five-fold reference sequence\n");
    ori_state *s = ori_new(1.0, SQRT2);
    if (!s) { printf("ori_new returned NULL\n\n"); return; }

    const double H = SQRT2;
    ori_line out[ORI_MAX_SOLUTIONS];
    int      n, idx;

    struct { const char *name; int step; } plan[] = {
        { "O2", 1 }, { "O1", 2 }, { "O2", 3 }, { "O3", 4 }, { "O2", 5 },
    };

    for (size_t k = 0; k < sizeof plan / sizeof plan[0]; k++) {
        switch (plan[k].step) {
        case 1: n = ori_axiom2(s, pidx(s, 0, 0), pidx(s, 0, H), out); break;
        case 2: n = ori_axiom1(s, pidx(s, 0, 0), pidx(s, 1, H), out); break;
        case 3: n = ori_axiom2(s, pidx(s, 0, H), pidx(s, 1, 0), out); break;
        case 4: n = ori_axiom3(s, 0, 4, out);                         break;
        case 5: n = ori_axiom2(s, pidx(s, 0, 0), pidx(s, 1, H / 2), out); break;
        default: n = 0;
        }
        if (n < 1) {
            printf("fold %zu: %s returned %d solutions, stopping\n",
                   k + 1, plan[k].name, n);
            break;
        }
        idx = ori_fold(s, out[0]);
        printf("fold %zu %s -> index %d  L %d  P %d  check %d\n",
               k + 1, plan[k].name, idx, ori_line_count(s),
               ori_point_count(s), ori_check(s) ? 1 : 0);
    }

    printf("--\n");
    dump_state(s);

    printf("undo %d\n", ori_undo(s) ? 1 : 0);
    printf("after undo  L %d  P %d  fold_count %d  check %d\n",
           ori_line_count(s), ori_point_count(s), ori_fold_count(s),
           ori_check(s) ? 1 : 0);

    ori_free(s);
    printf("\n");
}

/* ------------------------------------------------------------------ */
/* T4 - candidate counts (mirrors origami.candidates)                  */
/* ------------------------------------------------------------------ */
#define KEYCAP 8192
typedef struct { ori_lkey k[KEYCAP]; int n; } keyset;

static bool ks_has(const keyset *s, ori_lkey k)
{
    for (int i = 0; i < s->n; i++)
        if (ori__lkey_eq(s->k[i], k)) return true;
    return false;
}
static void ks_add(keyset *s, ori_lkey k)
{
    if (!ks_has(s, k) && s->n < KEYCAP) s->k[s->n++] = k;
}

static void take(const ori_state *st, const keyset *have, keyset *per,
                 keyset *uni, const ori_line *out, int n)
{
    for (int i = 0; i < n; i++) {
        ori_lkey k = ori__lkey(out[i]);
        if (ks_has(have, k))            continue;
        if (ori_chord(st, out[i]) <= 1e-6) continue;
        ks_add(per, k);
        ks_add(uni, k);
    }
}

static void t4_one(const ori_state *s, int depth)
{
    static keyset have, per[5], uni;
    memset(&have, 0, sizeof have);
    memset(per, 0, sizeof per);
    memset(&uni, 0, sizeof uni);

    int nP = ori_point_count(s), nL = ori_line_count(s);
    for (int i = 0; i < nL; i++) ks_add(&have, ori__lkey(ori_line_at(s, i)));

    ori_line out[ORI_MAX_SOLUTIONS];
    for (int i = 0; i < nP; i++)
        for (int j = i + 1; j < nP; j++) {
            take(s, &have, &per[0], &uni, out, ori_axiom1(s, i, j, out));
            take(s, &have, &per[1], &uni, out, ori_axiom2(s, i, j, out));
        }
    for (int i = 0; i < nL; i++)
        for (int j = i + 1; j < nL; j++)
            take(s, &have, &per[2], &uni, out, ori_axiom3(s, i, j, out));
    for (int p = 0; p < nP; p++)
        for (int l = 0; l < nL; l++)
            take(s, &have, &per[3], &uni, out, ori_axiom4(s, p, l, out));
    for (int p = 0; p < nP; p++)
        for (int x = 0; x < nL; x++)
            for (int y = 0; y < nL; y++)
                take(s, &have, &per[4], &uni, out, ori_axiom7(s, p, x, y, out));

    printf("depth %d P %d L %d | O1 %d O2 %d O3 %d O4 %d O7 %d | union %d\n",
           depth, nP, nL, per[0].n, per[1].n, per[2].n, per[3].n, per[4].n,
           uni.n);
}

static void t4(void)
{
    printf("### T4 closed-form candidate counts\n");
    ori_state *s = ori_new(1.0, SQRT2);
    if (!s) { printf("ori_new returned NULL\n\n"); return; }

    const double H = SQRT2;
    ori_line out[ORI_MAX_SOLUTIONS];

    t4_one(s, 0);
    for (int step = 1; step <= 5; step++) {
        int n = 0;
        switch (step) {
        case 1: n = ori_axiom2(s, pidx(s, 0, 0), pidx(s, 0, H), out); break;
        case 2: n = ori_axiom1(s, pidx(s, 0, 0), pidx(s, 1, H), out); break;
        case 3: n = ori_axiom2(s, pidx(s, 0, H), pidx(s, 1, 0), out); break;
        case 4: n = ori_axiom3(s, 0, 4, out);                         break;
        case 5: n = ori_axiom2(s, pidx(s, 0, 0), pidx(s, 1, H / 2), out); break;
        }
        if (n < 1 || ori_fold(s, out[0]) < 0) {
            printf("depth %d: could not build\n", step);
            break;
        }
        t4_one(s, step);
    }
    ori_free(s);
    printf("\n");
}

/* ------------------------------------------------------------------ */
int main(int argc, char **argv)
{
    int only = (argc > 1) ? atoi(argv[1]) : -1;
    if (only < 0 || only == 0) t0();
    if (only < 0 || only == 1) t1();
    if (only < 0 || only == 2) t2();
    if (only < 0 || only == 3) t3();
    if (only < 0 || only == 4) t4();
    return 0;
}
