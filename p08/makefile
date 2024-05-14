CC      = gcc
C_FLAGS = -Wall -g
EXE     = pingpong-wait
DP      = ppos_core.c queue.c

all: $(EXE)

debug: C_FLAGS += -DDEBUG
debug: all

v2: EXE = pingpong-wait-2
v2: all

$(EXE): $(EXE).c $(DP)
	$(CC) $(C_FLAGS) $(DP) $(EXE).c -o $(EXE)

purge:
	rm $(EXE)