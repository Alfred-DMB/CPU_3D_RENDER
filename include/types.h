#ifndef TYPES_H
#define TYPES_H

typedef struct {
    float x;
    float y;
    float z;
} Vec3;

typedef struct {
    int a;
    int b;
    int c;
} Cara;

extern Vec3 *vertices;
extern int num_vertices;
extern Cara *caras;
extern int num_caras;

#endif
