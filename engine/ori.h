/*
 * ori.h - origami axiom engine
 *
 * Design rules for this boundary:
 *   1. The engine NEVER allocates on the caller's behalf. The caller
 *      supplies every output buffer. This removes all ownership questions
 *      at the Swift boundary.
 *   2. Points and lines are addressed by INDEX, never by pointer. Indices
 *      are stable for the lifetime of a state and bridge trivially.
 *   3. Small structs are returned by value. That is safe and cheap across
 *      the C/Swift boundary.
 *   4. There is no separate "is this legal?" predicate. Every axiom returns
 *      a solution count; 0 means the operand choice is dead. One code path,
 *      so the UI can never disagree with the engine.
 */

#ifndef ORI_H
#define ORI_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/* geometry                                                           */
/* ------------------------------------------------------------------ */

typedef struct { double x, y; } ori_pt;

/* a*x + b*y = c, normalised so a*a + b*b == 1, with a canonical sign
 * (a > 0, or a == 0 and b > 0) so that equal lines compare equal. */
typedef struct { double a, b, c; } ori_line;

/* Any axiom yields at most this many creases (O6 is the cubic one). */
#define ORI_MAX_SOLUTIONS 3

/* ------------------------------------------------------------------ */
/* state                                                              */
/* ------------------------------------------------------------------ */

typedef struct ori_state ori_state;

/* Rectangular sheet, corner at the origin. Returns NULL on failure. */
ori_state *ori_new(double w, double h);
void       ori_free(ori_state *s);

/* ------------------------------------------------------------------ */
/* inspection                                                         */
/* ------------------------------------------------------------------ */

int      ori_point_count(const ori_state *s);
ori_pt   ori_point(const ori_state *s, int i);      /* i out of range -> (0,0) */

int      ori_line_count(const ori_state *s);
ori_line ori_line_at(const ori_state *s, int i);

/* Length of the part of `l` lying inside the sheet. 0 if it misses. */
double   ori_chord(const ori_state *s, ori_line l);

/* Nearest point to (x, y) in PAPER coordinates; the UI converts from
 * screen space. Returns -1 if there are no points. `out_dist` may be NULL. */
int      ori_nearest_point(const ori_state *s, double x, double y,
                           double *out_dist);

/* ------------------------------------------------------------------ */
/* axioms                                                             */
/*                                                                    */
/* Each writes its creases to `out` and returns how many it wrote.     */
/* A return of 0 means "no such fold exists" - use that to grey out    */
/* operands in the UI. Arguments are indices into the current state.   */
/* ------------------------------------------------------------------ */

/* through p1 and p2 */
int ori_axiom1(const ori_state *s, int p1, int p2, ori_line *out /* [1] */);

/* place p1 onto p2 */
int ori_axiom2(const ori_state *s, int p1, int p2, ori_line *out /* [1] */);

/* place l1 onto l2 (two bisectors, one if parallel) */
int ori_axiom3(const ori_state *s, int l1, int l2, ori_line *out /* [2] */);

/* through p, perpendicular to l */
int ori_axiom4(const ori_state *s, int p, int l, ori_line *out /* [1] */);

/* place p1 onto l1, crease passing through p2 */
int ori_axiom5(const ori_state *s, int p1, int l1, int p2, ori_line *out /* [2] */);

/* place p1 onto l1 AND p2 onto l2 - solves a cubic, up to 3 creases */
int ori_axiom6(const ori_state *s, int p1, int l1, int p2, int l2,
               ori_line *out /* [3] */);

/* place p1 onto l1, crease perpendicular to l2 */
int ori_axiom7(const ori_state *s, int p1, int l1, int l2, ori_line *out /* [1] */);

/* ------------------------------------------------------------------ */
/* mutation                                                           */
/* ------------------------------------------------------------------ */

/* Commit a crease: adds the line, then adds every new intersection point
 * that falls inside the sheet. Returns the new line's index, or -1 if the
 * crease already exists or does not cross the sheet. */
int  ori_fold(ori_state *s, ori_line crease);

/* Undo the most recent fold. False if there is nothing to undo. */
bool ori_undo(ori_state *s);

int  ori_fold_count(const ori_state *s);

/* ------------------------------------------------------------------ */
/* testing hooks - keep these, they are how you stay honest            */
/* ------------------------------------------------------------------ */

/* Checks internal invariants: lines normalised and canonically signed,
 * no duplicate lines or points, every point inside the sheet. */
bool ori_check(const ori_state *s);

/* Deterministic text dump, for diffing against the Python reference.
 * Writes at most `cap` bytes (always NUL-terminated when cap > 0) and
 * returns the number of bytes the full dump would need. */
int  ori_dump(const ori_state *s, char *buf, int cap);

#ifdef __cplusplus
}
#endif

#endif /* ORI_H */
