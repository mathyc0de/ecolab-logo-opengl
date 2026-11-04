#include "gl_platform.h"

#include "petal.h"
#include "math3d.h"

#include <math.h>

#define PETAL_RINGS 20        /* quantos anéis do miolo até a borda */
#define PETAL_STRANDS 3       /* quantas mechas uma pétala tem */
#define PETAL_SEGMENTS 96     /* quantos pontos fecham uma volta do anel */
#define PETAL_SLICES 8        /* quantos lados tem a grossura do traço */
#define PETAL_TUBE_RADIUS 0.0082f /* grossura de cada traço */
#define PETAL_DEPTH 0.50f     /* quanto o miolo afunda para trás */

/* Deixa a seta da superfície do tamanho certo. Seta nula aponta para a frente. */
/* Divide x, y e z pelo comprimento. Se o comprimento é quase zero, usa (0, 0, 1). */
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

/* Escolhe a cor do anel: escura no miolo, clara na borda. */
/* Mistura as duas cores: quanto maior u, mais entra a cor clara. */
static void petal_shade(float out[3], const float dark[3], const float light[3], float u)
{
    int i;

    for (i = 0; i < 3; i++) {
        out[i] = dark[i] + (light[i] - dark[i]) * u;
    }
}

/* Acha um ponto do oval e as direções usadas para engrossar o traço. */
/* O ponto sai de cos e sen. As direções saem da derivada, de um giro de 90° e do produto entre elas. */
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

    /* O ponto do oval. Ao longo da volta ele vai para trás: de lado vira funil. */
    /* x = rx cos t, y = ry sen t, e z sobe de z0 até z1 enquanto t vai de 0 a 2π. */
    *px = rx * c;
    *py = ry * s;
    *pz = z0 + (z1 - z0) * (t / (2.f * MATH3D_PI));

    /* Direção em que o traço está andando. */
    /* É a derivada do ponto, (-rx sen t, ry cos t, dz/dt), com comprimento 1. */
    normalize3(&tx, &ty, &tz);
    /* Direção para fora do oval. */
    /* Gira a direção do traço 90° no plano: (-ty, tx, 0), também com comprimento 1. */
    *nx = -ty;
    *ny = tx;
    *nz = 0.f;
    normalize3(nx, ny, nz);

    /* A outra direção, para o traço ficar redondo e não chato. */
    /* Produto da direção do traço pela direção para fora. As duas fecham o círculo da grossura. */
    *bx = ty * (*nz) - tz * (*ny);
    *by = tz * (*nx) - tx * (*nz);
    *bz = tx * (*ny) - ty * (*nx);
    normalize3(bx, by, bz);
}

/* Põe um ponto na beirada do traço, um pouco para fora do oval. */
/* Soma ao centro o raio na direção cos·fora + sen·lado, e essa mesma direção é a seta da superfície. */
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

/* Constrói um anel: pinta e liga os pontos em volta do oval. */
/* Uma cor para o anel. Cada lado da grossura vira uma faixa, com os dois pontos da faixa em cada passo da volta. */
static void draw_ring(float rx, float ry, float z0, float z1, float u,
                      const float dark[3], const float light[3])
{
    float rgb[3];
    int slice;

    /* O anel inteiro fica da mesma cor. */
    /* u mistura escuro e claro uma vez, e essa cor vale para todos os pontos do anel. */
    petal_shade(rgb, dark, light, u);
    glColor3fv(rgb);

    for (slice = 0; slice < PETAL_SLICES; slice++) {
        /* Um lado da grossura do traço. */
        /* a0 e a1 são dois ângulos vizinhos, a volta inteira dividida em 8. */
        float a0 = (2.f * MATH3D_PI * (float)slice) / (float)PETAL_SLICES;
        float a1 = (2.f * MATH3D_PI * (float)(slice + 1)) / (float)PETAL_SLICES;
        int i;

        glBegin(GL_TRIANGLE_STRIP);
        for (i = 0; i <= PETAL_SEGMENTS; i++) {
            /* Um passo da volta. Os dois pontos fecham essa faixa do traço. */
            /* t percorre a volta. Em cada t entram o ponto do ângulo a0 e o do ângulo a1. */
            float t = (2.f * MATH3D_PI * (float)i) / (float)PETAL_SEGMENTS;
            float px, py, pz, nx, ny, nz, bx, by, bz;

            ellipse_frame(t, rx, ry, z0, z1, &px, &py, &pz, &nx, &ny, &nz, &bx, &by, &bz);
            tube_vertex(px, py, pz, nx, ny, nz, bx, by, bz, a0, PETAL_TUBE_RADIUS);
            tube_vertex(px, py, pz, nx, ny, nz, bx, by, bz, a1, PETAL_TUBE_RADIUS);
        }
        glEnd();
    }
}

/* Decide o lugar do anel: juntos no miolo, mais abertos na borda. */
/* pow(u, expoente), com expoente > 1, segura os anéis no miolo. No meio, sen² mistura de volta com o passo igual. */
static float ring_position(float u, float exponent)
{
    float mid;

    /* Não deixa passar do miolo nem da borda. */
    /* u abaixo de 0 vira 0. u acima de 1 vira 1. */
    if (u < 0.f) {
        u = 0.f;
    }
    if (u > 1.f) {
        u = 1.f;
    }
    /* No meio do caminho o vão entre os anéis não abre demais. */
    /* sen(πu)² vale 0 nas pontas e 1 no meio, e é o peso dessa mistura. */
    mid = sinf(u * MATH3D_PI);
    mid = mid * mid;
    return powf(u, exponent) * (1.f - 0.62f * mid) + u * (0.62f * mid);
}

/* Monta uma mecha, anel por anel. Cada mecha fica um pouco deslocada, senão as três se encostam. */
/* O deslocamento 0, 1/3 e 2/3 separa os anéis. Um extra em z faz as mechas se cruzarem de lado. */
static void draw_strand(const float dark[3], const float light[3],
                        float offset, float exponent)
{
    /* Empurra esta mecha um pouco para trás ou para frente. */
    /* phase = (offset − 0,5) × 0,06, somado no z de todos os anéis desta mecha. */
    float phase = (offset - 0.5f) * 0.06f;
    int ring;

    for (ring = 0; ring < PETAL_RINGS; ring++) {
        /* Começo e fim deste anel, ainda espaçados por igual. */
        /* u0 e u1 repartem a mecha em 20 partes e somam o deslocamento dela. */
        float u0 = ((float)ring + offset) / (float)PETAL_RINGS;
        float u1 = ((float)(ring + 1) + offset) / (float)PETAL_RINGS;
        float v0;
        float v1;

        /* O último anel para na borda. */
        /* Se u1 passar de 1, corta em 1. */
        if (u1 > 1.f) {
            u1 = 1.f;
        }
        /* Onde o anel senta de verdade, já mais perto do miolo. */
        /* A posição do anel aplica o expoente em u0 e em u1. */
        v0 = ring_position(u0, exponent);
        v1 = ring_position(u1, exponent);
        /* Sem isso o primeiro anel nasce sem largura e some. */
        /* Se v1 − v0 for menor que 0,006, v1 vai para v0 + 0,006. */
        if (v1 < v0 + 0.006f) {
            v1 = v0 + 0.006f;
        }

        /* Oval mais comprido para cima. Miolo atrás, borda na frente. */
        /* rx e ry crescem com v. z = −profundidade × (1−v)^0,72 + phase. */
        draw_ring(0.058f + v0 * 0.29f, 0.090f + v0 * 0.46f,
                  -PETAL_DEPTH * powf(1.f - v0, 0.72f) + phase,
                  -PETAL_DEPTH * powf(1.f - v1, 0.72f) + phase,
                  v0, dark, light);
    }
}

/* Desenha a pétala: três mechas, cada uma mais fechada no miolo que a outra. */
/* Os expoentes 2,15, 1,75 e 1,38 apertam cada mecha num grau. O deslocamento é 0, 1/3 e 2/3. */
void petal_draw(const float dark[3], const float light[3])
{
    static const float kExp[PETAL_STRANDS] = {2.15f, 1.75f, 1.38f};
    int strand;

    for (strand = 0; strand < PETAL_STRANDS; strand++) {
        draw_strand(dark, light, (float)strand / (float)PETAL_STRANDS, kExp[strand]);
    }
}
