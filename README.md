# e-colab em OpenGL

Cena 3D da logo e-colab para aprender o pipeline clássico da OpenGL e cobrir o trabalho: modelagem, matrizes próprias com `glMultMatrix`, projeção perspectiva e ortogonal, Phong com Gouraud, Bresenham ou DDA, e Z-buffer.

O roteiro do grupo está em [docs/PLANO.md](docs/PLANO.md). A referência visual está em [docs/referencia-ecolab.jpg](docs/referencia-ecolab.jpg). O protótipo que implementa esse caminho está em `examples/`.

## Compilar

Com FreeGLUT instalado:

```text
cmake -S . -B build
cmake --build build
```

No MSYS2, o passo a passo e a linha direta do `gcc` estão no plano. O executável é `ecolab_exemplo`.

## Teclas

Arraste o mouse ou use as setas para orbitar. Roda do mouse, `[` `]` ou Page Up/Down dão zoom. `1`/`F` frente, `2` 3/4, `3` lado, `4` cima, `R` reinicia. `P` / `O` perspectiva e ortogonal. `W` aramado. `G` Gouraud. `L` luz. `D` Bresenham ou DDA. `Z` Z-buffer. `+` / `-` escala das pétalas. Espaço gira sozinho. `M` imprime as matrizes. Esc sai.
