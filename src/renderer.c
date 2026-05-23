#include "renderer.h"
#include <stdlib.h>

// Algoritmo de Bresenham
void dibujar_linea(SDL_Surface *surface, int x0, int y0, int x1, int y1, Uint32 color) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int error = dx - dy;
    int error2;

    SDL_LockSurface(surface);
    Uint32 *pixeles = (Uint32 *)surface->pixels;

    while (1) {
        if (x0 >= 0 && x0 < 800 && y0 >= 0 && y0 < 600)
            pixeles[y0 * 800 + x0] = color;
        if (x0 == x1 && y0 == y1) break;
        error2 = 2 * error;
        if (error2 > -dy) { error -= dy; x0 += sx; }
        if (error2 <  dx) { error += dx; y0 += sy; }
    }

    SDL_UnlockSurface(surface);
}

static void swap_int(int *a, int *b)     { int t = *a; *a = *b; *b = t; }
static void swap_float(float *a, float *b) { float t = *a; *a = *b; *b = t; }

void dibujar_pixel_sin_lock(Uint32 *pixeles, float *zbuffer, int x, int y, float z, Uint32 color) {
    if (x >= 0 && x < 800 && y >= 0 && y < 600) {
        if (z < zbuffer[y * 800 + x]) {
            zbuffer[y * 800 + x] = z;
            pixeles[y * 800 + x] = color;
        }
    }
}

void rellenar_triangulo(SDL_Surface *surface, float *zbuffer,
                        int x1, int y1, float z1,
                        int x2, int y2, float z2,
                        int x3, int y3, float z3,
                        Uint32 color) {
    if (y1 > y2) { swap_int(&x1, &x2); swap_int(&y1, &y2); swap_float(&z1, &z2); }
    if (y1 > y3) { swap_int(&x1, &x3); swap_int(&y1, &y3); swap_float(&z1, &z3); }
    if (y2 > y3) { swap_int(&x2, &x3); swap_int(&y2, &y3); swap_float(&z2, &z3); }

    float fx1 = x1, fy1 = y1, fz1 = z1;
    float fx2 = x2, fy2 = y2, fz2 = z2;
    float fx3 = x3, fy3 = y3, fz3 = z3;

    float dy_largo = fy3 - fy1;
    if (dy_largo == 0) return;

    SDL_LockSurface(surface);
    Uint32 *pixeles = (Uint32 *)surface->pixels;

    // Mitad superior
    if (fy2 != fy1) {
        float dy_corto = fy2 - fy1;
        for (int y = (int)fy1; y <= (int)fy2; y++) {
            float t       = (y - fy1) / dy_largo;
            float x_largo = fx1 + t * (fx3 - fx1);
            float z_largo = fz1 + t * (fz3 - fz1);
            float t_c     = (y - fy1) / dy_corto;
            float x_corto = fx1 + t_c * (fx2 - fx1);
            float z_corto = fz1 + t_c * (fz2 - fz1);

            float x_izq = x_largo, z_izq = z_largo;
            float x_der = x_corto, z_der = z_corto;
            if (x_izq > x_der) { swap_float(&x_izq, &x_der); swap_float(&z_izq, &z_der); }

            for (int x = (int)x_izq; x <= (int)x_der; x++) {
                float dx_scan = x_der - x_izq;
                float t_scan  = (dx_scan == 0) ? 0 : (x - x_izq) / dx_scan;
                dibujar_pixel_sin_lock(pixeles, zbuffer, x, y,
                                       z_izq + t_scan * (z_der - z_izq), color);
            }
        }
    }

    // Mitad inferior
    if (fy3 != fy2) {
        float dy_corto = fy3 - fy2;
        for (int y = (int)fy2; y <= (int)fy3; y++) {
            float t       = (y - fy1) / dy_largo;
            float x_largo = fx1 + t * (fx3 - fx1);
            float z_largo = fz1 + t * (fz3 - fz1);
            float t_c     = (y - fy2) / dy_corto;
            float x_corto = fx2 + t_c * (fx3 - fx2);
            float z_corto = fz2 + t_c * (fz3 - fz2);

            float x_izq = x_largo, z_izq = z_largo;
            float x_der = x_corto, z_der = z_corto;
            if (x_izq > x_der) { swap_float(&x_izq, &x_der); swap_float(&z_izq, &z_der); }

            for (int x = (int)x_izq; x <= (int)x_der; x++) {
                float dx_scan = x_der - x_izq;
                float t_scan  = (dx_scan == 0) ? 0 : (x - x_izq) / dx_scan;
                dibujar_pixel_sin_lock(pixeles, zbuffer, x, y,
                                       z_izq + t_scan * (z_der - z_izq), color);
            }
        }
    }

    SDL_UnlockSurface(surface);
}
