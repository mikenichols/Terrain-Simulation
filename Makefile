CC = gcc
CFLAGS = -g -Wall

INCLUDE = -I/u/home2/mykphyre/include
LINK = -L/u/home2/mykphyre/lib
LIBS = -lGL -lglut -lIL -lILU -lILUT

all: main.o functions.o gl_functions.o globals.o typedefs.o movement.o shaders.o
	$(CC) $(CFLAGS) main.o functions.o gl_functions.o globals.o typedefs.o movement.o shaders.o -o run $(LINK) $(LIBS)

# compile all .c files into .o files in this way
.c.o:
	$(CC) $(CFLAGS) -c $< $(INCLUDE)

clean:
	rm run *.o
