CC      = gcc
C_FLAGS = -Wall -g
EXE     = pingpong-semaphore
DP      = ppos_core.c queue.c

all: $(EXE)

debug: C_FLAGS += -DDEBUG
debug: all

debugr: C_FLAGS += -DDEBUG
debugr: EXE = pingpong-racecond
debugr: all

race: EXE = pingpong-racecond
race: all

$(EXE): $(EXE).c $(DP)
	$(CC) $(C_FLAGS) $(DP) $(EXE).c -o $(EXE)

purge:
	rm $(EXE)