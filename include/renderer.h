#ifndef RENDERER_H
#define RENDERER_H

#include <SDL2/SDL.h>

void dibujar_linea(SDL_Surface *surface, int x0, int y0, int x1, int y1, Uint32 color);
void dibujar_pixel_sin_lock(Uint32 *pixeles, float *zbuffer, int x, int y, float z, Uint32 color);
void rellenar_triangulo(SDL_Surface *surface, float *zbuffer,
                        int x1, int y1, float z1,
                        int x2, int y2, float z2,
                        int x3, int y3, float z3,
                        Uint32 color);

#endif
