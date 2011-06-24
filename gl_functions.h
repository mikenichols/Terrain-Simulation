#ifndef GL_FUNCTIONS_H
#define GL_FUNCTIONS_H

void redraw();
void reshape(int w, int h);
void animate();
void mouse(int button, int state, int x, int y);
void motion(int x, int y);
void key(unsigned char key, int x, int y);
void specialKey(int key, int x, int y);

#endif // GL_FUNCTIONS_H
