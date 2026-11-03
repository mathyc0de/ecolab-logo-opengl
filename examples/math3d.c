#include "gl_platform.h"

#include "math3d.h"

#include <math.h>
#include <stdio.h>

void mat4_identity(float m[16])
{
    int i;

    for (i = 0; i < 16; i++) {
        m[i] = 0.f;
    }
    m[0] = m[5] = m[10] = m[15] = 1.f;
}

void mat4_multiply(float out[16], const float a[16], const float b[16])
{
    float r[16];
    int col;
    int row;

    /* Multiplicamos a*b com a coluna à direita, como a OpenGL faz. */
    for (col = 0; col < 4; col++) {
        for (row = 0; row < 4; row++) {
            r[col * 4 + row] =
                a[0 * 4 + row] * b[col * 4 + 0] +
                a[1 * 4 + row] * b[col * 4 + 1] +
                a[2 * 4 + row] * b[col * 4 + 2] +
                a[3 * 4 + row] * b[col * 4 + 3];
        }
    }

    for (row = 0; row < 16; row++) {
        out[row] = r[row];
    }
}

void mat4_make_translation(float m[16], float tx, float ty, float tz)
{
    mat4_identity(m);
    m[12] = tx;
    m[13] = ty;
    m[14] = tz;
}

void mat4_make_scale(float m[16], float sx, float sy, float sz)
{
    mat4_identity(m);
    m[0] = sx;
    m[5] = sy;
    m[10] = sz;
}

void mat4_make_rotation_x(float m[16], float radians)
{
    float c = cosf(radians);
    float s = sinf(radians);

    mat4_identity(m);
    m[5] = c;
    m[6] = s;
    m[9] = -s;
    m[10] = c;
}

void mat4_make_rotation_y(float m[16], float radians)
{
    float c = cosf(radians);
    float s = sinf(radians);

    mat4_identity(m);
    m[0] = c;
    m[2] = -s;
    m[8] = s;
    m[10] = c;
}

void mat4_make_rotation_z(float m[16], float radians)
{
    float c = cosf(radians);
    float s = sinf(radians);

    mat4_identity(m);
    m[0] = c;
    m[1] = s;
    m[4] = -s;
    m[5] = c;
}

void mat4_make_perspective(float m[16], float fovy_rad, float aspect,
                           float znear, float zfar)
{
    float f = 1.f / tanf(fovy_rad * 0.5f);
    int i;

    for (i = 0; i < 16; i++) {
        m[i] = 0.f;
    }

    /* Montamos a perspectiva na mão. m[11]=-1 manda -z para w, aí a GPU divide. */
    m[0] = f / aspect;
    m[5] = f;
    m[10] = (zfar + znear) / (znear - zfar);
    m[11] = -1.f;
    m[14] = (2.f * zfar * znear) / (znear - zfar);
}

void mat4_make_ortho(float m[16], float left, float right, float bottom,
                     float top, float znear, float zfar)
{
    int i;

    for (i = 0; i < 16; i++) {
        m[i] = 0.f;
    }

    m[0] = 2.f / (right - left);
    m[5] = 2.f / (top - bottom);
    m[10] = -2.f / (zfar - znear);
    m[12] = -(right + left) / (right - left);
    m[13] = -(top + bottom) / (top - bottom);
    m[14] = -(zfar + znear) / (zfar - znear);
    m[15] = 1.f;
}

void mat4_transform_point(const float m[16], float x, float y, float z,
                          float *ox, float *oy, float *oz)
{
    *ox = m[0] * x + m[4] * y + m[8] * z + m[12];
    *oy = m[1] * x + m[5] * y + m[9] * z + m[13];
    *oz = m[2] * x + m[6] * y + m[10] * z + m[14];
}

void mat4_print(const char *name, const float m[16])
{
    int row;
    int col;

    printf("%s\n", name);
    for (row = 0; row < 4; row++) {
        printf("  ");
        for (col = 0; col < 4; col++) {
            printf("%9.3f", m[col * 4 + row]);
        }
        printf("\n");
    }
}

void apply_matrix(const float m[16])
{
    glMultMatrixf(m); /* é por aqui que a matriz nossa entra na OpenGL */
}
