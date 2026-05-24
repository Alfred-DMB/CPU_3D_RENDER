# Visor 3D CPU

A software 3D viewer written in C using SDL2, built entirely on the CPU — no GPU, no OpenGL, no Vulkan.

> Built for fun and to learn how 3D rendering actually works from scratch, without any graphics API doing the heavy lifting.

## What it does

- Loads `.obj` files (vertices and faces)
- Rotates the model in real time
- Renders filled triangles with a z-buffer (depth test)
- Draws wireframe edges on top
- Perspective projection
- FPS counter in the window title
- Camera panning with WASD

## Performance

Tested on Pentium Gold (x86-64, SSE4.1, no AVX), single core, uncapped render loop:

| Model | FPS | CPU (1 core) | RAM |
|---|---|---|---|
| Cube | ~350 fps | ~100% | ~85 MB |
| Utah teapot | ~60 fps | ~100% | ~85 MB |

Capped at 60 FPS with `SDL_Delay`, CPU drops to ~30-44%. The ~85 MB is mostly SDL2's own window/display buffers; geometry and z-buffer are a few MB at most.

---

## SSE4 optimizations

> This is my first time writing x86 assembly / SSE intrinsics.

The z-buffer must be cleared to `1e30f` (a large depth value) every frame — that's 480,000 floats (~1.9 MB) written on every tick. Instead of a plain `for` loop, the clear is done with SSE2 instructions using a 128-bit register (`xmm0`) to write **4 floats per cycle**.

**What the code does, step by step:**

```asm
movss val, xmm0          ; load the float into xmm0's lowest lane
shufps xmm0, xmm0, 0    ; copy lane 0 to all 4 lanes → [val, val, val, val]
loop:
  movups xmm0, [zbuffer] ; write 4 floats (16 bytes) to memory
  add zbuffer, 16        ; advance pointer by 4 floats
  dec iteraciones
  jnz loop
```

> Note: `vbroadcastss` (AVX) was tried first but the Pentium Gold only supports up to SSE4.1 — no AVX. The correct way to broadcast a scalar to all 4 float lanes in SSE2 is `movss` + `shufps $0`.

### Benchmark results — z-buffer clear (800×600, 10 000 iterations)

| Method | -O0 (no compiler opt) | -O2 (compiler auto-vectorizes) |
|---|---|---|
| Scalar `for` loop | 1304 µs | 534 µs |
| SSE2 inline ASM | 453 µs | 439 µs |
| **Speedup** | **2.88×** | **1.22×** |

With `-O2` the compiler already auto-vectorizes the scalar loop, so the manual ASM only adds ~20% on top. Without optimization (`-O0`) the advantage is closer to 3×, which reflects what the processor is actually doing differently.

Benchmark source: `test/bench_zbuffer.c`

## Why CPU only

No GPU was available during development. Everything — rasterization, z-buffer, triangle fill, Bresenham line drawing — runs on the CPU using SDL2 surfaces and raw pixel writes.

## Build

```bash
make
```

Or manually:

```bash
gcc -Wall -Wextra -Iinclude -o visor src/main.c src/loader.c src/renderer.c -lSDL2 -lm
```

## Usage

Place a `.obj` file named `modelo.obj` inside the `assets/` folder and run:

```bash
./visor
```

### Controls

| Key | Action |
|---|---|
| `→` / `Numpad +` | Increase rotation speed |
| `←` / `Numpad -` | Decrease rotation speed |
| `0` | Pause rotation |
| `W A S D` | Pan camera |
| `Escape` | Quit |

## Project structure

```
3d projecto/
  src/
    main.c        — SDL setup, render loop, input handling
    loader.c      — .obj file parser
    renderer.c    — Bresenham line, z-buffer, triangle rasterizer
  include/
    types.h       — Vec3, Cara structs and global declarations
    loader.h
    renderer.h
  assets/
    modelo.obj    — 3D model to render
  Makefile
  README.md
```

## Dependencies

- [SDL2](https://www.libsdl.org/)
- Standard C math library (`-lm`)
