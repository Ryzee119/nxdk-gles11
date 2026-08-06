#define _WINGDI_  // Guard against WGL headers
#include <GL/gl.h> // For gl4es desktop GL
#include <hal/video.h>

void glContextInit(GLint window_width, GLint window_height);
void glFlipNV2A(void);

int main(void)
{
    XVideoSetMode(640, 480, 32, REFRESH_DEFAULT);
    glContextInit(640, 480);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBegin(GL_TRIANGLES);
        glColor3f(1.0f, 0.0f, 0.0f);
        glVertex3f(0.0f, 0.5f, 0.0f);
        glColor3f(0.0f, 1.0f, 0.0f);
        glVertex3f(-0.5f, -0.5f, 0.0f);
        glColor3f(0.0f, 0.0f, 1.0f);
        glVertex3f(0.5f, -0.5f, 0.0f);
    glEnd();

    glFlipNV2A();
    return 0;
}
