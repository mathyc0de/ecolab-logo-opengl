#include "gl_platform.h"

#include "math3d.h"

#include <math.h>
#include <stdio.h>

/* Identidade deixa o ponto no mesmo lugar. As outras matrizes começam dela. */
/* Multiplica x, y e z por 1 e soma 0, então o ponto sai igual. */
void mat4_identity(float m[16])
{
    int i;

    for (i = 0; i < 16; i++) {
        m[i] = 0.f;
    }
    m[0] = m[5] = m[10] = m[15] = 1.f;
}

/* Multiplicar junta duas matrizes. A tecla M confere Rz · T · S no console. */
/* O ponto passa primeiro pela matriz da direita. Em Rz · T · S ele escala, depois anda, depois gira. */
void mat4_multiply(float out[16], const float a[16], const float b[16])
{
    float r[16];
    int col;
    int row;

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

/* Translação desloca o ponto. */
/* Soma tx em x, ty em y e tz em z. O ponto só anda, sem girar nem mudar de tamanho. */
void mat4_make_translation(float m[16], float tx, float ty, float tz)
{
    mat4_identity(m);
    m[12] = tx;
    m[13] = ty;
    m[14] = tz;
}

/* Escala aumenta ou diminui. */
/* Multiplica x por sx, y por sy e z por sz, em volta da origem. */
void mat4_make_scale(float m[16], float sx, float sy, float sz)
{
    mat4_identity(m);
    m[0] = sx;
    m[5] = sy;
    m[10] = sz;
}

/* Rotação em X gira a cena com o mouse. É o pitch. */
/* Sen e cos misturam Y com Z. X não entra, então o ponto sobe ou desce em volta desse eixo. */
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

/* Rotação em Y gira a cena com o mouse. É o yaw. */
/* Sen e cos misturam X com Z. Y fica, então o ponto gira para os lados. */
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

/* Rotação em Z manda cada pétala para uma diagonal. */
/* Sen e cos misturam X com Y. Z fica, então um ponto que estava em cima vai para a diagonal. */
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

/* Perspectiva faz o longe ficar menor. É a tecla P. */
/* Copia −z para w. A GPU divide x, y e z por w, e como o longe tem w maior, o ponto sai menor. */
void mat4_make_perspective(float m[16], float fovy_rad, float aspect,
                           float znear, float zfar)
{
    float f = 1.f / tanf(fovy_rad * 0.5f);
    int i;

    for (i = 0; i < 16; i++) {
        m[i] = 0.f;
    }

    m[0] = f / aspect;
    m[5] = f;
    m[10] = (zfar + znear) / (znear - zfar);
    m[11] = -1.f;
    m[14] = (2.f * zfar * znear) / (znear - zfar);
}

/* Ortogonal deixa longe e perto do mesmo tamanho. É a tecla O. */
/* w fica 1, então dividir por w não encolhe nada. Longe e perto saem do mesmo tamanho. */
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

/* Transformar um ponto aplica a matriz. A tecla M confere o resultado. */
/* Multiplica a matriz pelo ponto e para. A divisão que encolhe o fundo fica para a GPU. */
void mat4_transform_point(const float m[16], float x, float y, float z,
                          float *ox, float *oy, float *oz)
{
    *ox = m[0] * x + m[4] * y + m[8] * z + m[12];
    *oy = m[1] * x + m[5] * y + m[9] * z + m[13];
    *oz = m[2] * x + m[6] * y + m[10] * z + m[14];
}

/* Imprimir mostra a matriz no console. A tecla M confere a conta. */
/* Escreve por linha, como no papel, embora a memória guarde a matriz por coluna. */
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

/* Entrega a matriz para a OpenGL. */
/* Multiplica a matriz da cena pela nossa. A nova entra por último e, por isso, mexe no ponto primeiro. */
void apply_matrix(const float m[16])
{
    glMultMatrixf(m);
}
