#include "gl_platform.h"

#include "petal.h"
#include "math3d.h"

#include <math.h>

#define PETAL_RINGS 18
#define PETAL_STRANDS 3
#define PETAL_SEGMENTS 80
#define PETAL_SLICES 7
#define PETAL_TUBE_RADIUS 0.0043f
#define PETAL_DEPTH 0.34f

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

static void petal_shade(float out[3], const float dark[3], const float light[3],
                        float ring_u, float cy, float ry)
{
    float along;
    float s;
    int i;

    along = 0.5f + 0.5f * (cy / (ry > 1e-4f ? ry : 1.f));
    s = 0.78f * ring_u + 0.22f * along;
    if (s < 0.f) {
        s = 0.f;
    }
    if (s > 1.f) {
        s = 1.f;
    }
    s = s * s * (3.f - 2.f * s);

    for (i = 0; i < 3; i++) {
        out[i] = dark[i] + (light[i] - dark[i]) * s;
    }
}

static void helix_frame(float t, float rx, float ry, float z0, float z1,
                        float *px, float *py, float *pz,
                        float *nx, float *ny, float *nz,
                        float *bx, float *by, float *bz)
{
    float c = cosf(t);
    float s = sinf(t);
    float span = z1 - z0;
    float tx;
    float ty;
    float tz;
    float ux;
    float uy;
    float uz;

    *px = rx * c;
    *py = ry * s;
    *pz = z0 + span * (t / (2.f * MATH3D_PI)); /* elipse de frente, funil de lado */

    tx = -rx * s;
    ty = ry * c;
    tz = span / (2.f * MATH3D_PI);
    normalize3(&tx, &ty, &tz);

    ux = -ty;
    uy = tx;
    uz = 0.f;
    if (ux * ux + uy * uy + uz * uz < 1e-8f) {
        ux = tz;
        uy = 0.f;
        uz = -tx;
    }
    normalize3(&ux, &uy, &uz);

    *nx = ux;
    *ny = uy;
    *nz = uz;
    *bx = ty * uz - tz * uy;
    *by = tz * ux - tx * uz;
    *bz = tx * uy - ty * ux;
    normalize3(bx, by, bz);
}

static void draw_vortex_turn(float rx, float ry, float z0, float z1, float radius,
                             const float dark[3], const float light[3], float ring_u)
{
    int slice;

    for (slice = 0; slice < PETAL_SLICES; slice++) {
        float a0 = (2.f * MATH3D_PI * (float)slice) / (float)PETAL_SLICES;
        float a1 = (2.f * MATH3D_PI * (float)(slice + 1)) / (float)PETAL_SLICES;
        float c0 = cosf(a0);
        float s0 = sinf(a0);
        float c1 = cosf(a1);
        float s1 = sinf(a1);
        int i;

        glBegin(GL_TRIANGLE_STRIP);
        for (i = 0; i <= PETAL_SEGMENTS; i++) {
            float t = (2.f * MATH3D_PI * (float)i) / (float)PETAL_SEGMENTS;
            float px;
            float py;
            float pz;
            float nx;
            float ny;
            float nz;
            float bx;
            float by;
            float bz;
            float rgb[3];
            int k;

            helix_frame(t, rx, ry, z0, z1, &px, &py, &pz, &nx, &ny, &nz, &bx, &by, &bz);
            petal_shade(rgb, dark, light, ring_u, py, ry);

            for (k = 0; k < 2; k++) {
                float ca = (k == 0) ? c0 : c1;
                float sa = (k == 0) ? s0 : s1;
                float ox = ca * nx + sa * bx;
                float oy = ca * ny + sa * by;
                float oz = ca * nz + sa * bz;

                glColor3f(rgb[0], rgb[1], rgb[2]);
                glNormal3f(ox, oy, oz);
                glVertex3f(px + radius * ox, py + radius * oy, pz + radius * oz);
            }
        }
        glEnd();
    }
}

static float remap_ring(float t, float exponent, int ring)
{
    float v;
    float wobble;

    if (t < 0.f) {
        t = 0.f;
    }
    if (t > 1.f) {
        t = 1.f;
    }
    v = powf(t, exponent); /* mudamos o vão: uns anéis juntos no miolo, outros na borda */
    wobble = 0.016f * sinf((float)ring * 2.15f + exponent * 3.7f);
    v += wobble * v * (1.f - v);
    if (v < 0.f) {
        v = 0.f;
    }
    if (v > 1.f) {
        v = 1.f;
    }
    return v;
}

static void draw_strand(const float dark[3], const float light[3],
                        int rings, float u_offset, float exponent)
{
    int ring;

    for (ring = 0; ring < rings; ring++) {
        float u0 = ((float)ring + u_offset) / (float)rings;
        float u1 = ((float)(ring + 1) + u_offset) / (float)rings;
        float v0;
        float v1;
        float rx;
        float ry;
        float z0;
        float z1;

        if (u0 >= 1.f) {
            break;
        }
        if (u1 > 1.f) {
            u1 = 1.f;
        }

        v0 = remap_ring(u0, exponent, ring);
        v1 = remap_ring(u1, exponent, ring + 1);
        if (v1 < v0 + 0.012f) {
            v1 = v0 + 0.012f;
        }

        rx = 0.05f + v0 * 0.275f;
        ry = 0.092f + v0 * 0.50f;
        /* Recuamos o miolo e defasamos as mechas pra elas se cruzarem. */
        z0 = -PETAL_DEPTH * (1.f - v0) + (u_offset - 0.5f) * 0.05f;
        z1 = -PETAL_DEPTH * (1.f - v1) + (u_offset - 0.5f) * 0.05f;
        draw_vortex_turn(rx, ry, z0, z1, PETAL_TUBE_RADIUS, dark, light, v0);
    }
}

void petal_draw(const float dark[3], const float light[3])
{
    /* Três ritmos de espaçamento, como na foto os anéis não são iguais. */
    static const float kExp[PETAL_STRANDS] = {1.48f, 1.05f, 0.72f};
    int strand;

    for (strand = 0; strand < PETAL_STRANDS; strand++) {
        draw_strand(dark, light, PETAL_RINGS,
                    (float)strand / (float)PETAL_STRANDS, kExp[strand]);
    }
}
