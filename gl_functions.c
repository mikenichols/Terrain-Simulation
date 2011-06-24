#define GL_GLEXT_PROTOTYPES
#include <GL/glut.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "gl_functions.h"
#include "globals.h"
#include "typedefs.h"
#include "movement.h"
#include "functions.h"

#define FPS_ON

#ifndef DEBUG
#define DEBUG
#endif

void redraw() {
	#ifdef FPS_ON
    // TODO: change the fps code to take the average over ~60 frames because it's just too irratic now
	double elapsed_time;
	clock_t start_time, end_time;
	// start timing
	start_time = clock();
	#endif

	glEnable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // light blue "sky"
	glClearColor(0.0, 0.6, 1.0, 0.0);

	#ifdef DEBUG2
	// get the current location of the viewer as integer coords
	int lon = floor(eye.x);
	int lat = floor(eye.y);
	printf("eye is at (%.2f, %.2f, %.2f)\n", eye.x, eye.y, eye.z);
	printf("lower left corner of current tile is at (%d, %d)\n", lon, lat);
	#endif

	// plane showing extent of data files (world)
    #ifdef DEBUG
    float ground_z = -50.0;
	vec3f a = {0.0, 0.0, ground_z};
	vec3f b = {0.0, SIZE*lat_range, ground_z};
	vec3f c = {SIZE*lon_range, SIZE*lat_range, ground_z};
	vec3f d = {SIZE*lon_range, 0.0, ground_z};

	glColor3ub(0, 1, 5);
	glBegin(GL_QUADS);
		glVertex3f(a.x, a.y, a.z);
		glVertex3f(b.x, b.y, b.z);
		glVertex3f(c.x, c.y, c.z);
		glVertex3f(d.x, d.y, d.z);
	glEnd();
    #endif

	/* grid on plane showing seperation of data files
	glColor3ub(255, 255, 0);
	for (m = 0; m < SIZE*lat_range; m += SIZE) {
		glBegin(GL_LINES);
		glVertex3f(0.0, m, 0.0);
		glVertex3f(SIZE*lon_range, m, 0.0);
		glEnd();
	}
	for (m = 0; m < SIZE*lon_range; m += SIZE) {
		glBegin(GL_LINES);
		glVertex3f(m, 0.0, 0.0);
		glVertex3f(m, SIZE*lat_range, 0.0);
		glEnd();
	}
	// */

	/* dots on the grid close to viewer
	int dot_size = 200;
	glColor3ub(255, 255, 255);
	for (i = 0; i < dot_size; i++) {
		for (j = 0; j < dot_size; j++) {
			glBegin(GL_POINTS);
			glVertex3i(lon - dot_size/2 + i, lat - dot_size/2 + j, 0.0);
			glEnd();
		}
	}
	// */

    #ifdef DEBUG_OLD
    //printf("viewer(%f,%f,%f)\n", floor(eye.x), floor(eye.y), floor(eye.z));
    printf("level 0 center(%f,%f)\n", cmap_center[0].x, cmap_center[0].y);
    #endif

    // Render the levels in coarse to fine order to take advantage of OpenGL's
    // built in occlusion detection.
    int level;
    for (level = num_cmaps - 1; level >= finest_level; level--) {
        int rows_to_update, cols_to_update;

        // update the geometry clipmap
        update_clipmap_center(level, &rows_to_update, &cols_to_update);
        update_clipmap_level(level, rows_to_update, cols_to_update);

        // crop the active regions to the clip regions
        crop_active_regions();

        compute_indices(level);

		// draw the terrain for this level
		render_terrain_level(level);

		/* draw large white points over the data for debugging purposes
		#ifdef DEBUG_OLD
		int i;
		glBindBuffer(GL_ARRAY_BUFFER, cmap_vbuf_id[level]);
		int *bard = (int*)glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE);
		if (level == 0) glColor3ub(255, 200, 200);
        else if (level == 1) glColor3ub(200, 255, 200);
        else if (level == 1) glColor3ub(200, 200, 255);
        else glColor3ub(255, 255, 255);
		glPointSize(3.0);

		for (i = 0; i < cmap_size*cmap_size; i++) {
			glBegin(GL_POINTS);
				//printf("i = %d xyz(%d, %d, %d) ", i, bard[3*i], bard[3*i + 1], bard[3*i + 2] + 1);
				glVertex3f(bard[3*i], bard[3*i + 1], bard[3*i + 2] + 0.5);
			glEnd();
			//if (i % 7 == 0) printf("\n");
		}

		glPointSize(1.0);
		glUnmapBuffer(GL_ARRAY_BUFFER);
		#endif
        // */
	}
    #ifdef DEBUG
    verify_cmap_centers();
    #endif

	/* draw the active region boundaries
	for (level = 0; level < num_cmaps; level++) {
		// compute the scale for this level
		int scale = (int)pow(2, level);
		// compute the distance, in world coords, from the center to the outer edge
		int radius = scale * ((cmap_size - 1) / 2);
		// alternate colors
		//set_wireframe_color(level, NUM_COLORS);
		glColor3ub(255, 255, 255);

        int z = 0;
		glBegin(GL_LINE_LOOP);
			glVertex3i(cmap_center[level].x - radius, cmap_center[level].y - radius, z);
			glVertex3i(cmap_center[level].x + radius, cmap_center[level].y - radius, z);
			glVertex3i(cmap_center[level].x + radius, cmap_center[level].y + radius, z);
			glVertex3i(cmap_center[level].x - radius, cmap_center[level].y + radius, z);
		glEnd();
	}
	// */

	#ifdef FPS_ON
	// stop timing
	end_time = clock();
	elapsed_time = (double)(end_time - start_time) / (double)CLOCKS_PER_SEC;
	// add 10ms for FPS computation time
	elapsed_time += 0.01;

    // subtract the oldest elapsed time from the total, set the newest elapsed
    // time and add it to the total
    fps_total -= fps_array[fps_count];
    fps_array[fps_count] = 1.0 / elapsed_time;
    fps_total += fps_array[fps_count++];
    if (fps_count > fps_size) fps_count = 0;

	char fps_text[20];
	snprintf(fps_text, 19, "%.2f FPS", fps_total / fps_size);
	// compute the cross product
	vec3f right = cross3f(look, up);
	normalize3f(&right);

	glColor3ub(255, 255, 255);
    // TODO: line 1253 of ingest.cpp has some better code for drawing text
	// there was an error in the location for this
	draw_bitmap_string(eye.x + look.x + .5*right.x,
					   eye.y + look.y + .5*right.y,
					   eye.z - 0.4, (char*)BIG_FONT, fps_text);
	#endif

	glutSwapBuffers();
	glutPostRedisplay();
    check_for_glerror(glGetError(), __FILE__, __LINE__);
	return;
}

void reshape(int w, int h) {
	// prevent a divide by zero error
	if (h == 0) h = 1;

	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(FOVY, (float)w / (float)h, 0.01, 40000.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(eye.x, eye.y, eye.z,
			  eye.x + look.x, eye.y + look.y, eye.z + look.z,
			  up.x, up.y, up.z);

	screen_width = w;
	screen_height = h;

	return;
}

void animate() {
	return;
}

void mouse(int button, int state, int x, int y) {
	return;
}

void motion(int x, int y) {
	return;
}

void key(unsigned char key, int x, int y) {
	// escape exits the program
	if (key == 27) exit(0);

	switch (key) {
		case 'w':
			// move forward
			move(DIR_FORWARD, look);
			break;
		case 's':
			// move backward
			move(DIR_BACKWARD, look);
			break;
		case 'a':
			// turn left
			rotate(DIR_LEFT, up);
			break;
		case 'd':
			// turn right
			rotate(DIR_RIGHT, up);
			break;
		// numpad 8
		case 56:
			// pitch down
			rotate(DIR_DOWN, cross3f(up, look));
			break;
		// numpad 5
		case 53:
			// pitch up
			rotate(DIR_UP, cross3f(up, look));
			break;
		// numpad 4
		case 52:
			// roll left
			rotate(DIR_LEFT, look);
			break;
		// numpad 6
		case 54:
			// roll right
			rotate(DIR_RIGHT, look);
			break;
		default:
			printf("key = %d\n", (int)key);
	}

	reshape(screen_width, screen_height);
	glutPostRedisplay();
	return;
}

void specialKey(int key, int x, int y) {
	switch (key) {
		case GLUT_KEY_UP:
			// move up
			move(DIR_UP, up);
			break;
		case GLUT_KEY_DOWN:
			// move down
			move(DIR_DOWN, up);
			break;
		case GLUT_KEY_LEFT:
			// strafe left
			move(DIR_LEFT, cross3f(up, look));
			break;
		case GLUT_KEY_RIGHT:
			// strafe right
			move(DIR_RIGHT, cross3f(up, look));
			break;
		// no default needed
	}

	reshape(screen_width, screen_height);
	glutPostRedisplay();
	return;
}
