/*
 * bench_zbuffer.c
 * Compara la limpieza del zbuffer con loop simple vs SSE4.1 inline ASM.
 * Ejecuta cada método 10 000 veces sobre un buffer de 800x600 floats y
 * reporta el tiempo total y por iteración.
 *
 * Compile:  gcc -O2 -msse4.1 -o bench test/bench_zbuffer.c -lm
 * Run:      ./bench
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <smmintrin.h>  // instrucciones para sse4.1

#define W       800
#define H       600
#define N       (W * H)
#define RONDAS  10000

/* ----------------------------------------------------------
 * Método 1 — loop escalar (baseline)
 * ---------------------------------------------------------- */
static void limpiar_escalar(float *zbuffer, int n) {
    for (int i = 0; i < n; i++)
        zbuffer[i] = 1e30f;
}

/* ----------------------------------------------------------
 * Método 2 — SSE4.1 inline ASM
 * vbroadcastss: carga el float val en los 4 lanes de xmm0
 * movups:       escribe los 4 floats a memoria sin alinear
 * 4 floats × 4 bytes = 16 bytes por iteración
 * ---------------------------------------------------------- */
static void limpiar_sse4_asm(float *zbuffer, int n) {
    float val = 1e30f;
    int iteraciones = n / 4;
    __asm__ volatile (
        "movss %2, %%xmm0\n\t"          // carga val en el lane bajo de xmm0
        "shufps $0, %%xmm0, %%xmm0\n\t" // copia el lane 0 a los 4 lanes: [val,val,val,val]
        "1:\n\t"
        "movups %%xmm0, (%0)\n\t"        // escribe 4 floats en *zbuffer
        "add $16, %0\n\t"               // avanza 16 bytes
        "dec %1\n\t"                    // iteraciones--
        "jnz 1b\n\t"                    // loop hasta 0
        : "+r"(zbuffer), "+r"(iteraciones)
        : "m"(val)
        : "xmm0", "memory"
    );
}

/* ----------------------------------------------------------
 * Mide ns promedio de una función sobre RONDAS iteraciones
 * ---------------------------------------------------------- */
static double medir_ns(void (*fn)(float *, int), float *buf, int n) {
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int r = 0; r < RONDAS; r++)
        fn(buf, n);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double total_ns = (t1.tv_sec - t0.tv_sec) * 1e9 + (t1.tv_nsec - t0.tv_nsec);
    return total_ns / RONDAS;
}

int main(void) {
    float *zbuffer = aligned_alloc(16, N * sizeof(float));
    if (!zbuffer) { fprintf(stderr, "alloc failed\n"); return 1; }

    printf("Buffer: %dx%d = %d floats = %.1f KB\n\n", W, H, N, N * 4.0 / 1024);

    double ns_escalar = medir_ns(limpiar_escalar,  zbuffer, N);
    double ns_sse4    = medir_ns(limpiar_sse4_asm, zbuffer, N);

    printf("%-20s %8.0f ns/iter\n", "loop escalar:",  ns_escalar);
    printf("%-20s %8.0f ns/iter\n", "SSE4 asm:",      ns_sse4);
    printf("\nSpeedup SSE4 / escalar: %.2fx\n", ns_escalar / ns_sse4);

    free(zbuffer);
    return 0;
}
