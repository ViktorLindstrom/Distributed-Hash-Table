# Compiler and flags
CC := gcc
CFLAGS := -Wall -O2 -Iresources/Hashtable -Iresources
LDFLAGS := 

# Directories
SRC_DIR := src
OBJ_DIR := bin/objs
HASH_TABLE_DIR := resources/Hashtable

# Sources and objects
SRCS := $(wildcard $(SRC_DIR)/*.c) \
        $(wildcard $(HASH_TABLE_DIR)/*.c)
OBJS := $(patsubst %.c, $(OBJ_DIR)/%.o, $(notdir $(SRCS)))

# Executable
TARGET := bin/run_node

# Rules
all: $(TARGET)

# Link the final executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compile C files into objects
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(HASH_TABLE_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Specific dependency for c_node.c on pdu.h
$(OBJ_DIR)/c_node.o: $(SRC_DIR)/c_node.c resources/pdu.h

# Create object directory if it doesn't exist
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Clean up build files
clean:
	rm -rf $(OBJ_DIR) $(TARGET)
	rm -rf bin

cnode: $(TARGET)
	./$(TARGET) 0.0.0.0 7777
rnode:
	cargo run --manifest-path $(CURDIR)/resources/TestNode/Cargo.toml --bin node 0.0.0.0 7777
tracker:
	cargo run --manifest-path $(CURDIR)/resources/TestNode/Cargo.toml --bin tracker 7777 
client:
	cargo run --manifest-path $(CURDIR)/resources/TestNode/Cargo.toml --bin client -- --tracker 0.0.0.0:7777 --csv resources/TestData/data.csv
.PHONY: all clean