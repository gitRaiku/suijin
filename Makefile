all: build

.PHONY: build run debug clean remove tar

# Directories
TARGET   = suijin
SHARED   = suijin.so
SRC_DIR  = src
LIB_DIR  = lib
BIN_DIR  = bin
OBJ_DIR  = .obj

# Options
CC       = gcc
CCFLAGS  = -ggdb3 -Og \
					 -Wall -march=native -mtune=native -fmodulo-sched \
					 -fstack-clash-protection -pthread \
					 -pipe -I$(LIB_DIR)/glad \
					 -fkeep-inline-functions \
					 -I/usr/include/freetype2 \
					 -D_FORTIFY_SOURCE=2 \
					 -DGLAD_GLAPI_EXPORT -DGLAD_GLAPI_EXPORT_BUILD \
					 -DSUIJIN_DEBUG -rdynamic

LINKER   = $(CC)

DEBUGGER = gdb -q
VALGRIND = valgrind --leak-check=summary --log-file=valgrind.log

DATE := $(shell date "+%Y-%m-%d")

LFLAGS = -lglfw -lm -lpng -lfreetype

C_SRC = $(filter-out src/main.c,$(wildcard $(SRC_DIR)/*.c $(SRC_DIR)/*/*.c))
L_SRC = $(wildcard $(LIB_DIR)/*.c $(LIB_DIR)/*/*.c)
C_OBJ = $(C_SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
L_OBJ = $(L_SRC:$(LIB_DIR)/%.c=$(OBJ_DIR)/%.o)

build: $(BIN_DIR) $(BIN_DIR)/$(TARGET) $(BIN_DIR)/$(SHARED)

$(BIN_DIR):
	mkdir -p $@

tt:
	@echo $(C_SRC)
	@echo $(C_OBJ)

$(C_OBJ): $(OBJ_DIR)/%.o : $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) -c -fPIC $(CCFLAGS) $< -o $@

$(L_OBJ): $(OBJ_DIR)/%.o : $(LIB_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) -c -fPIC $(CCFLAGS) $< -o $@

$(BIN_DIR)/$(TARGET): $(SRC_DIR)/main.c $(L_OBJ)
	unlink $@
	$(CC) -o $(BIN_DIR)/$(TARGET) $(CCFLAGS) $(LFLAGS) $(L_OBJ) $<

$(BIN_DIR)/$(SHARED): $(C_OBJ) $(L_OBJ)
	$(CC) -shared -fPIC -o $@ $(C_OBJ) $(L_OBJ)

run: build
	./$(BIN_DIR)/$(TARGET)

valgrind: build
	$(VALGRIND) $(BIN_DIR)/$(TARGET)

debug: build
	$(DEBUGGER) $(BIN_DIR)/$(TARGET)

clean:
	rm -f $(C_OBJ)
	rm -f $(L_OBJ)
	rm -f $(BIN_DIR)/$(TARGET) $(BIN_DIR)/$(SHARED)

remove: clean
	rm -f $(BIN_DIR)/$(TARGET)

tar:
	tar -czvf suijin-$(DATE).tar.gz Makefile $(SRC_DIR)/* $(LIB_DIR)/* Resources AUTHORS
