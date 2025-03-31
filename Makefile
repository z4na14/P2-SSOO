CC=gcc
FLAGS=-Wno-implicit-function-declaration
CFLAGS=-I.

SRC_DIR = ./src
BUILD_DIR = ./build
ZIP_NAME = ./tests/ssoo_p2_100522240_100522110_100522257.zip

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

all: mygrep scripter

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(FLAGS) -c $< -o $@ $(CFLAGS)

.PHONY: mygrep
mygrep: $(BUILD_DIR)/mygrep

$(BUILD_DIR)/mygrep:  $(BUILD_DIR)/mygrep.o
	$(CC) $(FLAGS) $^ -o $@

.PHONY: scripter
scripter: $(BUILD_DIR)/scripter

$(BUILD_DIR)/scripter: $(BUILD_DIR)/scripter.o
	$(CC) $(FLAGS) $^ -o $@

clean:
	rm -rf $(BUILD_DIR)
	rm $(ZIP_NAME)
	@mkdir -p $(BUILD_DIR)

.PHONY: zip
zip:
	zip -j $(ZIP_NAME) $(SRCS) Makefile autores.txt
