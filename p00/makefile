CC      = gcc
C_FLAGS = -Wall -std=c99 -g
EXE     = testafila
OBJ     = queue

all: $(EXE)

$(EXE): $(OBJ).o $(EXE).c
	$(CC) $(C_FLAGS) $(EXE).c $(OBJ).o -o $(EXE)

$(OBJ).o: $(OBJ).c $(OBJ).h
	$(CC) $(C_FLAGS) -c $(OBJ).c -o $(OBJ).o

clean:
	rm *.o

purge: clean
	rm $(EXE)