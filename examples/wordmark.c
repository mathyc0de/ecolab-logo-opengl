#include "gl_platform.h"

#include "wordmark.h"
#include "bresenham.h"

#include <string.h>

#define WORDMARK_MAX_PIXELS 4096
#define WORDMARK_GRID_W 128
#define WORDMARK_GRID_H 22
#define WORDMARK_BRUSH 1

typedef struct {
    const int (*seg)[4];
    int count;
    int advance;
} Glyph;

typedef struct {
    int xs[WORDMARK_MAX_PIXELS];
    int ys[WORDMARK_MAX_PIXELS];
    unsigned char seen[WORDMARK_GRID_H][WORDMARK_GRID_W];
    int count;
} PixelList;

/* Contorno das letras. y pra cima. Cada segmento passamos no Bresenham/DDA. */

static const int seg_e[][4] = {
    {1, 8, 13, 8},
    {13, 8, 13, 12},
    {13, 12, 11, 14},
    {11, 14, 7, 15},
    {7, 15, 3, 14},
    {3, 14, 1, 12},
    {1, 12, 0, 8},
    {0, 8, 1, 4},
    {1, 4, 3, 2},
    {3, 2, 7, 1},
    {7, 1, 11, 2},
};

static const int seg_minus[][4] = {
    {1, 8, 8, 8},
};

static const int seg_c[][4] = {
    {11, 14, 7, 15},
    {7, 15, 3, 14},
    {3, 14, 1, 12},
    {1, 12, 0, 8},
    {0, 8, 1, 4},
    {1, 4, 3, 2},
    {3, 2, 7, 1},
    {7, 1, 11, 2},
};

static const int seg_o[][4] = {
    {14, 8, 13, 12},
    {13, 12, 11, 14},
    {11, 14, 7, 15},
    {7, 15, 3, 14},
    {3, 14, 1, 12},
    {1, 12, 0, 8},
    {0, 8, 1, 4},
    {1, 4, 3, 2},
    {3, 2, 7, 1},
    {7, 1, 11, 2},
    {11, 2, 13, 4},
    {13, 4, 14, 8},
};

static const int seg_l[][4] = {
    {3, 16, 3, 1},
};

static const int seg_a[][4] = {
    {12, 8, 11, 11},
    {11, 11, 9, 13},
    {9, 13, 6, 14},
    {6, 14, 3, 13},
    {3, 13, 1, 11},
    {1, 11, 0, 8},
    {0, 8, 1, 5},
    {1, 5, 3, 3},
    {3, 3, 6, 2},
    {6, 2, 9, 3},
    {9, 3, 11, 5},
    {11, 5, 12, 8},
    {12, 8, 12, 1},
};

static const int seg_b[][4] = {
    {2, 16, 2, 1},
    {2, 12, 5, 14},
    {5, 14, 8, 14},
    {8, 14, 11, 12},
    {11, 12, 13, 10},
    {13, 10, 14, 7},
    {14, 7, 13, 4},
    {13, 4, 11, 2},
    {11, 2, 8, 1},
    {8, 1, 5, 1},
    {5, 1, 2, 3},
};

#define GLYPH(arr, adv) { arr, (int)(sizeof(arr) / sizeof((arr)[0])), adv }

static const Glyph kWord[] = {
    GLYPH(seg_e, 17),
    GLYPH(seg_minus, 11),
    GLYPH(seg_c, 17),
    GLYPH(seg_o, 17),
    GLYPH(seg_l, 10),
    GLYPH(seg_a, 17),
    GLYPH(seg_b, 17),
};

static int g_use_dda = 0;

const char *wordmark_algorithm_name(void)
{
    return g_use_dda ? "DDA" : "Bresenham";
}

const char *wordmark_toggle_algorithm(void)
{
    g_use_dda = !g_use_dda;
    return wordmark_algorithm_name();
}

static void plot_dot(int x, int y, PixelList *list)
{
    if (x < 0 || y < 0 || x >= WORDMARK_GRID_W || y >= WORDMARK_GRID_H) {
        return;
    }
    if (list->seen[y][x]) {
        return;
    }
    if (list->count >= WORDMARK_MAX_PIXELS) {
        return;
    }

    list->seen[y][x] = 1;
    list->xs[list->count] = x;
    list->ys[list->count] = y;
    list->count++;
}

static void plot_store(int x, int y, void *user)
{
    /* Engrosso o traço em volta do pixel que o algoritmo marcou. */
    PixelList *list = (PixelList *)user;
    int dx;
    int dy;

    for (dy = -WORDMARK_BRUSH; dy <= WORDMARK_BRUSH; dy++) {
        for (dx = -WORDMARK_BRUSH; dx <= WORDMARK_BRUSH; dx++) {
            if (dx * dx + dy * dy > WORDMARK_BRUSH * WORDMARK_BRUSH + 1) {
                continue;
            }
            plot_dot(x + dx, y + dy, list);
        }
    }
}

static void rasterize_word(PixelList *list)
{
    int glyph_index;
    int pen_x = 2;
    void (*line)(int, int, int, int, PlotFn, void *);

    memset(list, 0, sizeof(*list));
    /* Tecla D troca o algoritmo. Os dois são nossos, em bresenham.c. */
    line = g_use_dda ? dda_line : bresenham_line;

    for (glyph_index = 0; glyph_index < (int)(sizeof(kWord) / sizeof(kWord[0]));
         glyph_index++) {
        const Glyph *glyph = &kWord[glyph_index];
        int s;

        for (s = 0; s < glyph->count; s++) {
            line(pen_x + glyph->seg[s][0], glyph->seg[s][1],
                 pen_x + glyph->seg[s][2], glyph->seg[s][3],
                 plot_store, list);
        }
        pen_x += glyph->advance;
    }
}

void wordmark_draw(float center_x, float baseline_y, float pixel_size,
                   float r, float g, float b)
{
    PixelList list;
    int max_x = 0;
    float ox;
    float cell;
    int i;

    rasterize_word(&list);
    if (list.count == 0) {
        return;
    }

    for (i = 0; i < list.count; i++) {
        if (list.xs[i] > max_x) {
            max_x = list.xs[i];
        }
    }

    ox = center_x - (float)(max_x + 1) * pixel_size * 0.5f;
    cell = pixel_size * 0.92f;

    glColor3f(r, g, b);
    glBegin(GL_QUADS);
    for (i = 0; i < list.count; i++) {
        float x0 = ox + (float)list.xs[i] * pixel_size;
        float y0 = baseline_y + (float)list.ys[i] * pixel_size;
        float x1 = x0 + cell;
        float y1 = y0 + cell;
        float z0 = 0.f;
        float z1 = cell * 0.40f;

        glNormal3f(0.f, 0.f, 1.f);
        glVertex3f(x0, y0, z1);
        glVertex3f(x1, y0, z1);
        glVertex3f(x1, y1, z1);
        glVertex3f(x0, y1, z1);

        glNormal3f(0.f, 0.f, -1.f);
        glVertex3f(x1, y0, z0);
        glVertex3f(x0, y0, z0);
        glVertex3f(x0, y1, z0);
        glVertex3f(x1, y1, z0);

        glNormal3f(0.f, -1.f, 0.f);
        glVertex3f(x0, y0, z0);
        glVertex3f(x1, y0, z0);
        glVertex3f(x1, y0, z1);
        glVertex3f(x0, y0, z1);

        glNormal3f(0.f, 1.f, 0.f);
        glVertex3f(x0, y1, z1);
        glVertex3f(x1, y1, z1);
        glVertex3f(x1, y1, z0);
        glVertex3f(x0, y1, z0);

        glNormal3f(-1.f, 0.f, 0.f);
        glVertex3f(x0, y0, z0);
        glVertex3f(x0, y0, z1);
        glVertex3f(x0, y1, z1);
        glVertex3f(x0, y1, z0);

        glNormal3f(1.f, 0.f, 0.f);
        glVertex3f(x1, y0, z1);
        glVertex3f(x1, y0, z0);
        glVertex3f(x1, y1, z0);
        glVertex3f(x1, y1, z1);
    }
    glEnd();
}
