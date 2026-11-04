#include "bresenham.h"

#include <math.h>
#include <stdlib.h>

void bresenham_line(int x0, int y0, int x1, int y1, PlotFn plot, void *user)
{
    /* Incremento só com inteiro. */
    int dx = abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    for (;;) {
        int e2;

        plot(x0, y0, user);
        if (x0 == x1 && y0 == y1) {
            break;
        }

        e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void dda_line(int x0, int y0, int x1, int y1, PlotFn plot, void *user)
{
    /* Mesma reta, mas o passo é float. */
    int dx = x1 - x0;
    int dy = y1 - y0;
    int steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);
    float xinc;
    float yinc;
    float x;
    float y;
    int i;

    if (steps == 0) {
        plot(x0, y0, user);
        return;
    }

    xinc = (float)dx / (float)steps;
    yinc = (float)dy / (float)steps;
    x = (float)x0;
    y = (float)y0;

    for (i = 0; i <= steps; i++) {
        plot((int)floorf(x + 0.5f), (int)floorf(y + 0.5f), user);
        x += xinc;
        y += yinc;
    }
}
