CC      = gcc
C_FLAGS = -Wall -g
EXE     = pingpong-prodcons
DP      = ppos_core.c queue.c

all: $(EXE)

debug: C_FLAGS += -DDEBUG
debug: all

array: EXE = pingpong-prodcons-array
array: all

$(EXE): $(EXE).c $(DP)
	$(CC) $(C_FLAGS) $(DP) $(EXE).c -o $(EXE)

purge:
	rm $(EXE)