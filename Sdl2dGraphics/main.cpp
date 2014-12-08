#include "SDL.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "types.h"

/*
TODO(AARON):
Given a polygon in viewspace ((0,0), (640,480)) we want to rasterize the polygon
onto our viewspace buffer.
At this point we can skip our modelview and projection matrices.
We need a polygon rasterization algorithm, whether that directly involves line
drawing or not.
See here: http://www.sccs.swarthmore.edu/users/05/zap/labs/E26/lab7/index.html
(We will also probably need a z-buffer once we start dealing with more than one object.)

Scanline Algorithm
------------------

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
*/

triangle_edges
EdgesFor(triangle Triangle) {
  triangle_edges Edges;
  Edges.Edges[0].X  = (float)Triangle.Point[0].X;
  Edges.Edges[0].Y  = (float)Triangle.Point[0].Y;
  Edges.Edges[0].Dx = (float)Triangle.Point[1].X;
  Edges.Edges[0].Dy = (float)Triangle.Point[1].Y;
  Edges.Edges[1].X  = (float)Triangle.Point[1].X;
  Edges.Edges[1].Y  = (float)Triangle.Point[1].Y;
  Edges.Edges[1].Dx = (float)Triangle.Point[2].X;
  Edges.Edges[1].Dy = (float)Triangle.Point[2].Y;
  Edges.Edges[2].X  = (float)Triangle.Point[2].X;
  Edges.Edges[2].Y  = (float)Triangle.Point[2].Y;
  Edges.Edges[2].Dx = (float)Triangle.Point[0].X;
  Edges.Edges[2].Dy = (float)Triangle.Point[0].Y;
  return Edges;
}

ray2d
PositiveXVectorAtHeight(int Height) {
  ray2d Result;
  Result.X = 0;
  Result.Y = (float)Height;
  Result.Dx = 1;
  Result.Dy = 0;
  return Result;
}

// Read here: http://www.gamedev.net/topic/528210-2d-ray-to-line-intersection-test/#entry4423493
//
intersection_point
Intersect(ray2d Ray, line_segment LineSegment) {
  intersection_point Miss = {};

  real32 C = LineSegment.Dy * LineSegment.X - LineSegment.Dx * LineSegment.Y;
  real32 Denominator = -LineSegment.Dy*Ray.Dx + LineSegment.Dx*Ray.Dy;

  if (Denominator == 0) return Miss;

   real32    Numerator = -(LineSegment.Dy)*Ray.X + LineSegment.Dx*Ray.Y + C;
   real32            T = -(Numerator / Denominator);
  point2f Intersection =   Evaluate(Ray, T);
   real32           T0 =  (Intersection.X - LineSegment.X) / LineSegment.Dx;

  if (T0 > 1 || T0 < 0) return Miss;

  intersection_point Result;
  Result.IsIntersection = true;
  Result.Intersection = Intersection;
  return Result;
}

int
ScanlineIntersectionSort(const void* Left, const void* Right) {
  scanline_intersection First  = *((scanline_intersection*)Left);
  scanline_intersection Second = *((scanline_intersection*)Right);

  if (First.X > Second.X) return 1;
  if (First.X < Second.X) return -1;
  return 0;
}

void
GenerateScanlines(triangle* Triangles, int NumTriangles, scanline* Scanlines) {
  for (int h = 0; h < DISPLAY_HEIGHT; h++) {
    scanline* Scanline = &Scanlines[h];
    Scanline->NumIntersections = 0;

    ray2d Ray = PositiveXVectorAtHeight(h);

    // NOTE(AARON):
    // Triangles should be sorted front-to-back.
    for (int t = 0; t < NumTriangles; t++) {
      triangle *Triangle = &Triangles[t];
      triangle_edges Edges = EdgesFor(*Triangle);

      for (int i=0; i < 3; ++i) {
        line_segment Edge = FromPoints(Edges.Edges[i].Start, Edges.Edges[i].End);
        intersection_point Hit = Intersect(Ray, Edge);

        if (Hit.IsIntersection) {
          scanline_intersection* Intersection = &Scanline->Intersections[Scanline->NumIntersections];
          Intersection->Triangle = Triangle;
          Intersection->X = (int)Hit.Intersection.X;
          Scanline->NumIntersections++;
        }
      }
    }

    qsort(Scanline->Intersections,
          Scanline->NumIntersections,
          sizeof(scanline_intersection),
          ScanlineIntersectionSort);
  }
}

void
Rasterize(int* DisplayBuffer, scanline* Scanlines) {
  // NOTE(AARON):
  // This is a local stack implementation to track what polygon we are currently
  // rendering.
  triangle* HitStack[DISPLAY_WIDTH];
  triangle* Empty = HitStack[0];
  triangle* Next = Empty;

  // Basic algorithm:
  // Iterate through the columns of every display row:
  //   If the current column matches with an intersection of a polygon:
  //     Get the polygon
  //     See if we are colliding, or ending the collision.
  //     If we are colliding, then set a new material (on the material stack)
  //     If we are ending collision, then pop the material (off of the material stack).

  for (int i=0; i<ArrayCount(Scanlines); ++i) {
    scanline* Scanline = &Scanlines[i];
    for (int x=0; x<Scanline->NumIntersections; ++x) {
      // TODO(AARON): Rasterizing goes here.
    }
  }
}

void
PutPixel(int* DisplayBuffer, int X, int Y, int Pixel) {
  DisplayBuffer[(Y * DISPLAY_WIDTH) + X] = Pixel;
}

global_variable scanline Scanlines[DISPLAY_HEIGHT];

int
main(int argc, char** argv) {
  SDL_Window* Window;
  SDL_Renderer* Renderer;
  SDL_Texture* Texture;
  int Pitch = 1;
  int* DisplayBuffer;

  triangle Triangle = {};
  Triangle.Point[0].X = 100;
  Triangle.Point[0].Y = 200;
  Triangle.Point[1].X = 200;
  Triangle.Point[1].Y = 200;
  Triangle.Point[2].X = 150;
  Triangle.Point[2].Y = 100;

  DisplayBuffer = (int*)malloc(DISPLAY_WIDTH * DISPLAY_HEIGHT * 4);

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS) != 0) {
    fprintf(stderr, "\nUnable to initialize SDL: %s\n", SDL_GetError());
    return 1;
  }

  Window = SDL_CreateWindow("My Awesome Window", 100, 100, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0);
  if (Window == NULL) {
    fprintf(stderr, "\nCouldn't create window: %s\n", SDL_GetError());
    return 1;
  }

  Renderer = SDL_CreateRenderer(Window, -1, SDL_RENDERER_SOFTWARE);
  if (Renderer == NULL) {
    fprintf(stderr, "\nCouldn't create renderer: %s\n", SDL_GetError());
    return 1;
  }

  Texture = SDL_CreateTexture(Renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, DISPLAY_WIDTH, DISPLAY_HEIGHT);
  if (Texture == NULL) {
    fprintf(stderr, "\nCouldn't create texture: %s\n", SDL_GetError());
    return 1;
  }

  SDL_LockTexture(Texture, NULL, (void**)&DisplayBuffer, &Pitch);

  GenerateScanlines(&Triangle, 1, Scanlines);

  int h = 0, w = 0;
  for (h = 0; h < DISPLAY_HEIGHT; h++) {
    for (w = 0; w < DISPLAY_WIDTH; w++) {
      int Color = COLOR_OPAQUE;

      if (w < 320 && h < 240) {
        Color |= COLOR_BLUE;
      }
      else if (w > 320 && h < 240) {
        Color |= COLOR_GREEN;
      }
      else if (w < 320 && h > 240) {
        Color |= COLOR_RED;
      }
      else if (w > 320 && h > 240) {
        Color = COLOR_RED | COLOR_GREEN | COLOR_OPAQUE;
      }

      PutPixel(DisplayBuffer, w, h, Color);
    }
  }

  SDL_UnlockTexture(Texture);

  while(true) {
    SDL_Event e;
    if (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT) {
        break;
      }
    }

    SDL_RenderClear(Renderer);
    SDL_RenderCopy(Renderer, Texture, 0, 0);
    SDL_RenderPresent(Renderer);
  }

  SDL_DestroyTexture(Texture);
  SDL_DestroyRenderer(Renderer);
  SDL_DestroyWindow(Window);
  SDL_Quit();

  return 0;
}
