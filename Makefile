CC = /usr/local/Cellar/gcc49/4.9.3/bin/gcc-4.9
CFLAGS = -g -Wall

LIBS = -framework opengl -lglut -lIL -lILU -lILUT

all: main.o functions.o gl_functions.o globals.o typedefs.o movement.o shaders.o
	$(CC) $(CFLAGS) main.o functions.o gl_functions.o globals.o typedefs.o movement.o shaders.o -o run $(LIBS)

# compile all .c files into .o files in this way
.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm run *.o
