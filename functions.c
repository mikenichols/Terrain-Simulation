#define GL_GLEXT_PROTOTYPES
#include <GL/glut.h>

#include <IL/il.h>
#define ILUT_USE_OPENGL
#include <IL/ilu.h>
#include <IL/ilut.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <glob.h>
#include <wordexp.h>
#include "globals.h"
#include "functions.h"
#include "shaders.h"

#ifndef DEBUG
#define DEBUG
#endif

#ifndef FUNC_NAMES
//#define FUNC_NAMES
#endif

#ifndef COPIOUS_ERROR_CHECKING
#define COPIOUS_ERROR_CHECKING
#endif

#ifndef USE_TEXTURES
//#define USE_TEXTURES
#endif

void check_and_set_cmap_size() {
    if (verify_power_of_two(cmap_size + 1) == 0) {
        fprintf(stderr, "WARNING: specified clipmap size not of form (2^n)-1.\n");
        fprintf(stderr, "Setting clipmap size to next lowest valid number\n");
        while (verify_power_of_two(cmap_size + 1) == 0) {
            cmap_size--;
        }
    }
    // In practice, you'd never want to set the cmap_size this low, but at 7 it
    // starts looking pretty bad.
    if (cmap_size < 15) {
        cmap_size = 15;
    }
    // FIXME: This is the max size allowed by shorts. Ints can support up to 65536x65536
    // 255*255 = 65025; MAX_SHORT = 2^16 = 65536
    // The reason for this limit is that the indices can be stored as shorts if
    // they only need to index 255x255 and they take up half the space.
    if (cmap_size > 255) {
        fprintf(stderr, "WARNING: Largest clipmap size currently supported is 255\n");
        cmap_size = 255;
    }
    return;
}

void load_data(char *fnames) {
    // fnames is wildcarded list of files to load
    char **file_list;
    wordexp_t p;

    wordexp(fnames, &p, 0);
    int num_files = p.we_wordc;
    file_list = p.we_wordv;

    int lat, lon;
    char *name, *orig_name;

    int i;
    // this loop is to get the ranges for longitude and latitude
    for (i = 0; i < num_files; i++) {
        orig_name = file_list[i];
        name = orig_name;

        // extract the lat and lon from filename
        latlon_from_filename(name, &lat, &lon);

        // update min/maxs
        if (lat > lat_max) lat_max = lat;
        if (lat < lat_min) lat_min = lat;
        if (lon > lon_max) lon_max = lon;
        if (lon < lon_min) lon_min = lon;
    }

    // adding 1 because we want inclusive range
    lat_range = lat_max - lat_min + 1;
    lon_range = lon_max - lon_min + 1;
    #ifdef DEBUG
    printf("lon_range = %d lat_range = %d\n", lon_range, lat_range);
    printf("lon %d to %d lat %d to %d\n", lon_min, lon_max, lat_min, lat_max);
    #endif

    // Input files cover a range of lon_range*lat_range and each has SIZE*SIZE
    // points giving us lon_range*lat_range * SIZE*SIZE total points
    // TODO: make sure having blank squares is ok. For example, a row or column
    // may only have 1 valid data square in it but as it is now, there are
    // lots of possibly blank entries in terrain_data.
    terrain_size = lon_range*lat_range * SIZE*SIZE;

    // TODO: compress initially and decompress as needed
    terrain_data = (short*)calloc(terrain_size, sizeof(short));
    check_for_null((void*)terrain_data, __FILE__, __LINE__);

    // data for read_from_file to use
    short *data = (short*)calloc(SIZE*SIZE, sizeof(short));
    check_for_null((void*)data, __FILE__, __LINE__);

    // read the data into terrain array
    for (i = 0; i < num_files; i++) {
        orig_name = file_list[i];
        name = orig_name;

        // extract the lat and lon from filename
        latlon_from_filename(name, &lat, &lon);

        // compute the x and y index for this grid square
        int x_index = lon - lon_min;
        int y_index = lat - lat_min;

        #ifdef DEBUG_OLD
        printf("file %d: %s\n", i, orig_name);
        printf("current square at (%d,%d) has index (%d,%d)\n",
                lon, lat, x_index, y_index);
        #endif

        read_from_file(orig_name, data);

        // x_step = 1
        int y_step = lon_range * SIZE;

        int x_offset = x_index * SIZE;
        int y_offset = y_index * SIZE * y_step;

        int x, y;
        // copy the data into the big array
        for (y = 0; y < SIZE; y++) {
            for (x = 0; x < SIZE; x++) {
                terrain_data[y_offset + y*y_step + x_offset + x] = data[y*SIZE + x];
            }
        }
    }
    wordfree(&p);
    free(data);
    return;
}

void load_textures() {
    // TODO: set num_textures in load_files()
    num_textures = 1;
    textures = (GLuint*)calloc(num_textures, sizeof(GLuint));
    check_for_null((void*)textures, __FILE__, __LINE__);

    glGenTextures(num_textures, textures);
    check_for_glerror(glGetError(), __FILE__, __LINE__);

    int i;
    for (i = 0; i < num_textures; i++) {
        jpeg2texture(i, "grass.jpg");
    }

    texbuf_id = (GLuint*)calloc(num_textures, sizeof(GLuint));
    glGenBuffers(num_textures, texbuf_id);
    for (i = 0; i < num_textures; i++) {
        glBindBuffer(GL_ARRAY_BUFFER, texbuf_id[i]);
        if (glIsBuffer(texbuf_id[i]) != GL_TRUE) {
            fprintf(stderr, "ERROR: buffer texbuf_id[%d] not bound correctly. Aborting\n", i);
            exit(1);
        }
    }
    compute_tex_coords();

    return;
}

// TODO: implement this function
void fix_nulls() {
    return;
}

// data should be a SIZE * SIZE array
void read_from_file(char *fname, short *data) {
    #ifdef FUNC_NAMES2
    printf("read_from_file(%s, data)\n", fname);
    #endif

    int x, y;
    FILE *fin = fopen(fname, "r");
    // make sure the file exists
    if (fin == NULL) {
        fprintf(stderr, "Input file \"%s\" doesn't exist. Setting nulls\n", fname);
        for (y = 0; y < SIZE; y++) {
            for (x = 0; x < SIZE; x++) {
                data[y*SIZE + x] = 0;
            }
        }
        return;
    }

    // the input files have an extra row and column of overlap
    int size_plus = SIZE + 1;
    // array to read file into
    short *input = (short*)calloc(size_plus*size_plus, sizeof(short));
    check_for_null((void*)input, __FILE__, __LINE__);

    // read in the whole file
    #ifndef DEBUG_OLD
    fread(input, sizeof(short), size_plus * size_plus, fin);
    #else
    //#ifdef DEBUG_OLD
    long num = fread(input, sizeof(short), size_plus * size_plus, fin);
    printf("%ld elements read\n", num);
    #endif

    // convert from big endian to little endian
    // 0xABCD --> 0xCDBA
    for (y = 0; y < SIZE; y++) {
        for (x = 0; x < SIZE; x++) {
            short n = input[y*size_plus + x];
            short left = n << 8;
            short right = n >> 8;
            // since we're using signed shorts, we want logical shift, not arithmetic
            right &= 0x00FF;
            short result = left | right;

            // check for data voids
            if (result == DATA_VOID) result = 0;

            // invert the row order so the lower left coord is at the start
            data[SIZE*SIZE - (y+1)*SIZE + x] = result;
            #ifdef DEBUG_OLD
            // check that we swapped the bytes correctly
            if (x%10==0 && y%10==0) printf("before: %hx after: %hx\n", n, data[SIZE*SIZE - (y+1)*SIZE + x]);
            #endif
        }
    }
    // TODO: actually implement and call this function
    //fix_nulls();

    free(input);
    fclose(fin);
    return;
}

void latlon_from_filename(char *name, int *lat, int *lon) {
    int lat_mod, lon_mod;
    char *orig_name = name;

    // strip off the N/S
    while (*name != 'N' && *name != 'S') name++;

    // make sure file name is well-formed
    if (*name == 'N') lat_mod = 1;
    else if (*name == 'S') lat_mod = -1;
    else fprintf(stderr, "ERROR: Malformed filename: \"%s\"\n", orig_name);

    name++;

    // get the lat (x coord)
    *lat = lat_mod * atoi(name);

    // strip off the E/W
    while (*name != 'E' && *name != 'W') name++;

    // make sure file name is well-formed
    if (*name == 'E') lon_mod = 1;
    else if (*name == 'W') lon_mod = -1;
    else fprintf(stderr, "ERROR: Malformed filename: \"%s\"\n", orig_name);

    name++;

    // get the lon (y coord)
    *lon = lon_mod * atoi(name);

    return;
}

void jpeg2texture(int tex_num, char *imageName) {
    GLuint success = 0;

    success = ilutGLLoadImage(imageName);
    // check for error
    if (ilGetError() || !success) {
        printf("ERROR: jpeg2texture: %s\n", iluErrorString(ilGetError()));
        return;
    }

    // if the image was loaded successfully
    success = ilConvertImage(IL_RGB, IL_UNSIGNED_BYTE);
    if (!success) {
        #ifndef ERROR_NOTIFICATION_OFF
        fprintf(stderr, "Error converting image \"%s\".\n", imageName);
        #endif
        return;
    }
    glActiveTexture(GL_TEXTURE0);
    // bind this texture to a number
    glBindTexture(GL_TEXTURE_2D, textures[tex_num]);

    // TODO: see if these are the parameters we really want
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    // declare texture attributes
    glTexImage2D(GL_TEXTURE_2D, 0, ilGetInteger(IL_IMAGE_BPP), ilGetInteger(IL_IMAGE_WIDTH),
            ilGetInteger(IL_IMAGE_HEIGHT), 0, ilGetInteger(IL_IMAGE_FORMAT), GL_UNSIGNED_BYTE, ilGetData());

    return;
}

void initialize_clipmaps() {
    #ifdef FUNC_NAMES
    printf("initialize_clipmaps()...");
    #endif

    // TODO: Eventually, all vertex data will be stored in compressed form
    // in VRAM. For now, it's just in RAM.

    // array to hold clipmap vertex buffer ids
    cmap_vbuf_id = (GLuint*)calloc(num_cmaps, sizeof(GLuint));
    check_for_null((void*)cmap_vbuf_id, __FILE__, __LINE__);

    glGenBuffers(num_cmaps, cmap_vbuf_id);
    // may sure the vertex buffer ids are valid
    int i;
    for (i = 0; i < num_cmaps; i++) {
        glBindBuffer(GL_ARRAY_BUFFER, cmap_vbuf_id[i]);
        if (glIsBuffer(cmap_vbuf_id[i]) != GL_TRUE) {
            fprintf(stderr, "ERROR: buffer cmap_vbuf_id[%d] not bound correctly. Aborting\n", i);
            exit(1);
        }
    }

    // get the current position
    int lon = floor(eye.x);
    int lat = floor(eye.y);

    // array of to hold center location for all clipmap levels
    cmap_center = (vec3f*)calloc(num_cmaps, sizeof(vec3f));
    check_for_null((void*)cmap_center, __FILE__, __LINE__);

    // initialize the clipmap centers
    int level;
    for (level = 0; level < num_cmaps; level++) {
        if (level == 0) {
            cmap_center[0].x = lon;
            cmap_center[0].y = lat;
            continue;
        }
        // world distance between adjacent points in clipmap
        int scale = (int)pow(2, level - 1);
        // The clipmaps are nested alternately in the upper right and lower
        // left of the next coarser level to balance updating.
        int mod;
        if (level % 2 == 0) mod = 1;
        else mod = -1;

        cmap_center[level].x = cmap_center[level - 1].x + mod * scale;
        cmap_center[level].y = cmap_center[level - 1].y + mod * scale;
        // not used, but initialized for safety
        cmap_center[level].z = 0;
    }

    // Since clipmaps are updated toroidally, we need to keep track of the
    // clipmap index of the world space bottom left coord.
    cmap_start_index = (vec2i*)calloc(num_cmaps, sizeof(vec2i));
    check_for_null((void*)cmap_start_index, __FILE__, __LINE__);

    // initialize cmap_start_indices to 0s
    for (level = 0; level < num_cmaps; level++) {
        cmap_start_index[level].x = 0;
        cmap_start_index[level].y = 0;
    }

    /////////////////////
    // Vertex Buffer Data

    // need x,y,z,zc for each point in a level
    int data_size = POINTS_PER_COORD * cmap_size * cmap_size;
    float data[data_size];

    // compute the row length
    int Y_stride = SIZE * lon_range;

    // load initial data into the clipmaps
    for (level = 0; level < num_cmaps; level++) {
        // world distance between adjacent points in clipmap
        int scale = (int)pow(2, level);
        // distance from the center of the clipmap to the edge
        float radius = scale * 0.5 * (cmap_size - 1);

        vec3f level_offset = {0.0, 0.0, 0.0};
        // compute the current level's offset from viewpoint
        if (level != finest_level) {
            level_offset.x = 0.5*(cmap_center[level].x - cmap_center[level - 1].x);
            level_offset.y = 0.5*(cmap_center[level].y - cmap_center[level - 1].y);
        }

        int x, y;
        for (y = 0; y < cmap_size; y++) {
            for (x = 0; x < cmap_size; x++) {
                // compute the location in the main terrain_data grid
                // these are also the coords in world space
                int X = lon - radius + x*scale + level_offset.x;
                int Y = lat - radius + y*scale + level_offset.y;
                int data_index = POINTS_PER_COORD * (y*cmap_size + x);

                // set data to 0 if we're outside the domain boundary
                if (X <= 0.0 || Y <= 0.0 || X >= lon_range*SIZE || Y >= lat_range*SIZE) {
                    data[data_index] = X;
                    data[data_index + 1] = Y;
                    data[data_index + 2] = 0.0;
                    if (POINTS_PER_COORD == 4) data[data_index + 3] = 0.0;
                    continue;
                }

                #ifdef DEBUG_OLD
                printf("X = %d Y = %d data_index = %d\n", X, Y, data_index);
                #endif
                data[data_index] = (float)X;
                data[data_index + 1] = (float)Y;
                data[data_index + 2] = (float)(terrain_data[Y*Y_stride + X] * Z_SCALE);
                // DATA_VOID (for vim search)
                // TODO: get the data for the next coarser level
                if (POINTS_PER_COORD == 4) data[data_index + 3] = 0.0;
                //data[data_index + 3] = terrain_data[];
            }
        }
        // set the current buffer and copy the data to it
        glBindBuffer(GL_ARRAY_BUFFER, cmap_vbuf_id[level]);
        check_for_glerror(glGetError(), __FILE__, __LINE__);
        glBufferData(GL_ARRAY_BUFFER, data_size*sizeof(float), data, GL_DYNAMIC_DRAW);
        check_for_glerror(glGetError(), __FILE__, __LINE__);
    }
    #ifdef FUNC_NAMES
    printf("ic returned\n");
    #endif
    return;
}

// Sets index buffer sizes, ids, and sets up buffers to be used later
// NOTE: Index arrays are copied to the buffers without values being set
// to them because these buffers are recomputed per frame per level.
// We just need to reserve data for them at this point.
void initialize_index_buffers() {
    #ifdef FUNC_NAMES
    printf("initialize_index_buffers()...");
    #endif

    // array of index buffer ids
    index_id = (GLuint*)calloc(NUM_IBUFFERS, sizeof(GLuint));
    check_for_null((void*)index_id, __FILE__, __LINE__);

    glGenBuffers(NUM_IBUFFERS, index_id);
    // activate each index buffer
    int i;
    for (i = 0; i < NUM_IBUFFERS; i++) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_id[i]);
        // check for error
        if (glIsBuffer(index_id[i]) != GL_TRUE) {
            fprintf(stderr, "ERROR: buffer %u not bound. Aborting\n", index_id[i]);
            exit(1);
        }
    }

    // clipmap subregion parameter
    int m = (cmap_size + 1) / 4;
    index_size.m = m;

    // since we're initializing the index buffers for the first time, we have no offset
    vec2i offset = {0, 0};

    //////////////////////
    // Finest level square

    #ifdef USE_TEXTURES
    index_size.finest = 2*cmap_size * (cmap_size - 1);
    #else
    index_size.finest = cmap_size*cmap_size + (cmap_size - 1) * 2*(cmap_size - 1);
    #endif
    GLushort indices_F[index_size.finest];

    // copy the array into the buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_id[INDEX_FINEST]);
    check_for_glerror(glGetError(), __FILE__, __LINE__);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_size.finest*sizeof(GLushort), indices_F, GL_STATIC_DRAW);
    check_for_glerror(glGetError(), __FILE__, __LINE__);

    //////////////////////////
    // Top and Bottom sections

    #ifdef USE_TEXTURES
    index_size.top_bottom = 2*cmap_size * (m - 1);
    #else
    index_size.top_bottom = m*cmap_size + (m - 1) * 2*(cmap_size - 1);
    #endif
    GLushort indices_TB[index_size.top_bottom];

    // copy the array into the top buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_id[INDEX_TOP]);
    check_for_glerror(glGetError(), __FILE__, __LINE__);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_size.top_bottom*sizeof(GLushort), indices_TB, GL_STREAM_DRAW);
    check_for_glerror(glGetError(), __FILE__, __LINE__);

    // copy the array into the bottom buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_id[INDEX_BOTTOM]);
    check_for_glerror(glGetError(), __FILE__, __LINE__);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_size.top_bottom*sizeof(GLushort), indices_TB, GL_STREAM_DRAW);
    check_for_glerror(glGetError(), __FILE__, __LINE__);

    //////////////////////////
    // Left and Right sections

    // FIXME: this line below was added because m was getting clobbered
    m = index_size.m;
    #ifdef USE_TEXTURES
    index_size.left_right = 2*m * 2*m;
    #else
    index_size.left_right = m*(2*m + 1) + (2*m) * 2*(m - 1);
    #endif
    //printf("m = %d cmap_size = %d\n", m, cmap_size);
    GLushort indices_LR[index_size.left_right];

    // copy the array into the left buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_id[INDEX_LEFT]);
    check_for_glerror(glGetError(), __FILE__, __LINE__);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_size.left_right*sizeof(GLushort), indices_LR, GL_STREAM_DRAW);
    check_for_glerror(glGetError(), __FILE__, __LINE__);

    // copy the array into the right buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_id[INDEX_RIGHT]);
    check_for_glerror(glGetError(), __FILE__, __LINE__);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_size.left_right*sizeof(GLushort), indices_LR, GL_STREAM_DRAW);
    check_for_glerror(glGetError(), __FILE__, __LINE__);

    /////////////////
    // "L" Sub-region

    index_size.L_region = 2*m * 4 + 1;
    GLushort indices_L[index_size.L_region];

    // copy the array into the Lregion buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_id[INDEX_L_REGION]);
    check_for_glerror(glGetError(), __FILE__, __LINE__);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_size.L_region*sizeof(GLushort), indices_L, GL_STREAM_DRAW);
    check_for_glerror(glGetError(), __FILE__, __LINE__);

    // Set the index offset of the lower left corner of the 4 main clipmap subregions
    index_section_offset = (vec2i*)calloc(NUM_IBUFFERS, sizeof(vec2i));
    check_for_null((void*)index_section_offset, __FILE__, __LINE__);

    // top
    offset.x = 0;
    offset.y = cmap_size - index_size.m;
    index_section_offset[INDEX_TOP] = offset;

    // bottom
    offset.x = 0;
    offset.y = 0;
    index_section_offset[INDEX_BOTTOM] = offset;

    // left
    offset.x = 0;
    offset.y = index_size.m - 1;
    index_section_offset[INDEX_LEFT] = offset;

    // right
    offset.x = cmap_size - index_size.m;
    offset.y = index_size.m - 1;
    index_section_offset[INDEX_RIGHT] = offset;

    #ifdef FUNC_NAMES
    printf("iib returned\n");
    #endif
    return;
}

// per-frame, per-level computation of clipmap indices
void compute_indices(int level) {
    #ifdef FUNC_NAMES
    printf("compute_indices()...\n");
    #endif

    GLushort *indices;
    vec2i total_offset;

    vec2i level_offset = cmap_start_index[level];

    // finest level is drawn differently
    if (level == finest_level) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_id[INDEX_FINEST]);
        check_for_glerror(glGetError(), __FILE__, __LINE__);
        // get a pointer to data in VRAM
        indices = (GLushort*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_READ_WRITE);
        check_for_null((void*)indices, __FILE__, __LINE__);
        check_for_glerror(glGetError(), __FILE__, __LINE__);

        #ifdef USE_TEXTURES
        compute_texture_indices_rect(index_size.finest, cmap_size, indices, level_offset);
        #else
        compute_wireframe_indices_rect(index_size.finest, cmap_size, indices, level_offset);
        #endif

        if (!glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER)) check_for_glerror(glGetError(), __FILE__, __LINE__);

        #ifdef FUNC_NAMES
        printf("ci returned\n");
        #endif
        return;
    }

    // clipmap subregion parameter
    int m = index_size.m;

    //////////////////////////
    // Top and Bottom Sections

    // top
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_id[INDEX_TOP]);
    check_for_glerror(glGetError(), __FILE__, __LINE__);
    indices = (GLushort*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_READ_WRITE);
    check_for_null((void*)indices, __FILE__, __LINE__);
    total_offset = add2i(index_section_offset[INDEX_TOP], level_offset);

    #ifdef USE_TEXTURES
    compute_texture_indices_rect(index_size.top_bottom, cmap_size, indices, total_offset);
    #else
    compute_wireframe_indices_rect(index_size.top_bottom, cmap_size, indices, total_offset);
    #endif

    if (!glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER)) check_for_glerror(glGetError(), __FILE__, __LINE__);

    // bottom
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_id[INDEX_BOTTOM]);
    check_for_glerror(glGetError(), __FILE__, __LINE__);
    indices = (GLushort*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_READ_WRITE);
    check_for_null((void*)indices, __FILE__, __LINE__);
    total_offset = add2i(index_section_offset[INDEX_BOTTOM], level_offset);

    #ifdef USE_TEXTURES
    compute_texture_indices_rect(index_size.top_bottom, cmap_size, indices, total_offset);
    #else
    compute_wireframe_indices_rect(index_size.top_bottom, cmap_size, indices, total_offset);
    #endif

    if (!glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER)) check_for_glerror(glGetError(), __FILE__, __LINE__);

    //////////////////////////
    // Left and Right Sections

    // left
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_id[INDEX_LEFT]);
    check_for_glerror(glGetError(), __FILE__, __LINE__);
    indices = (GLushort*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_READ_WRITE);
    check_for_null((void*)indices, __FILE__, __LINE__);
    total_offset = add2i(index_section_offset[INDEX_LEFT], level_offset);

    #ifdef USE_TEXTURES
    compute_texture_indices_rect(index_size.left_right, m, indices, total_offset);
    #else
    compute_wireframe_indices_rect(index_size.left_right, m, indices, total_offset);
    #endif

    if (!glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER)) check_for_glerror(glGetError(), __FILE__, __LINE__);

    // right
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_id[INDEX_RIGHT]);
    check_for_glerror(glGetError(), __FILE__, __LINE__);
    indices = (GLushort*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_READ_WRITE);
    check_for_null((void*)indices, __FILE__, __LINE__);
    total_offset = add2i(index_section_offset[INDEX_RIGHT], level_offset);

    #ifdef USE_TEXTURES
    compute_texture_indices_rect(index_size.left_right, m, indices, total_offset);
    #else
    compute_wireframe_indices_rect(index_size.left_right, m, indices, total_offset);
    #endif

    if (!glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER)) check_for_glerror(glGetError(), __FILE__, __LINE__);

    ///////////////////
    // "L" Sub-region

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_id[INDEX_L_REGION]);
    check_for_glerror(glGetError(), __FILE__, __LINE__);
    indices = (GLushort*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_READ_WRITE);
    check_for_null((void*)indices, __FILE__, __LINE__);

    #ifdef USE_TEXTURES
    compute_texture_indices_Lregion(index_size.L_region, indices, level_offset, level);
    #else
    compute_wireframe_indices_Lregion(index_size.L_region, indices, level_offset, level);
    #endif

    if (!glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER)) check_for_glerror(glGetError(), __FILE__, __LINE__);

    ///////////////////////
    // Degenerate triangles

    // TODO: fill this in

    #ifdef FUNC_NAMES
    printf("ci returned\n");
    #endif
    return;
}

// this function computes wireframe indices for a rectangular region
void compute_wireframe_indices_rect(int num_indices, int row_len, GLushort *indices, vec2i offset) {
    #ifdef FUNC_NAMES
    printf("\tcompute_wireframe_indices_rect()...");
    #endif

    int ibuf_row_len = row_len + 2*(row_len - 1);
    int col_len = num_indices / ibuf_row_len + 1;

    int ibuf_index = 0;
    vec2i ibuf;
    for (ibuf.y = 0; ibuf.y < col_len; ibuf.y++) {
        for (ibuf.x = 0; ibuf.x < row_len; ibuf.x++) {
            int x = ibuf.x + offset.x;
            int y = ibuf.y + offset.y;

            // indexing should wrap around edges
            while (x >= cmap_size) x -= cmap_size;
            while (x < 0) x += cmap_size;
            while (y >= cmap_size) y -= cmap_size;
            while (y < 0) y += cmap_size;

            /*
            printf("loop index = (%d,%d)\n", ibuf.x, ibuf.y);
            printf("ibuf_index = %d\n", ibuf_index);
            printf("indices (unsigned long) = %lu\n", (unsigned long)indices);
            printf("indices (hex) = %lx\n", (unsigned long)indices);
            printf("The following line will try to assign to indices[%d]\n", ibuf_index);
            // */
            indices[ibuf_index] = y*cmap_size + x;

            #ifdef DEBUG_OLD
            printf("indices[%d] = %d\n",
                    ibuf_index, indices[ibuf_index]);
            #endif

            ibuf_index++;

            if ((ibuf.x == row_len - 1) && (ibuf.y < col_len - 1)) {
                int zx;
                for (zx = ibuf.x + offset.x; zx > offset.x; zx--) {
                    int foo = zx;
                    int bar = y + 1;

                    // indexing should wrap around edges
                    while (foo >= cmap_size) foo -= cmap_size;
                    while (foo < 0) foo += cmap_size;
                    while (bar >= cmap_size) bar -= cmap_size;
                    while (bar < 0) bar += cmap_size;

                    indices[ibuf_index] = bar*cmap_size + foo;

                    #ifdef DEBUG_OLD
                    printf("indices[%d] = %d\n",
                            ibuf_index, indices[ibuf_index]);
                    #endif

                    ibuf_index++;

                    foo--;
                    bar--;
                    // indexing should wrap around edges
                    while (foo >= cmap_size) foo -= cmap_size;
                    while (foo < 0) foo += cmap_size;
                    while (bar >= cmap_size) bar -= cmap_size;
                    while (bar < 0) bar += cmap_size;

                    indices[ibuf_index] = bar*cmap_size + foo;
                    #ifdef DEBUG_OLD
                    printf("indices[%d] = %d\n",
                            ibuf_index, indices[ibuf_index]);
                    #endif

                    ibuf_index++;
                }
            }
        }
    }

    #ifdef FUNC_NAMES
    printf("wris returned\n");
    #endif
    return;
}

// Compute indices for texturing. Zig zag from left to right on first row
// then right to left on the second row. Repeat.
void compute_texture_indices_rect(int num_indices, int num_cols, GLushort *indices, vec2i offset) {
    #ifdef FUNC_NAMES
    printf("compute_texture_indices_rect()...");
    #endif

    int num_rows = num_indices / num_cols;
    if (num_rows * num_cols != num_indices) unreachable(__FILE__, __LINE__);

    int x, y;
    for (y = 0; y < num_rows; y++) {
        for (x = 0; x < num_cols; x++) {
            // left to right
            if (y % 2 == 0) {
                indices[2*(y*num_cols + x)] = x;
                indices[2*(y*num_cols + x) + 1] = x + num_cols;
            }
            // right to left
            else {
                indices[2*(y*num_cols + x)] = num_cols - 1 - (x);
                indices[2*(y*num_cols + x) + 1] = num_cols - 1 - (x + num_cols);
            }
        }
    }

    #ifdef FUNC_NAMES
    printf("ctir returned\n");
    #endif
    return;
}

// TODO: remove this function
void compute_wireframe_indices_Lregion(int num_indices, GLushort *indices, vec2i offset, int level) {
    #ifdef FUNC_NAMES
    printf("compute_wireframe_indices_Lregion()...");
    #endif

    #ifdef FUNC_NAMES
    printf("cwiL returned\n");
    #endif
    return;
}

void compute_texture_indices_Lregion(int num_indices, GLushort *indices, vec2i offset, int level) {
    #ifdef FUNC_NAMES
    printf("compute_texture_indices_Lregion()...");
    #endif

    // TODO: fill this in
    int side_len =  (cmap_size + 1) / 2;

    int ibuf_index = 0;
    int i;
    for (i = 0; i < side_len; i++) {
        (void)ibuf_index;
    }

    #ifdef FUNC_NAMES
    printf("ctiL returned\n");
    #endif
    return;
}

void render_terrain_level(int level) {
    #ifdef FUNC_NAMES
    printf("render_terrain()...");
    #endif

    // turn off shaders
    glUseProgram(0);
    glDisable(GL_TEXTURE_2D);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_INDEX_ARRAY);
    #ifdef USE_TEXTURES
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    #endif

    // set the color for wireframe
    set_wireframe_color(level, NUM_COLORS);

    // vertex data
    glBindBuffer(GL_ARRAY_BUFFER, cmap_vbuf_id[level]);

    // finest level is drawn as single square
    if (level == finest_level) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_id[INDEX_FINEST]);
        glIndexPointer(GL_SHORT, 0, 0);
        glVertexPointer(3, GL_FLOAT, POINTS_PER_COORD*sizeof(GLfloat), BUFFER_OFFSET(0));
        glDrawElements(GL_LINE_STRIP, index_size.finest, GL_UNSIGNED_SHORT, 0);

        #ifdef FUNC_NAMES
        printf("rtl returned\n");
        #endif
        return;
    }

    // top
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_id[INDEX_TOP]);
    glIndexPointer(GL_SHORT, 0, 0);
    glVertexPointer(3, GL_FLOAT, POINTS_PER_COORD*sizeof(GLfloat), BUFFER_OFFSET(0));
    glDrawElements(GL_LINE_STRIP, index_size.top_bottom, GL_UNSIGNED_SHORT, 0);

    // bottom
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_id[INDEX_BOTTOM]);
    glIndexPointer(GL_SHORT, 0, 0);
    glVertexPointer(3, GL_FLOAT, POINTS_PER_COORD*sizeof(GLfloat), BUFFER_OFFSET(0));
    glDrawElements(GL_LINE_STRIP, index_size.top_bottom, GL_UNSIGNED_SHORT, 0);

    // left
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_id[INDEX_LEFT]);
    glIndexPointer(GL_SHORT, 0, 0);
    glVertexPointer(3, GL_FLOAT, POINTS_PER_COORD*sizeof(GLfloat), BUFFER_OFFSET(0));
    glDrawElements(GL_LINE_STRIP, index_size.left_right, GL_UNSIGNED_SHORT, 0);

    // right
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_id[INDEX_RIGHT]);
    glIndexPointer(GL_SHORT, 0, 0);
    glVertexPointer(3, GL_FLOAT, POINTS_PER_COORD*sizeof(GLfloat), BUFFER_OFFSET(0));
    glDrawElements(GL_LINE_STRIP, index_size.left_right, GL_UNSIGNED_SHORT, 0);

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_INDEX_ARRAY);
    #ifdef USE_TEXTURES
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    #endif

    // return to normal pointer operation
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    #ifdef FUNC_NAMES
    printf("rtl returned\n");
    #endif
    return;
}

// Check to see if the viewpoint has moved outside the inner bounding box for
// this level and if it has, shift the it until it contains the viewpoint.
// This function does what the determine_active_regions step in the paper does.
void update_clipmap_center(int level, int *rows_to_update, int *cols_to_update) {
    #ifdef FUNC_NAMES
    printf("update_clipmap_center()...");
    #endif
    // initialize counters for number of rows/cols to update
    *rows_to_update = 0;
    *cols_to_update = 0;

    // get the current position as an integer
    int lon = floor(eye.x);
    int lat = floor(eye.y);

    // the scale for a particular level just happens to be the distance
    // from the center of the clipmap to the edge of the center box
    int scale = (int)pow(2, level);

    // if the viewpoint has moved outside the center box, shift the box until it
    // contains the viewpoint, keeping track of how many rows/cols to update
    while (lon >= cmap_center[level].x + scale) {
        cmap_center[level].x += 2 * scale;
        *cols_to_update += 2;
        cmap_start_index[level].x += 2;
    }
    while (lat >= cmap_center[level].y + scale) {
        cmap_center[level].y += 2 * scale;
        *rows_to_update += 2;
        cmap_start_index[level].y += 2;
    }
    while (lon < cmap_center[level].x - scale) {
        cmap_center[level].x -= 2 * scale;
        *cols_to_update -= 2;
        cmap_start_index[level].x -= 2;
    }
    while (lat < cmap_center[level].y - scale) {
        cmap_center[level].y -= 2 * scale;
        *rows_to_update -= 2;
        cmap_start_index[level].y -= 2;
    }

    // index offset should wrap around edges
    while (cmap_start_index[level].x >= cmap_size) cmap_start_index[level].x -= cmap_size;
    while (cmap_start_index[level].x < 0) cmap_start_index[level].x += cmap_size;
    while (cmap_start_index[level].y >= cmap_size) cmap_start_index[level].y -= cmap_size;
    while (cmap_start_index[level].y < 0) cmap_start_index[level].y += cmap_size;

    #ifdef DEBUG_OLD
    if (*rows_to_update != 0 || *cols_to_update != 0)
        printf("ucc(): rtu = %d ctu = %d\n", *rows_to_update, *cols_to_update);
    #endif

    #ifdef FUNC_NAMES
    printf("ucc returned\n");
    #endif
    return;
}

void update_clipmap_level(int level, int rows_to_update, int cols_to_update) {
    #ifdef FUNC_NAMES
    printf("update_clipmap_level()...");
    #endif

    // return immediately if there's nothing to update
    if (rows_to_update == 0 && cols_to_update == 0) {
        #ifdef FUNC_NAMES
        printf("ucl returned\n");
        #endif
        return;
    }

    // get a pointer to the clipmap vertex data in VRAM
    glBindBuffer(GL_ARRAY_BUFFER, cmap_vbuf_id[level]);
    float *cmap_data = (float*)glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE);
    check_for_null((void*)cmap_data, __FILE__, __LINE__);
    check_for_glerror(glGetError(), __FILE__, __LINE__);

    vec2i size, max;
    // get the absolute value of the number of rows/cols to update
    if (cols_to_update > 0) size.x = cols_to_update;
    else size.x = -cols_to_update;
    if (rows_to_update > 0) size.y = rows_to_update;
    else size.y = -rows_to_update;

    // negative size is never possible
    if (size.x < 0 || size.y < 0) {
        fprintf(stderr, "ERROR: %s:%d size.x or size.y is negative. Aborting\n", __FILE__, __LINE__);
        exit(1);
    }

    int scale = (int)pow(2, level);
    // distance from center to outer edge
    int radius = (int)(scale * 0.5 * (cmap_size - 1));

    vec2i newc, oldc;
    newc.x = cmap_center[level].x;
    newc.y = cmap_center[level].y;
    oldc.x = newc.x - scale*cols_to_update;
    oldc.y = newc.y - scale*rows_to_update;

    vec2i world_start, cmap_start;

    /* I added a kind of hackish fix to the code below. This only applies to the
       lines that assign to world_start.x/y and have a "+ radius" in the line.
       The hackish fix was to add "+ scale" to these lines to fix the updating
       problems. These lines are all marked by TODO: verify because I don't
       completely understand why this fix works.  */

    ////////////////
    // Center Square

    // set the lower left corner in world and local coords
    if (newc.x > oldc.x) {
        // TODO: verify
        world_start.x = oldc.x + radius + scale;
        cmap_start.x = cmap_start_index[level].x - size.x;
    }
    else {
        world_start.x = newc.x - radius;
        cmap_start.x = cmap_start_index[level].x;
    }
    if (newc.y > oldc.y) {
        // TODO: verify
        world_start.y = oldc.y + radius + scale;
        cmap_start.y = cmap_start_index[level].y - size.y;
    }
    else {
        world_start.y = newc.y - radius;
        cmap_start.y = cmap_start_index[level].y;
    }

    max.x = size.x;
    max.y = size.y;
    // update the center region
    update_clipmap_subregion(level, cmap_data, world_start, cmap_start, max);

    /////////////////
    // Vertical Section

    // set the lower left corner in world and local coords
    if (newc.x > oldc.x) {
        // TODO: verify
        world_start.x = oldc.x + radius + scale;
        cmap_start.x = cmap_start_index[level].x - size.x;
    }
    else {
        world_start.x = newc.x - radius;
        cmap_start.x = cmap_start_index[level].x;
    }
    if (newc.y > oldc.y) {
        world_start.y = newc.y - radius;
        cmap_start.y = cmap_start_index[level].y;
    }
    else {
        world_start.y = oldc.y - radius;
        cmap_start.y = cmap_start_index[level].y + size.y;
    }

    max.x = size.x;
    max.y = cmap_size - size.y;
    // update the vertical section
    update_clipmap_subregion(level, cmap_data, world_start, cmap_start, max);

    ///////////////////
    // Horizontal Section

    if (newc.x > oldc.x) {
        world_start.x = newc.x - radius;
        cmap_start.x = cmap_start_index[level].x;
    }
    else {
        world_start.x = oldc.x - radius;
        cmap_start.x = cmap_start_index[level].x + size.x;
    }
    if (newc.y > oldc.y) {
        // TODO: verify
        world_start.y = oldc.y + radius + scale;
        cmap_start.y = cmap_start_index[level].y - size.y;
    }
    else {
        world_start.y = newc.y - radius;
        cmap_start.y = cmap_start_index[level].y;
    }

    max.x = cmap_size - size.x;
    max.y = size.y;
    // update the horizontal section
    update_clipmap_subregion(level, cmap_data, world_start, cmap_start, max);

    // unmap the buffer so we can use it again later
    GLenum err = glUnmapBuffer(GL_ARRAY_BUFFER);
    if (err == GL_FALSE) {
        printf("ERROR: problem unmapping buffer at %d\n", __LINE__);
    }

    #ifdef FUNC_NAMES
    printf("ucl returned\n");
    #endif
    return;
}

// updates a rectangular section of the clipmap
void update_clipmap_subregion(int level, float *cmap_data, vec2i world_start,
        vec2i cmap_start, vec2i max) {
    #ifdef DEBUG_OLD
    printf("UCS(level=%d ws(%d,%d) cmps(%d,%d) max(%d,%d)\n", level,
            world_start.x, world_start.y, cmap_start.x, cmap_start.y, max.x, max.y);
    #endif
    int x, y;
    int scale = (int)pow(2, level);

    for (y = 0; y < max.y; y++) {
        for (x = 0; x < max.x; x++) {
            // compute current local and world coords
            vec2i cmap = {cmap_start.x + x, cmap_start.y + y};
            vec2i world = {world_start.x + scale*x, world_start.y + scale*y};

            // wrap local coords around clipmap edges
            while (cmap.x >= cmap_size) cmap.x -= cmap_size;
            while (cmap.x < 0) cmap.x += cmap_size;
            while (cmap.y >= cmap_size) cmap.y -= cmap_size;
            while (cmap.y < 0) cmap.y += cmap_size;

            int cmap_index = POINTS_PER_COORD * (cmap.y*cmap_size + cmap.x);
            int terrain_index = world.y*lon_range*SIZE + world.x;
            /*
            printf("local(%d,%d) world(%d,%d)\n",
                    (int)cmap_start.x, (int)cmap_start.y, (int)world_start.x, (int)world_start.y);
            printf("doff = %d toff = %d\n", cmap_index, terrain_offset);
            printf("x = %d y = %d\n", x, y);
            printf("1\n");
            // */

            cmap_data[cmap_index] = world.x;
            cmap_data[cmap_index + 1] = world.y;

            // make sure the terrain index is valid
            if (is_terrain_index_valid(world)) {
                cmap_data[cmap_index + 2] = (float)(terrain_data[terrain_index] * Z_SCALE);
            }
            // set to zero if outside terrain boundaries
            else {
                cmap_data[cmap_index + 2] = 0.0;
            }

            // Zc
            // TODO: change this to the real Zc
            cmap_data[cmap_index + 3] = 0.0;
        }
    }

    return;
}

bool is_terrain_index_valid(vec2i world) {
    bool ret = true;

    // return false if the index is out of bounds
    if (world.x < 0) ret = false;
    if (world.x > lon_range*SIZE) ret = false;
    if (world.y < 0) ret = false;
    if (world.y > lat_range*SIZE) ret = false;

    return ret;
}

// Don't draw levels that we are too high to see.
// NOTE: In the GPU version, they don't crop. Instead, levels that are not
// updated completely are not drawn at all. This is the method I'm using.
void crop_active_regions() {
    int height = (int)eye.z - 100;

    int new_finest = 0;
    while (height > 100) {
        // negative numbers skip this loop, arithmetic shift is fine
        height >>= 1; // height /= 2;
        new_finest++;
    }
    finest_level = new_finest;

    return;
}

inline void verify_cmap_centers() {
    #ifdef FUNC_NAMES
    printf("verify_cmap_centers()...");
    #endif

    bool error = false;

    int level;
    for (level = finest_level; level < num_cmaps; level++) {
        if (level == finest_level) continue;

        int scale = (int)pow(2, level - 1);
        // if this clipmap is not in a valid configuration, set error
        if (abs(cmap_center[level].x - cmap_center[level - 1].x) != scale) error = true;
        if (abs(cmap_center[level].y - cmap_center[level - 1].y) != scale) error = true;
    }
    if (error) {
        fprintf(stderr, "ERROR: clipmap centers in invalid configuration. Aborting\n");
        exit(1);
    }

    #ifdef FUNC_NAMES
    printf("vcc returned\n");
    #endif
    return;
}

void check_for_invalid_data() {
    int i;
    int min = -10;
    int count = 0;

    for (i = 0; i < terrain_size; i++) {
        short d = terrain_data[i];
        if (d < min) {
            if (d != DATA_VOID) printf("under %d: %hd\n", min, d);
            count++;
        }
    }

    printf("%d total under min(%d)\n", count, min);
    return;
}

inline void draw_bitmap_string(float x, float y, float z, void *font, char *text) {
    char *c;
    glColor3ub(255, 255, 255);
    glRasterPos3f(x, y, z);
    for (c = text; *c != '\0'; c++) {
        glutBitmapCharacter(font, *c);
    }
    return;
}

// sets color for clipmap levels for debugging
inline void set_wireframe_color(int level, int total) {
    int result = level % total;

    glColor3ub(255, 255, 255);
    if (result == 0) glColor3ub(220, 0, 0);
    else if (result == 1) glColor3ub(0, 220, 0);
    else if (result == 2) glColor3ub(0, 0, 220);
    else if (result == 3) glColor3ub(220, 220, 0);
    else if (result == 4) glColor3ub(90, 0, 220);
    else if (result == 5) glColor3ub(220, 90, 0);
    else if (result == 6) glColor3ub(0, 220, 220);
    else if (result == 7) glColor3ub(220, 220, 220);
    else {
        printf("Add more colors to set_wireframe_color!\n");
        glColor3ub(220, 220, 220);
    }

    return;
}

inline int verify_power_of_two(unsigned int x) {
    // from GNU C lib, malloc.c
    return ((x != 0) && !(x & (x - 1)));
}
