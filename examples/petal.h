#ifndef PETAL_H
#define PETAL_H

/*
 * Uma pétala, nascida no +Y e centrada na origem.
 * main.c faz as quatro cópias: escala, translação e rotação em Z.
 * dark e light são RGB em 0..1; o miolo usa dark, a borda usa light.
 */

void petal_draw(const float dark[3], const float light[3]);

#endif
