#ifndef LETTER_E_H
#define LETTER_E_H

/*
 * O "e" da logo, rasterizado à mão com Bresenham: uma reta para a barra
 * e um círculo sem um octante para a volta.
 * cx, cy e radius estão em células de uma grade inteira. Cada célula acesa
 * vira um GL_POINT no centro dela; quem chama escolhe a projeção e o
 * glPointSize que dão o tamanho da célula na tela.
 */

void letter_e_draw(int cx, int cy, int radius);

#endif
