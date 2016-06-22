A software rasterizer.

This is my first stab at writing a library for software rasterization.
It is far from ready in its current form, but it works and can be modified as needed.
The rasterization routing expects a 2D array of pixels to be rendered onto, and a list of triangles to be
rasterized.

Currently everything lives in main.c and types.h.
Eventually I would like to build this into a single-header static library like those maintained by Sean Barrett.

# Dependencies
- gcc
- kdbg
- sdl2

# Setup

    cd /path/to/project
    source env/shell

# Building

    build

# Running

    run triangles.def

# Debugging

    debug triangles.def &

# Developer Information

## Rasterization Outline
Given a polygon in viewspace ((0,0), (640,480)) we want to rasterize the polygon
onto our viewspace buffer.
At this point we can skip our modelview and projection matrices.
We need a polygon rasterization algorithm, whether that directly involves line
drawing or not.
See here: http://www.sccs.swarthmore.edu/users/05/zap/labs/E26/lab7/index.html
(We will also probably need a z-buffer once we start dealing with more than one object.)

### Rasteriazation Algorithm

- Convert each polygon into a list of edges. (Line segments.)
- Sort all polygons front to back (ignore transparency support for now).
- For each scanline keep track of which polygon has been intersected via the scanline stack.
- This requires a line-line intersection algorithm in view space.
- For now just use a solid, unique color for each individual polygon.

We'll have a list of polygons and divide the video buffer into lines.
For each line that we'll rasterize, we want to subdivide it into line segments corresponding with
polygons.  So we'll need a stack representing the current polygon we're on.
For every polygon we encounter in the stack while rasterizing, we need to do a per-pixel material
calculation for that polygon. Ie., lighting + shading + textures + bump/parallalax.

## Rasterization Limitations

At this point (2016-06-21) this program only handles scanline rasterization.
The assumption is that we have a collection of adjacent, non-overlapping
triangles in screen space.  These triangles are the end result of the 3D
rendering engine and no longer need Z-depth.

# To Do

- Fix rasterization bug. (See horizontal yellow line in rendering.)
- Improve style consistency in codebase.
- Improve APIs; define an actual API. (Think about usage!)
- Define materials beyond simple colors.
- Interpolate materials across triangle surfaces.

**NOTE**: I'm not actually entirely clear on how real rasterizers integrate with
rendering engines and what information they have for rasterization.
I am assuredly conflating 3D transformations, rendering and rasterization to
various extents and should do some more study to sort that out.