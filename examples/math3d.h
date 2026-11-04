#ifndef MATH3D_H
#define MATH3D_H

/*
 * Matrizes 4x4 nossas, iguais às da OpenGL (coluna na memória).
 * Linha r, coluna c: m[c*4+r]. apply_matrix manda isso no glMultMatrixf.
 */

#define MATH3D_PI 3.14159265358979323846f

void mat4_identity(float m[16]);
void mat4_multiply(float out[16], const float a[16], const float b[16]);

void mat4_make_translation(float m[16], float tx, float ty, float tz);
void mat4_make_scale(float m[16], float sx, float sy, float sz);
void mat4_make_rotation_x(float m[16], float radians);
void mat4_make_rotation_y(float m[16], float radians);
void mat4_make_rotation_z(float m[16], float radians);

void mat4_make_perspective(float m[16], float fovy_rad, float aspect,
                           float znear, float zfar);
void mat4_make_ortho(float m[16], float left, float right, float bottom,
                     float top, float znear, float zfar);

void mat4_transform_point(const float m[16], float x, float y, float z,
                          float *ox, float *oy, float *oz);

void mat4_print(const char *name, const float m[16]);
void apply_matrix(const float m[16]); /* aqui aplicamos a matriz na OpenGL */

#endif
