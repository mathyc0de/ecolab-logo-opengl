#ifndef WORDMARK_H
#define WORDMARK_H

/* A palavra do trabalho: Bresenham/DDA, cada pixel vira um cubinho. */

void wordmark_draw(float center_x, float baseline_y, float pixel_size,
                   float r, float g, float b);

const char *wordmark_algorithm_name(void);
const char *wordmark_toggle_algorithm(void);

#endif
