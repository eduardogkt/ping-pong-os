#########################################################################
###   AUTHOR:  Eduardo Gabriel Kenzo Tanaka                           ###
###   WARNING: Do not change OBJ_DIR or UTL_DIR                       ###
###   USAGE:   make [clean/purge] e=executable s=source_dir           ###
#########################################################################

e =  # executable file to be specified by the user
s =  # source directory to be specified by the user

CC      = gcc
C_FLAGS = -Wall -std=c99 -g
EXE     = $(e)
SRC_DIR = $(s)
OBJ_DIR = objs
UTL_DIR = utils

SRC_FILES  = $(wildcard $(SRC_DIR)/*.c)
HDR_FILES  = $(wildcard $(SRC_DIR)/*.h)
UTL_FILES  = $(wildcard $(UTL_DIR)/*.c)
OBJ_FILES  = $(patsubst $(SRC_DIR)/%, $(OBJ_DIR)/%, $(SRC_FILES:.c=.o))
OBJ_FILES += $(patsubst $(UTL_DIR)/%, $(OBJ_DIR)/%, $(UTL_FILES:.c=.o))

# makefile input check
ifndef e
	$(error Executable directory not specified.\
	        Use 'make [clean/purge] e=executable s=source_dir')
endif

ifndef s
    $(error Source directory not specified.\
	        Use 'make [clean/purge] e=executable s=source_dir')
endif

# default rule
all: mkdir_obj $(EXE)

# binding rules
$(EXE): $(OBJ_FILES)
	$(CC) $(C_FLAGS) -o $@ $^

# compilation rules
$(OBJ_DIR)/$(EXE).o: $(SRC_DIR)/$(EXE).c $(HDR_FILES)
	$(CC) $(C_FLAGS) -o $@ -c $<

$(OBJ_DIR)/%.o: $(UTL_DIR)/%.c $(UTL_DIR)/%.h
	$(CC) $(C_FLAGS) -o $@ -c $<

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(SRC_DIR)/%.h
	$(CC) $(C_FLAGS) -o $@ -c $<

# utility rules
mkdir_obj:
	@ mkdir -p $(OBJ_DIR)

run: mkdir_obj $(EXE)
	@ ./$(EXE)

clean:
	rm -f $(OBJ_FILES)
	rmdir $(OBJ_DIR)

purge: clean
	rm -f $(EXE)

# $@ target of the current rule
# $< terget dependency of the current rule
# $^ all dependences of the current target
# @  suppress (don't display) the command
