# Directories
WORK_DIR = $(shell pwd)

# Source
SRC = $(wildcard src/*.c)
OBJ = $(addsuffix .o, $(basename $(SRC)))

# Flags
INCFLAGS := -I$(WORK_DIR)/include

CFLAGS = -fno-pic -static -O2 -Wall $(INCFLAGS) \
         -fno-stack-protector -ffreestanding -Wno-main

# Build target
LIB = $(BUILD_DIR)/libmmk_u.a

all: $(LIB)

$(LIB): $(OBJ)
	$(AR) -r $@ $^

# Compile: a single `.c` -> `.o` (gcc)
$(DST_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $(realpath $<)

clean:
	rm -rf $(OBJ)	

.PHONY: clean
