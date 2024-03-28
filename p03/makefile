CC      = gcc
C_FLAGS = -Wall -std=c99 -g
TEST    = pingpong-dispatcher

all: $(TEST)

debug: C_FLAGS += -DDEBUG
debug: all

pingpong-dispatcher: pingpong-dispatcher.c ppos_core.c queue.c
	$(CC) $(C_FLAGS) ppos_core.c queue.c pingpong-dispatcher.c -o pingpong-dispatcher

purge:
	rm $(TEST)

test: test.c ppos_core.c
	$(CC) $(C_FLAGS) ppos_core.c test.c -o test