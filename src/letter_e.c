#include "gl_platform.h"

#include "letter_e.h"

#include <stdlib.h>

/* Acende uma célula. O +0,5 põe o ponto no meio dela, não no canto. */
static void plot(int x, int y)
{
    glVertex2f((float)x + 0.5f, (float)y + 0.5f);
}

/*
 * Reta de Bresenham, em qualquer direção.
 * Anda sempre 1 no eixo maior; p decide se o eixo menor também anda.
 */
static void bresenham_line(int x0, int y0, int x1, int y1)
{
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = x1 >= x0 ? 1 : -1;
    int sy = y1 >= y0 ? 1 : -1;
    /* Mais inclinada que 45°: o eixo maior é y, então os papéis trocam. */
    int steep = dy > dx;
    int x = x0;
    int y = y0;
    int p;
    int i;

    if (steep) {
        int tmp = dx;
        dx = dy;
        dy = tmp;
    }

    p = 2 * dy - dx;
    for (i = 0; i <= dx; i++) {
        plot(x, y);
        if (p >= 0) {
            if (steep) {
                x += sx;
            } else {
                y += sy;
            }
            p -= 2 * dx;
        }
        p += 2 * dy;
        if (steep) {
            y += sy;
        } else {
            x += sx;
        }
    }
}


static void bresenham_circle_e(int cx, int cy, int r)
{
    int x = 0;
    int y = r;
    int p = 1 - r;

    while (x <= y) {
        plot(cx + y, cy + x); /*   0° a  45° */
        plot(cx + x, cy + y); /*  45° a  90° */
        plot(cx - x, cy + y); /*  90° a 135° */
        plot(cx - y, cy + x); /* 135° a 180° */
        plot(cx - y, cy - x); /* 180° a 225° */
        plot(cx - x, cy - y); /* 225° a 270° */
        plot(cx + x, cy - y); /* 270° a 315° */
        /* 315° a 360°: (cx + y, cy - x) fica de fora, é a abertura do e. */

        x++;
        if (p < 0) {
            p += 2 * x + 1;
        } else {
            y--;
            p += 2 * (x - y) + 1;
        }
    }
}

void letter_e_draw(int cx, int cy, int radius)
{
    glBegin(GL_POINTS);
    /* A barra vai de uma borda do círculo à outra, na altura do centro. */
    bresenham_line(cx - radius, cy, cx + radius, cy);
    bresenham_circle_e(cx, cy, radius);
    glEnd();
}
