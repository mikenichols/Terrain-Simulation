#include <GL/glut.h>

#include <stdio.h>
#include <stdlib.h>
#include "globals.h"

// Fail hard when we hit an error that should be fixed
#ifndef SHOULD_HALT_AND_CATCH_FIRE
#define SHOULD_HALT_AND_CATCH_FIRE
#endif

///////////////////////////
// Global Utility Functions

inline void unreachable(char *file, int line) {
    printf("ERROR: %s:%d Unreachable stated reached.\n", file, line);
    #ifdef SHOULD_HALT_AND_CATCH_FIRE
    exit(1);
    #endif
    return;
}

inline void check_for_null(void *ptr, char *file, int line) {
    if (ptr == NULL) {
        fprintf(stderr, "ERROR: %s:%d: Null pointer found where not expected.\n",
                file, line);
        #ifdef SHOULD_HALT_AND_CATCH_FIRE
        exit(1);
        #endif
    }
    return;
}

inline void check_for_glerror(GLenum err, char *file, int line) {
    // get the error string if there's been an error
    if (err != GL_NO_ERROR) {
        const GLubyte *err_string = gluGetString(err);
        fprintf(stderr, "GL_ERROR: %s:%d: %s\n", file, line, err_string);
        #ifdef SHOULD_HALT_AND_CATCH_FIRE
        exit(1);
        #endif
    }
    return;
}

inline void check_array_bounds(int index, int size, char *file, int line) {
    if (index < 0 || index >= size) {
        fprintf(stderr, "ERROR: %s:%d Array out of bounds.\n", file, line);
        #ifdef SHOULD_HALT_AND_CATCH_FIRE
        exit(1);
        #endif
    }
    return;
}

///////////////////
// Global Constants

const double PI = 3.14159265358979;
const float MOVE_SCALE = 10.0;
const float Z_SCALE = 1.0 / 20.0;
const float FOVY = 60.0;
const void *BIG_FONT = GLUT_BITMAP_HELVETICA_18;
const int NUM_IBUFFERS = 7;
const short DATA_VOID = -32768;
const int NUM_COLORS = 5;
const int POINTS_PER_COORD = 4;

///////////////////
// Global Variables

int win_id;
int screen_width = 1280;
int screen_height = 720;

int fps_count = 0;
int fps_size = 60;
float fps_total;
float *fps_array;

vec3f eye = {3600, 3600, 50};
#ifdef LOOK_DOWN
vec3f look = {0.0, 0.0, -1.0};
vec3f up = {0.0, 1.0, 0.0};
#else
vec3f look = {0.0, 1.0, 0.0};
vec3f up = {0.0, 0.0, 1.0};
#endif
float y_rot;
float rotate_step;

// clipmap variables
int num_cmaps;
int cmap_size;
vec3f *cmap_center;
vec2i *cmap_start_index;
int finest_level;

// terrain data variables
int SIZE;
short *terrain_data;
int terrain_size;
int lat_range, lon_range;
int lon_min, lat_min, lon_max, lat_max;

GLuint terrain_id;
GLuint *cmap_vbuf_id;
float *vertex_data;

// index data variables
GLuint *index_id;
indexsize_t index_size;
vec2i *index_section_offset;

// texture variables
GLuint *textures;
GLuint *texbuf_id;
int num_textures = 1;
GLfloat *tex_coords;

// Shader Data
GLuint shader_prog_id;
