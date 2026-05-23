#include "loader.h"
#include <stdio.h>
#include <stdlib.h>

int cargar_obj(char *ruta) {
    FILE *archivo = fopen(ruta, "r");
    if (archivo == NULL) {
        printf("No se pudo abrir el archivo\n");
        return 1;
    }

    char linea[1024];
    while (fgets(linea, sizeof(linea), archivo) != NULL) {
        float x, y, z;
        int a, b, c;

        if (linea[0] == 'v' && linea[1] == ' ') {
            sscanf(linea, "v %f %f %f", &x, &y, &z);
            num_vertices++;
            vertices = realloc(vertices, num_vertices * sizeof(Vec3));
            vertices[num_vertices - 1].x = x;
            vertices[num_vertices - 1].y = y;
            vertices[num_vertices - 1].z = z;
        }
        if (linea[0] == 'f' && linea[1] == ' ') {
            sscanf(linea, "f %d %d %d", &a, &b, &c);
            num_caras++;
            caras = realloc(caras, num_caras * sizeof(Cara));
            caras[num_caras - 1].a = a - 1;
            caras[num_caras - 1].b = b - 1;
            caras[num_caras - 1].c = c - 1;
        }
    }

    fclose(archivo);
    printf("Cargados %d vertices y %d caras\n", num_vertices, num_caras);
    return 0;
}
