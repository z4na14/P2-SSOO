CC=gcc
FLAGS=-Wno-implicit-function-declaration
CFLAGS=-I.

SRC_DIR = ./src
BUILD_DIR = ./build
ZIP_NAME = ./tests/ssoo_p2_100522240_100522110_100522257.zip

SRCS = $(wildcard $(SRC_DIR)/*.c)

#############################################################
#			  PART USED FOR THE DEVELOPMENT WITH			#
#			  DIFFERENT FILE STRUCTURE TO THE TESTER		#
#############################################################
all: mygrep scripter

dev: mygrep_d scripter_d

.PHONY: mygrep_d
mygrep_d: $(BUILD_DIR)/mygrep

$(BUILD_DIR)/mygrep:  $(BUILD_DIR)/mygrep.o
	$(CC) $(FLAGS) $^ -o $@

$(BUILD_DIR)/mygrep.o: $(SRC_DIR)/mygrep.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(FLAGS) -c $< -o $@ $(CFLAGS)

.PHONY: scripter_d
scripter_d: $(BUILD_DIR)/scripter

$(BUILD_DIR)/scripter: $(BUILD_DIR)/scripter.o
	$(CC) $(FLAGS) $^ -o $@

$(BUILD_DIR)/scripter.o: $(SRC_DIR)/scripter.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(FLAGS) -c $< -o $@ $(CFLAGS)

clean:
	rm -rf $(BUILD_DIR)
	@mkdir -p $(BUILD_DIR)

.PHONY: zip
zip:
	zip -j $(ZIP_NAME) $(SRCS) Makefile autores.txt

#############################################################
#			    USED TO BUILD THE TESTER FILES				#
#############################################################
scripter: scripter.o
	$(CC) $(FLAGS) $^ -o $@
scripter.o: scripter.c
	$(CC) $(FLAGS) -c $< -o $@ $(CFLAGS)

mygrep: mygrep.o
	$(CC) $(FLAGS) $^ -o $@
mygrep.o: mygrep.c
	$(CC) $(FLAGS) -c $< -o $@ $(CFLAGS)
