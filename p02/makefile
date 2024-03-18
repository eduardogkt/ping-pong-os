CC      = gcc
C_FLAGS = -Wall -std=c99 -g
TESTS   = pingpong-tasks1 pingpong-tasks2 pingpong-tasks3

all: $(TESTS)

debug: C_FLAGS += -DDEBUG
debug: all

pingpong-tasks1: pingpong-tasks1.c
	$(CC) $(C_FLAGS) ppos_core.c pingpong-tasks1.c -o pingpong-tasks1

pingpong-tasks2: pingpong-tasks2.c
	$(CC) $(C_FLAGS) ppos_core.c pingpong-tasks2.c -o pingpong-tasks2

pingpong-tasks3: pingpong-tasks3.c
	$(CC) $(C_FLAGS) ppos_core.c pingpong-tasks3.c -o pingpong-tasks3

purge:
	rm $(TESTS)

test: test.c ppos_core.c
	$(CC) $(C_FLAGS) ppos_core.c test.c -o test