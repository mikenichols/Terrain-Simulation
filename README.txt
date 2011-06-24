Terrain Rendering Engine
Written By: Mike Nichols

Run with the command:
~$ ./run 3 31 "top_data/*.hgt"
or for a large terrain:
~$ ./run 10 255 "top_data/*.hgt"

Note that the second argument is of the form (2^n)-1 where n is an integer

Movement:
w: move forward
s: move backward
a: turn left
d: turn right
up arrow: move up
down arrow: move down
left arrow: strafe left
right arrow: strafe right

run this command:
valgrind --tool=memcheck --error-limit=no --track-origins=yes --leak-check=yes ./run 10 31 "top_data/*.hgt"
