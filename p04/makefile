CC      = gcc
C_FLAGS = -Wall -std=c99 -g
EXE    = pingpong-scheduler

all: $(EXE)

debug: C_FLAGS += -DDEBUG
debug: all

$(EXE): $(EXE).c ppos_core.c queue.c
	$(CC) $(C_FLAGS) ppos_core.c queue.c  $(EXE).c -o $(EXE)

purge:
	rm $(EXE)

test: test.c ppos_core.c
	$(CC) $(C_FLAGS) ppos_core.c test.c -o test