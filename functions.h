#ifndef FUNCTIONS_H
#define FUNCTIONS_H

//#include <GL/glut.h>
#include "typedefs.h"

void check_and_set_cmap_size();
void load_data(char *fnames);
void load_textures();
void fix_nulls();
void read_from_file(char *fname, short *data);
void latlon_from_filename(char *name, int *lat, int *lon);
void jpeg2texture(int texNum, char *imageName);

void initialize_clipmaps();
void initialize_index_buffers();

void compute_indices(int level);
void compute_wireframe_indices_rect(int num_indices, int row_len, GLushort *indices, vec2i offset);
void compute_texture_indices_rect(int num_indices, int num_cols, GLushort *indices, vec2i offset);
void compute_wireframe_indices_Lregion(int num_indices, GLushort *indices, vec2i level_offset, int level);
void compute_texture_indices_Lregion(int num_indices, GLushort *indices, vec2i level_offset, int level);

void render_terrain_level(int level);
void update_clipmap_center(int level, int *rows_to_update, int *cols_to_update);
void update_clipmap_level(int level, int rows_to_update, int cols_to_update);
void update_clipmap_subregion(int level, float *cmap_data, vec2i world_start, vec2i cmap_start, vec2i max);

bool is_terrain_index_valid(vec2i world);
void crop_active_regions();
inline void verify_cmap_centers();
void check_for_invalid_data();
inline void draw_bitmap_string(float x, float y, float z, void *font, char *text);
inline void set_wireframe_color(int level, int total);
inline int verify_power_of_two(unsigned int x);

#endif // FUNCTIONS_H
