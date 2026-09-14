# engine/tests/Makefile
#
# Lives one level below the engine. Nothing in this directory is ever
# compiled into the iOS app - test_ori.c has its own main().
#
#     make          build
#     make check    diff the C transcript against origami.py
#     make check S=0    stage T0 only
#
# -ffp-contract=off matters: without it clang fuses a*b - c*d into an FMA,
# which keeps an intermediate at higher precision and changes the last bits.

CC      = cc
CFLAGS  = -std=c11 -Wall -Wextra -Werror -O2 -g -ffp-contract=off -I..
LDLIBS  = -lm

ENGINE  = ../ori_geom.c ../ori_state.c
HDRS    = ../ori.h ../ori_internal.h
S      ?=

all: test_ori

# one command, no object files - the engine is small and this way there
# is no such thing as a stale .o
test_ori: test_ori.c $(ENGINE) $(HDRS)
	$(CC) $(CFLAGS) -o $@ test_ori.c $(ENGINE) $(LDLIBS)

# origami.py sits in ../, so point Python at it
check: test_ori
	@./test_ori $(S) > .c.txt
	@PYTHONPATH=.. python3 oracle.py $(S) > .py.txt
	@if diff -u .py.txt .c.txt > .diff.txt; then \
	  echo "MATCH  ($$(wc -l < .c.txt) lines)"; \
	else \
	  echo "DIFF   (python < , c > )"; head -60 .diff.txt; \
	  echo "... full diff in .diff.txt"; exit 1; \
	fi

clean:
	rm -f test_ori .c.txt .py.txt .diff.txt
	rm -rf __pycache__ ../__pycache__

.PHONY: all check clean