CC=gcc
CFLAGS= -Wall -Wextra -Werror -Wno-implicit-function-declaration
SRC_DIR = ./src
BUILD_DIR = ./build

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

all: mygrepbld scripterbld

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: mygrepbld
mygrepbld: $(BUILD_DIR)/mygrep

$(BUILD_DIR)/mygrep:  $(BUILD_DIR)/mygrep.o
	$(CC) $(CFLAGS) $^ -o $@

.PHONY: scripterbld
scripterbld: $(BUILD_DIR)/scripter

$(BUILD_DIR)/scripter: $(BUILD_DIR)/scripter.o
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -rf $(BUILD_DIR)
	@mkdir -p $(BUILD_DIR)
