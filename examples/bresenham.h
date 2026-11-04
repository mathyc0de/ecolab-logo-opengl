#ifndef BRESENHAM_H
#define BRESENHAM_H

/* Raster nosso. Não usamos GL_LINES; cada pixel vai num callback. */

typedef void (*PlotFn)(int x, int y, void *user);

void bresenham_line(int x0, int y0, int x1, int y1, PlotFn plot, void *user);
void dda_line(int x0, int y0, int x1, int y1, PlotFn plot, void *user);

#endif
