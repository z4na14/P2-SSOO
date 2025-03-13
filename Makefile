CC=gcc
FLAGS=-02 -Wall -Wextra -Werror -Wno-implicit-function-declaration
CFLAGS=-I./src/inc
###############################

OBJ = scripter.o

all: scripter

%.o: %.c 
	$(CC) $(FLAGS) -c -o $@ $< $(CFLAGS)

scripter: $(OBJ)
	$(CC) $(FLAGS) -L. -o $@ $< $(LIBS)

clean:
	rm -f ./scripter.o ./scripter
