#ifndef GL_PLATFORM_H
#define GL_PLATFORM_H

/* No Windows, near e far são macros. Por isso chamamos de znear e zfar. */

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include <GL/freeglut.h>

#ifndef GLUT_MULTISAMPLE
#define GLUT_MULTISAMPLE 128
#endif

#endif
