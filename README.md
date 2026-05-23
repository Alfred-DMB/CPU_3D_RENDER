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

Tested on CPU with SSE4 (Pentium Gold):

| Model | FPS | CPU (1 core) | RAM |
|---|---|---|---|
| Cube | ~350 fps | ~100% | ~85 MB |
| Utah teapot | ~60 fps | ~100% | ~85 MB |

The render loop runs uncapped — no vsync, no frame limiter — so it burns one full CPU core to maximize FPS. Capped at 60 FPS with `SDL_Delay`, CPU drops to ~30-44%. The ~85 MB RAM is mostly SDL2's own window and display buffers; the actual geometry and z-buffer are a few MB at most.

No GPU involved — pure software rasterization.

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
