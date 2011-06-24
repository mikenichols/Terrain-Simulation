#define GL_GLEXT_PROTOTYPES
#define GLX_GLEXT_PROTOTYPES
#include <GL/glx.h>
#include <GL/glut.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "globals.h"
#include "shaders.h"

#ifndef DEBUG
#define DEBUG
#endif

void load_shader(char *file_name) {
    const char *vshader =
        "attribute vec4 gl_Vertex;"\
        "void main() {"\
        "    gl_TexCoord[0] = gl_MultiTexCoord0;"\
        "    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;"\
        "}";

    const char *fshader =
        "uniform sampler2D tex;"\
        "void main () {"\
        "    vec4 color = texture2D(tex, gl_TexCoord[0].st);"\
        "    gl_FragColor = color;"\
        "}";

    int vslen = strlen(vshader);
    int fslen = strlen(fshader);

    // vertex shader
    GLuint vsid = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vsid, 1, &vshader, &vslen);
    glCompileShader(vsid);

    // fragment shader
    GLuint fsid = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fsid, 1, &fshader, &fslen);
    glCompileShader(fsid);

    // create shader program and link
    shader_prog_id = glCreateProgram();
    glAttachShader(shader_prog_id, vsid);
    glAttachShader(shader_prog_id, fsid);
    glLinkProgram(shader_prog_id);
    glUseProgram(shader_prog_id);

    /* setup variable to be used in shader
    GLint loc = glGetUniformLocation(shader_prog_id, "foo");
    int foo = 5;
    glUniform1i(loc, foo);
    // */

    return;
}

void compute_tex_coords() {
    int tc_size = 2*cmap_size*cmap_size;
    tex_coords = (GLfloat*)calloc(tc_size, sizeof(GLfloat));
    check_for_null((void*)tex_coords, __FILE__, __LINE__);

    int x, y;
    for (y = 0; y < cmap_size; y++) {
        for (x = 0; x < cmap_size; x++) {
            int index = 2*(y*cmap_size + x);
            #ifdef DEBUG_OLD
            printf("xy(%d, %d) index = %d size = %d\n", x, y, index, tc_size);
            #endif
            check_array_bounds(index, tc_size, __FILE__, __LINE__);
            tex_coords[index]= (float)x / cmap_size;
            check_array_bounds(index + 1, tc_size, __FILE__, __LINE__);
            tex_coords[index + 1] = (float)y / cmap_size;
        }
    }
    return;
}
