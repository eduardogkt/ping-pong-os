CC      = gcc
C_FLAGS = -Wall -g
EXE     = pingpong-mqueue
DP      = ppos_core.c queue.c
LIB     = -lm

all: $(EXE)

debug: C_FLAGS += -DDEBUG
debug: all

$(EXE): $(EXE).c $(DP)
	$(CC) $(C_FLAGS) $(DP) $(EXE).c -o $(EXE) $(LIB) 

purge:
	rm $(EXE)