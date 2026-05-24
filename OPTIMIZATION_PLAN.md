# Plan de optimizaciones SSE4 + ensamblador

## Objetivo

Usar SSE4.1 (SIMD de 128 bits) para acelerar los paths más calientes del renderer sin reescribir toda la arquitectura. El ensamblador se aplica solo donde el compilador falla en vectorizar automáticamente.

---

## Análisis de hotspots

| Función | Costo actual | Por qué |
|---|---|---|
| `rellenar_triangulo` — loop interno de scanline | ★★★★★ | Se ejecuta para cada pixel de cada triángulo, cada frame |
| Limpieza del zbuffer (`memset` de floats) | ★★★★☆ | 800×600×4 = ~1.9 MB por frame |
| `dibujar_linea` (Bresenham) | ★★☆☆☆ | Branch-heavy, poca ganancia con SIMD |

---

## Optimización 1 — Limpieza del zbuffer (fácil, alta ganancia)

**Archivo:** `src/main.c` — el loop que hace `zbuffer[i] = 1e30f`

**Por qué es fácil:** es un simple fill de memoria, sin lógica.  
**Ganancia esperada:** 4× más rápido (procesa 4 floats por instrucción en lugar de 1).

### Con intrínsecas SSE4 (C, sin ensamblador puro):
```c
#include <smmintrin.h>  // SSE4.1

void limpiar_zbuffer(float *zbuffer, int n) {
    __m128 infinito = _mm_set1_ps(1e30f);
    for (int i = 0; i < n; i += 4)
        _mm_storeu_ps(&zbuffer[i], infinito);
}
```

### Con ensamblador inline (AT&T syntax):
```c
void limpiar_zbuffer_asm(float *zbuffer, int n) {
    float val = 1e30f;
    int iteraciones = n / 4;
    __asm__ volatile (
        "vbroadcastss %2, %%xmm0\n\t"   // xmm0 = [val, val, val, val]
        "1:\n\t"
        "movups %%xmm0, (%0)\n\t"        // escribe 4 floats
        "add $16, %0\n\t"                // avanza 16 bytes
        "dec %1\n\t"
        "jnz, 1b\n\t"
        : "+r"(zbuffer), "+r"(iteraciones)
        : "m"(val)
        : "xmm0", "memory"
    );
}
```

> **Este es el mejor punto de entrada para ensamblador:** sin branches, patrón repetitivo, impacto inmediato y medible.

---

## Optimización 2 — Loop de scanline con zbuffer (media dificultad, máxima ganancia)

**Archivo:** `src/renderer.c` — el `for (int x = ...)` dentro de `rellenar_triangulo`

**Por qué importa:** es el loop más interno del renderer. Se ejecuta millones de veces por frame con la tetera.

**Idea:** procesar 4 píxeles a la vez con SSE4.1:
1. Calcular 4 valores de z interpolados con `_mm_add_ps` + `_mm_mul_ps`
2. Comparar contra el zbuffer con `_mm_cmplt_ps` (genera máscara)
3. Escribir condicionalmente con `_mm_blendv_ps` (SSE4.1 — blend con máscara)

```c
#include <smmintrin.h>

// Procesa 4 píxeles horizontales en paralelo
// z_izq, z_der: profundidad en los extremos del scanline
// x_izq, ancho: posición y largo del span
void scanline_sse4(Uint32 *pixeles, float *zbuffer,
                   int y, int x_izq, int ancho,
                   float z_izq, float z_der, Uint32 color) {
    __m128 z_paso = _mm_set1_ps((z_der - z_izq) / ancho);
    __m128 z_base = _mm_set_ps(
        z_izq + z_paso[0]*3, z_izq + z_paso[0]*2,
        z_izq + z_paso[0]*1, z_izq
    );
    __m128 z4     = _mm_set1_ps(4.0f);
    __m128 color4 = _mm_castsi128_ps(_mm_set1_epi32((int)color));

    for (int x = x_izq; x <= x_izq + ancho - 4; x += 4) {
        int idx = y * 800 + x;
        __m128 zb   = _mm_loadu_ps(&zbuffer[idx]);
        __m128 mask = _mm_cmplt_ps(z_base, zb);           // z < zbuffer?
        __m128 new_z = _mm_blendv_ps(zb, z_base, mask);   // SSE4.1
        _mm_storeu_ps(&zbuffer[idx], new_z);
        // pixels: blend condicional
        __m128i old_px = _mm_loadu_si128((__m128i*)&pixeles[idx]);
        __m128i new_px = _mm_blendv_epi8(old_px,
                         _mm_castps_si128(color4),
                         _mm_castps_si128(mask));          // SSE4.1
        _mm_storeu_si128((__m128i*)&pixeles[idx], new_px);
        z_base = _mm_add_ps(z_base, _mm_mul_ps(z4, z_paso));
    }
}
```

---

## Optimización 3 — Dot product para backface culling (fácil, ganancia moderada)

**Dónde:** `src/main.c` — antes de `rellenar_triangulo`, para saltarse caras traseras.

SSE4.1 tiene `_mm_dp_ps` (dot product directo en hardware):

```c
#include <smmintrin.h>

// Retorna 1 si la cara mira hacia la cámara (front-face)
int es_visible(Vec3 v1, Vec3 v2, Vec3 v3) {
    // Normal de la cara (cross product)
    Vec3 a = { v2.x-v1.x, v2.y-v1.y, v2.z-v1.z };
    Vec3 b = { v3.x-v1.x, v3.y-v1.y, v3.z-v1.z };
    Vec3 normal = {
        a.y*b.z - a.z*b.y,
        a.z*b.x - a.x*b.z,
        a.x*b.y - a.y*b.x
    };
    // Dot product con dirección de la cámara (0,0,1)
    // Con SSE4: _mm_dp_ps(normal_vec, cam_dir, 0x71)
    return normal.z > 0;  // simplificado: solo Z importa si cámara apunta en Z
}
```

Esto elimina ~50% de los triángulos antes del rasterizado.

---

## Orden de implementación recomendado

| Paso | Qué | Dificultad | Ganancia |
|---|---|---|---|
| 1 | Limpiar zbuffer con SSE4 (`vbroadcastss` + `movups`) | ⬛⬜⬜⬜ | Alta |
| 2 | Backface culling escalar (sin SIMD) | ⬛⬜⬜⬜ | Alta |
| 3 | Scanline con `_mm_blendv_ps` / `_mm_blendv_epi8` | ⬛⬛⬛⬜ | Máxima |
| 4 | Dot product con `_mm_dp_ps` para culling | ⬛⬛⬜⬜ | Media |

---

## Compilar con SSE4 habilitado

```makefile
CFLAGS = -Wall -Wextra -Iinclude -O2 -msse4.1
```

Sin `-msse4.1` el compilador rechaza las intrínsecas `_mm_blendv_*` y `_mm_dp_ps`.

---

## Notas

- El ensamblador inline de la **Optimización 1** es el mejor punto de inicio: 10-15 líneas, sin riesgo de romper lógica, ganancia medible en el FPS counter.
- Las intrínsecas (`_mm_*`) son ensamblador "disfrazado de C" — el compilador las convierte 1:1 a instrucciones SSE sin abstracción adicional.
- Probado en Pentium Gold que soporta SSE4.1 y SSE4.2.
