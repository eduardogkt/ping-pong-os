CC      = gcc
C_FLAGS = -Wall -g
EXE     = pingpong-mqueue
DP      = ppos_core.c queue.c
LIB     = -lm

all: $(EXE)

prod: EXE = pingpong-prodcons
prod: all

mqueue: EXE = pingpong-mqueue
mqueue: all

race: EXE = pingpong-racecond
race: all

sem: EXE = pingpong-semaphore
sem: all

purgem: EXE = pingpong-mqueue
purgem: purge

purgep: EXE = pingpong-prodcons
purgep: purge

purger: EXE = pingpong-racecond
purger: purge

purges: EXE = pingpong-semaphore
purges: all

debug: C_FLAGS += -DDEBUG
debug: all

$(EXE): $(EXE).c $(DP)
	$(CC) $(C_FLAGS) $(DP) $(EXE).c -o $(EXE) $(LIB) 

purge:
	rm $(EXE)