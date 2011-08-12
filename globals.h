#ifndef GLOBALS_H
#define GLOBALS_H

#define GL_GLEXT_PROTOTYPES
#include <GL/glut.h>

#include "typedefs.h"

#define BUFFER_OFFSET(bytes)  ((GLubyte*) NULL + (bytes))

//#define LOOK_DOWN

///////////////////////////
// Global Utility Functions

inline void unreachable(char *file, int line);
inline void check_for_null(void *ptr, char *file, int line);
inline void check_for_glerror(GLenum err, char *file, int line);
inline void check_array_bounds(int index, int size, char *file, int line);

///////////////////
// Global Constants

extern const double PI;
extern const float MOVE_SCALE;
extern const float Z_SCALE;
extern const float FOVY;
extern const void *BIG_FONT;
extern const int NUM_IBUFFERS;
extern const short DATA_VOID;
extern const int NUM_COLORS;
extern const int POINTS_PER_COORD;

///////////////////
// Global Variables

extern int win_id;
extern int screen_width;
extern int screen_height;

extern int fps_count;
extern int fps_size;
extern float fps_total;
extern float *fps_array;

extern vec3f eye;
extern vec3f look;
extern vec3f up;
// rotation amount in radians
extern float y_rot;
extern float rotate_step;

extern int num_cmaps;
extern int cmap_size;
extern vec3f *cmap_center;
extern vec2i *cmap_start_index;
extern int finest_level;

// this is the number of rows/cols for input data
extern int SIZE;
extern short *terrain_data;
extern int terrain_size;
extern int lat_range, lon_range;
extern int lon_min, lat_min, lon_max, lat_max;

// Vertex Data
extern GLuint terrain_id;
// pointer to VBO ids
extern GLuint *cmap_vbuf_id;
extern float *vertex_data;

// Index Data
// pointer to IBO ids
extern GLuint *index_id;
extern indexsize_t index_size;
extern vec2i *index_section_offset;

// Texture Data
// texture ids
extern GLuint *textures;
// texure buffer ids
extern GLuint *texbuf_id;
extern int num_textures;
extern GLfloat *tex_coords;

// Shader Data
extern GLuint shader_prog_id;

#endif // GLOBALS_H
