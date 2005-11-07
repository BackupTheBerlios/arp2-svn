#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <sys/time.h>
#include <GL/glut.h>

GLuint tex;
uint32_t* buf;

size_t width = 800, height = 600;

#define FORMAT 1

#if FORMAT == 0
GLenum iformat = GL_RGBA8;
GLenum format  = GL_BGRA;
GLenum type    = GL_UNSIGNED_INT_8_8_8_8_REV;
#elif FORMAT == 1
GLenum iformat = GL_RGBA8;
GLenum format  = GL_BGRA;
GLenum type    = GL_UNSIGNED_BYTE;
#elif FORMAT == 2
GLenum iformat = GL_RGBA8;
GLenum format  = GL_RGBA;
GLenum type    = GL_UNSIGNED_BYTE;
#elif FORMAT == 3
GLenum iformat = GL_RGBA4;
GLenum format  = GL_BGRA;
GLenum type    = GL_UNSIGNED_SHORT_4_4_4_4;
#endif

void display(void) {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tex);
  glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0,
		  0, 0, width, height,
		  format, type,
		  buf);
  size_t x;

  for (x = 0; x < width*height; ++x) {
    buf[x] = 0;
  }

  glGetTexImage(GL_TEXTURE_RECTANGLE_ARB, 0,
		format, type,
		buf);
  
  glEnable(GL_COLOR_LOGIC_OP);
  glLogicOp(GL_COPY_INVERTED);

  glBegin(GL_QUADS); {
    glActiveTexture(GL_TEXTURE0);

    glVertex2f(    0.0f,   0.0f);
    glTexCoord2f(  0.0f,   0.0f);
    glVertex2f(  128.0f,   0.0f);
    glTexCoord2f( width,   0.0f);
    glVertex2f(  128.0f, 128.0f);
    glTexCoord2f( width, height);
    glVertex2f(    0.0f, 128.0f);
    glTexCoord2f(  0.0f, height);
  }
  glEnd();

  glRasterPos2i(100,100);
  glCopyPixels(50,50,50,50,GL_COLOR);

  glDisable(GL_COLOR_LOGIC_OP);
  
  glutSwapBuffers();

  static int cnt = 0;
  static struct timeval last;
  static struct timeval now;

  if (++cnt % 50 == 0) {
/*     for (x = 0; x < 100; ++x) { */
/*       printf("%08x ", buf[x]); */
/*     } */

/*     printf("\n"); */

    last = now;
    gettimeofday(&now, NULL);

    double secs = (now.tv_sec + now.tv_usec * 1e-6)-(last.tv_sec + last.tv_usec * 1e-6);

    secs /= cnt;
    cnt = 0;
    
    printf("%zdx%zdx%zd textels *2 in %f milliseconds; %f Mtextels/s; %f MiB/s\n",
	   width, height, sizeof *buf, secs * 1e3,
	   2 * width * height / secs / 1e6,
	   2 * width * height * (sizeof *buf) / secs / 1024 / 1024);
  }
}

void reshape(int width, int height) {
  glViewport(0, 0, width, height);
  
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0, width, 0, height);
  glMatrixMode(GL_MODELVIEW);}

void idle(void) {
  glutPostRedisplay();
}

int main(int argc, char** argv) {
  glutInit(&argc, argv);

  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
  glutInitWindowSize(640, 480);

  glutCreateWindow("GLUT Program");
    
  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutIdleFunc(idle);

  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tex);
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0,
	       iformat,
	       width, height, 0,
	       format, type,
	       NULL);
  buf = calloc(width * height, sizeof *buf);

  size_t x, y;
  for (y = 0; y < height; ++y) {
    uint32_t* line = buf + width * y;

    for (x = 0; x < width; ++x) {
      line[x] = (0xff * x / width) | ((0xff00 * y / height) & 0xff00);
    }
  }
  
  glEnable(GL_TEXTURE_RECTANGLE_ARB);

  glutMainLoop();
  return EXIT_SUCCESS;
}
