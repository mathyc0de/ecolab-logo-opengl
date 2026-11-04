#include "gl_platform.h"

#include "petal.h"
#include "math3d.h"

#include <math.h>

#define PETAL_RINGS 20       /* contornos do miolo até a borda */
#define PETAL_STRANDS 3      /* mechas entrelaçadas, cada uma com seu expoente */
#define PETAL_SEGMENTS 96    /* amostras ao longo de uma volta */
#define PETAL_SLICES 8       /* lados da seção do tubo; 8 deixa o GL_FLAT visível */
#define PETAL_TUBE_RADIUS 0.0082f
#define PETAL_DEPTH 0.50f    /* quanto o miolo recua em z */

/* A luz usa N com comprimento 1. Vetor nulo vira +Z para não gerar NaN. */
static void normalize3(float *x, float *y, float *z)
{
    float len = sqrtf(*x * *x + *y * *y + *z * *z);

    if (len < 1e-8f) {
        *x = 0.f;
        *y = 0.f;
        *z = 1.f;
        return;
    }
    *x /= len;
    *y /= len;
    *z /= len;
}

/* u = 0 no miolo, u = 1 na borda. A cor entra no glColor e vira o kd do Phong. */
static void petal_shade(float out[3], const float dark[3], const float light[3], float u)
{
    int i;

    for (i = 0; i < 3; i++) {
        out[i] = dark[i] + (light[i] - dark[i]) * u;
    }
}

/*
 * Um ponto da elipse e os dois eixos da seção do tubo.
 * z sobe ao longo da volta: de frente é elipse, de lado é funil.
 */
static void ellipse_frame(float t, float rx, float ry, float z0, float z1,
                          float *px, float *py, float *pz,
                          float *nx, float *ny, float *nz,
                          float *bx, float *by, float *bz)
{
    float c = cosf(t);
    float s = sinf(t);
    float tx = -rx * s;
    float ty = ry * c;
    float tz = (z1 - z0) / (2.f * MATH3D_PI);

    /* Ponto da elipse. t vai de 0 a 2π e z vai de z0 a z1 nessa volta. */
    *px = rx * c;
    *py = ry * s;
    *pz = z0 + (z1 - z0) * (t / (2.f * MATH3D_PI));

    /* Tangente: derivada (-rx sen t, ry cos t, dz/dt). */
    normalize3(&tx, &ty, &tz);
    /* Girar a tangente 90° no XY dá a normal para fora da curva. */
    *nx = -ty;
    *ny = tx;
    *nz = 0.f;
    normalize3(nx, ny, nz);

    /* Binormal = tangente × normal. N e B fecham o círculo da seção. */
    *bx = ty * (*nz) - tz * (*ny);
    *by = tz * (*nx) - tx * (*nz);
    *bz = tx * (*ny) - ty * (*nx);
    normalize3(bx, by, bz);
}

/* Um vértice do tubo: centro da curva mais o raio na direção (cos N + sen B).
   A normal aponta para fora, que é o que o N·L precisa. */
static void tube_vertex(float px, float py, float pz,
                        float nx, float ny, float nz,
                        float bx, float by, float bz,
                        float angle, float radius)
{
    float c = cosf(angle);
    float s = sinf(angle);
    float ox = c * nx + s * bx;
    float oy = c * ny + s * by;
    float oz = c * nz + s * bz;

    glNormal3f(ox, oy, oz);
    glVertex3f(px + radius * ox, py + radius * oy, pz + radius * oz);
}

/*
 * Um anel. Cada fatia da seção vira uma triangle strip ao longo da elipse:
 * em todo t mandamos o vértice do lado a0 e o do lado a1.
 */
static void draw_ring(float rx, float ry, float z0, float z1, float u,
                      const float dark[3], const float light[3])
{
    float rgb[3];
    int slice;

    petal_shade(rgb, dark, light, u);
    glColor3fv(rgb);

    for (slice = 0; slice < PETAL_SLICES; slice++) {
        float a0 = (2.f * MATH3D_PI * (float)slice) / (float)PETAL_SLICES;
        float a1 = (2.f * MATH3D_PI * (float)(slice + 1)) / (float)PETAL_SLICES;
        int i;

        glBegin(GL_TRIANGLE_STRIP);
        for (i = 0; i <= PETAL_SEGMENTS; i++) {
            float t = (2.f * MATH3D_PI * (float)i) / (float)PETAL_SEGMENTS;
            float px, py, pz, nx, ny, nz, bx, by, bz;

            ellipse_frame(t, rx, ry, z0, z1, &px, &py, &pz, &nx, &ny, &nz, &bx, &by, &bz);
            tube_vertex(px, py, pz, nx, ny, nz, bx, by, bz, a0, PETAL_TUBE_RADIUS);
            tube_vertex(px, py, pz, nx, ny, nz, bx, by, bz, a1, PETAL_TUBE_RADIUS);
        }
        glEnd();
    }
}

/*
 * u cresce parelho de 0 a 1. pow(u, expoente) com expoente > 1 junta os anéis
 * no miolo e abre a borda. No meio do caminho (sen²) misturamos de volta com u,
 * senão o espaço entre anéis estoura antes da borda.
 */
static float ring_position(float u, float exponent)
{
    float mid;

    if (u < 0.f) {
        u = 0.f;
    }
    if (u > 1.f) {
        u = 1.f;
    }
    mid = sinf(u * MATH3D_PI);
    mid = mid * mid;
    return powf(u, exponent) * (1.f - 0.62f * mid) + u * (0.62f * mid);
}

/*
 * Uma mecha. offset em {0, 1/3, 2/3} desloca os anéis para eles não coincidirem.
 * phase empurra a mecha em z, e é isso que faz as três se cruzarem de lado.
 */
static void draw_strand(const float dark[3], const float light[3],
                        float offset, float exponent)
{
    float phase = (offset - 0.5f) * 0.06f;
    int ring;

    for (ring = 0; ring < PETAL_RINGS; ring++) {
        float u0 = ((float)ring + offset) / (float)PETAL_RINGS;
        float u1 = ((float)(ring + 1) + offset) / (float)PETAL_RINGS;
        float v0;
        float v1;

        if (u1 > 1.f) {
            u1 = 1.f;
        }
        v0 = ring_position(u0, exponent);
        v1 = ring_position(u1, exponent);
        /* O pow achata o passo no miolo. Sem este piso o tubo nasce com comprimento 0. */
        if (v1 < v0 + 0.006f) {
            v1 = v0 + 0.006f;
        }

        /* rx, ry: oval mais comprida no eixo da pétala. z: miolo para trás, borda em 0. */
        draw_ring(0.058f + v0 * 0.29f, 0.090f + v0 * 0.46f,
                  -PETAL_DEPTH * powf(1.f - v0, 0.72f) + phase,
                  -PETAL_DEPTH * powf(1.f - v1, 0.72f) + phase,
                  v0, dark, light);
    }
}

void petal_draw(const float dark[3], const float light[3])
{
    /* Cada mecha aperta o miolo num grau diferente, e elas se cruzam. */
    static const float kExp[PETAL_STRANDS] = {2.15f, 1.75f, 1.38f};
    int strand;

    for (strand = 0; strand < PETAL_STRANDS; strand++) {
        draw_strand(dark, light, (float)strand / (float)PETAL_STRANDS, kExp[strand]);
    }
}
