/*
 * main.c
 *
 * Copyright 2026 Alfred@mx
 * GPL v2
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <SDL2/SDL.h>

#include "types.h"
#include "renderer.h"
#include "loader.h"

// Definición de las variables globales declaradas en types.h
Vec3 *vertices    = NULL;
int   num_vertices = 0;
Cara *caras       = NULL;
int   num_caras    = 0;

int main(void) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("Error al iniciar SDL\n");
        return 1;
    }

    float angulo   = 0.0f;
    float velocidad = 0.0001f;
    float cam_x    = 0.0f;
    float cam_y    = 0.0f;

    Uint32 tiempo_anterior = SDL_GetTicks();
    int   frames = 0;
    float fps    = 0.0f;

    SDL_Window *window = SDL_CreateWindow(
        "Visor CPU 3D",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        800, 600,
        SDL_WINDOW_SHOWN
    );
    if (window == NULL) {
        printf("Error al crear la ventana\n");
        SDL_Quit();
        return 1;
    }

    SDL_Surface *surface = SDL_GetWindowSurface(window);

    Uint32 colores[] = {
        SDL_MapRGB(surface->format, 255,  60,  60),
        SDL_MapRGB(surface->format,  60, 200,  60),
        SDL_MapRGB(surface->format,  60, 120, 255),
        SDL_MapRGB(surface->format, 255, 210,  50),
        SDL_MapRGB(surface->format, 220,  60, 220),
        SDL_MapRGB(surface->format,  50, 210, 210),
    };

    float *zbuffer = malloc(800 * 600 * sizeof(float));
    if (zbuffer == NULL) {
        printf("Error al crear el zbuffer\n");
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    if (cargar_obj("modelo.obj") != 0) {
        printf("Error al cargar el modelo\n");
        free(zbuffer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_Event evento;
    int running = 1;

    while (running) {
        while (SDL_PollEvent(&evento)) {
            if (evento.type == SDL_QUIT) running = 0;
            if (evento.type == SDL_KEYDOWN) {
                switch (evento.key.keysym.sym) {
                    case SDLK_ESCAPE:    running = 0; break;
                    case SDLK_RIGHT:
                    case SDLK_KP_PLUS:  velocidad += 0.005f; printf("velocidad: %.3f\n", velocidad); break;
                    case SDLK_LEFT:
                    case SDLK_KP_MINUS: velocidad -= 0.005f; printf("velocidad: %.3f\n", velocidad); break;
                    case SDLK_0:        velocidad = 0.0f;    printf("pausado\n");                    break;
                    case SDLK_w: cam_y -= 0.1f; break;
                    case SDLK_s: cam_y += 0.1f; break;
                    case SDLK_a: cam_x -= 0.1f; break;
                    case SDLK_d: cam_x += 0.1f; break;
                    default: break;
                }
            }
        }

        SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 0, 0, 0));
        for (int i = 0; i < 800 * 600; i++) zbuffer[i] = 1e30f;

        Uint32 color_blanco    = SDL_MapRGB(surface->format, 255, 255, 255);
        float  distancia_focal = 300.0f;
        float  centro_x        = 400.0f;
        float  centro_y        = 300.0f;

        angulo += velocidad;
        float s = sinf(angulo);
        float c = cosf(angulo);

        for (int i = 0; i < num_caras; i++) {
            Vec3 v1 = vertices[caras[i].a];
            Vec3 v2 = vertices[caras[i].b];
            Vec3 v3 = vertices[caras[i].c];

            float ox;
            ox = v1.x; v1.x = ox * c - v1.z * s; v1.z = ox * s + v1.z * c;
            ox = v2.x; v2.x = ox * c - v2.z * s; v2.z = ox * s + v2.z * c;
            ox = v3.x; v3.x = ox * c - v3.z * s; v3.z = ox * s + v3.z * c;

            v1.z += 3.0f; v2.z += 3.0f; v3.z += 3.0f;

            if (v1.z > 0 && v2.z > 0 && v3.z > 0) {
                int px1 = (int)(v1.x * distancia_focal / v1.z + centro_x + cam_x * 100);
                int py1 = (int)(v1.y * distancia_focal / v1.z + centro_y + cam_y * 100);
                int px2 = (int)(v2.x * distancia_focal / v2.z + centro_x + cam_x * 100);
                int py2 = (int)(v2.y * distancia_focal / v2.z + centro_y + cam_y * 100);
                int px3 = (int)(v3.x * distancia_focal / v3.z + centro_x + cam_x * 100);
                int py3 = (int)(v3.y * distancia_focal / v3.z + centro_y + cam_y * 100);

                rellenar_triangulo(surface, zbuffer,
                    px1, py1, v1.z,
                    px2, py2, v2.z,
                    px3, py3, v3.z,
                    colores[(i / 2) % 6]);
                dibujar_linea(surface, px1, py1, px2, py2, color_blanco);
                dibujar_linea(surface, px2, py2, px3, py3, color_blanco);
                dibujar_linea(surface, px3, py3, px1, py1, color_blanco);
            }
        }

        SDL_UpdateWindowSurface(window);

        frames++;
        Uint32 tiempo_actual = SDL_GetTicks();
        if (tiempo_actual - tiempo_anterior >= 1000) {
            fps = (float)frames / ((tiempo_actual - tiempo_anterior) / 1000.0f);
            char titulo[64];
            sprintf(titulo, "Visor CPU 3D | FPS: %.1f", fps);
            SDL_SetWindowTitle(window, titulo);
            printf("FPS %.1f\n", fps);
            frames = 0;
            tiempo_anterior = tiempo_actual;
        }
    }

    free(vertices);
    free(caras);
    free(zbuffer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

