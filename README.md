A software rasterizer.

This is my first stab at writing a library for software rasterization.
It is far from ready in its current form, but it works and can be modified as needed.
The rasterization routing expects a 2D array of pixels to be rendered onto, and a list of triangles to be
rasterized.

Currently everything lives in main.c and types.h.
Eventually I would like to build this into a single-header static library like those maintained by Sean Barrett.

# Dependencies
- gcc
- nemiver (for 'debug' command)
- sdl2

# Setup

    cd $PROJECT_PATH
    source env/shell

# Building

    build

## Linux

    run

# Debugging

    debug
