# e-colab em OpenGL

Alunos: Karoline de Jesus Vieira e Matheus Pimenta Silveira

Cena 3D da logo e-colab para aprender o pipeline clássico da OpenGL e cobrir o trabalho: modelagem, matrizes próprias com `glMultMatrix`, projeção perspectiva e ortogonal, Phong com Gouraud e Z-buffer.



## Compilar

Com FreeGLUT instalado:

```text
cmake -S . -B build
cmake --build build
```

O executável estará disponível na pasta build, como ecolab_logo

## Teclas

Arraste o mouse ou use as setas para orbitar. Roda do mouse, `[` `]` ou Page Up/Down dão zoom. `1`/`F` frente, `2` 3/4, `3` lado, `4` cima, `R` reinicia. `P` / `O` perspectiva e ortogonal. `W` aramado. `G` Gouraud. `L` luz. `D` Bresenham ou DDA. `Z` Z-buffer. `+` / `-` escala das pétalas. Espaço gira sozinho. `M` imprime as matrizes. Esc sai.
