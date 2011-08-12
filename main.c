// Terrain Renderer by Mike Nichols
// UCLA Academic Technology Services
// Based on Geometry Clipmaps: Terrain Rendering Using Nested Regular
// Grids. Losasso and Hoppe, SIGGRAPH 2004

// This #define must be included before including glut.h
#define GL_GLEXT_PROTOTYPES
// This header includes all necessary gl* headers
// Must include this before other headers
#include <GL/glut.h>

#include <IL/il.h>
#define ILUT_USE_OPENGL
#include <IL/ilu.h>
#include <IL/ilut.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "functions.h"
#include "gl_functions.h"
#include "globals.h"

#define CONSOLE_OUTPUT

// uncomment the line below to launch the simulation in fullscreen mode
//#define FULLSCREEN

#ifndef DEBUG
#define DEBUG
#endif

void at_exit() {
    // free the glut window
    glutDestroyWindow(win_id);

    // free vertex buffers
    glDeleteBuffers(num_cmaps, cmap_vbuf_id);
    glDeleteBuffers(NUM_IBUFFERS, index_id);

    // free allocated memory
    free (cmap_vbuf_id);
    free (terrain_data);
    free (cmap_center);
    free (cmap_start_index);
    free (index_id);
    free (index_section_offset);
    free (fps_array);
    free (textures);
    free (texbuf_id);

    printf("Simulation Complete.\n");
    return;
}

int main(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr, "Usage: ./run <num_cmaps> <clipmap_size> \"filename regex\"\n");
        exit(1);
    }

    num_cmaps = atoi(argv[1]);
    cmap_size = atoi(argv[2]);
    check_and_set_cmap_size();
    #ifdef CONSOLE_OUTPUT
    printf("Using %d levels with clipmap size %d\n", num_cmaps, cmap_size);
    #endif

    // OpenGL setup
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH | GLUT_MULTISAMPLE);
    glutInitWindowSize(screen_width, screen_height);
    win_id = glutCreateWindow("Terrain Renderer");
    #ifdef FULLSCREEN
    glutFullScreen();
    #endif

    // callback functions
    glutDisplayFunc(redraw);
    glutReshapeFunc(reshape);
    glutIdleFunc(animate);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutKeyboardFunc(key);
    glutSpecialFunc(specialKey);

    /* for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // */

    // devIL setup
    ilInit();
    iluInit();
    ilutInit();
    ilutRenderer(ILUT_OPENGL);

    // initialize global variables
    y_rot = 3*PI / 4.0;
    look.x = -cos(y_rot);
    look.y = sin(y_rot);
    rotate_step = 5.0*PI / 180.0;

    lon_min = 181;
    lat_min = 181;
    lon_max = -181;
    lat_max = -181;

    SIZE = 1200;

    finest_level = 0;

    fps_array = (float*)calloc(fps_size, sizeof(float));
    check_for_null((void*)fps_array, __FILE__, __LINE__);
    fps_total = 6.0;
    int i;
    for (i = 0; i < fps_size; i++) {
        fps_array[i] = 0.1;
    }

    atexit(at_exit);

    // initialize all data
    load_data(argv[3]);
    #ifdef DEBUG2
    check_for_invalid_data();
    exit(1);
    #endif

    initialize_clipmaps();
    initialize_index_buffers();
    load_textures();

    printf("Starting Simulation.\n");
    glutMainLoop();

    return 0;
}
