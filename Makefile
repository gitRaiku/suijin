all: build

.PHONY: build run debug clean remove tar

# Directories
TARGET   = suijin
SRC_DIR  = src
LIB_DIR  = lib
BIN_DIR  = bin
OBJ_DIR  = .obj
MOD_DIR  = .mod

# Options
CC       = gcc
CCFLAGS  = -ggdb3 -Og \
					 -Wall -march=native -mtune=native -fmodulo-sched \
					 -fstack-clash-protection -pthread \
					 -pipe -I$(LIB_DIR)/glad \
					 -fkeep-inline-functions \
					 -I/usr/include/freetype2 \
					 -D_FORTIFY_SOURCE=2 \
					 -DSUIJIN_DEBUG 

LINKER   = $(CC)

DEBUGGER = gdb

DATE := $(shell date "+%Y-%m-%d")

LFLAGS   = -lglfw -lm -lpng -lfreetype

C_SRC = $(wildcard $(SRC_DIR)/*.c $(SRC_DIR)/*/*.c)
L_SRC = $(wildcard $(LIB_DIR)/*.c $(LIB_DIR)/*/*.c)
C_OBJ = $(C_SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
L_OBJ = $(L_SRC:$(LIB_DIR)/%.c=$(OBJ_DIR)/%.o)
MOD   = $(wildcard $(MOD_DIR)/*.mod)

build: $(BIN_DIR) $(BIN_DIR)/$(TARGET)

$(BIN_DIR):
	mkdir -p $@

tt:
	@echo $(C_SRC)
	@echo $(C_OBJ)

$(C_OBJ): $(OBJ_DIR)/%.o : $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) -c $(CCFLAGS) $< -o $@

$(L_OBJ): $(OBJ_DIR)/%.o : $(LIB_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) -c $(CCFLAGS) $< -o $@

$(MOD_DIR):
	mkdir -p $@

$(BIN_DIR)/$(TARGET): $(C_OBJ) $(L_OBJ)
	$(LINKER) $(L_OBJ) $(C_OBJ) $(LFLAGS) -o $@

run: build
	./$(BIN_DIR)/$(TARGET)

debug: build
	$(DEBUGGER) $(BIN_DIR)/$(TARGET)

clean:
	rm -f $(C_OBJ)
	rm -f $(L_OBJ)
	rm -f $(MOD)

remove: clean
	rm -f $(BIN_DIR)/$(TARGET)

tar:
	tar -czvf suijin-$(DATE).tar.gz Makefile $(SRC_DIR)/* $(LIB_DIR)/* Resources AUTHORS
