#include "SDL.h"
#include <stdio.h>
#include <stdlib.h>
//#include <stdbool.h>

#include "data.h"
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
FromTriangle(triangle Triangle) {
  triangle_edges Result;

  Result.Edges[0].Start = Triangle.Point[0];
  Result.Edges[0].End = Triangle.Point[1];

  Result.Edges[1].Start = Triangle.Point[1];
  Result.Edges[1].End = Triangle.Point[2];

  Result.Edges[2].Start = Triangle.Point[2];
  Result.Edges[2].End = Triangle.Point[0];

  return Result;
}

ray2d
PositiveXVectorAtHeight(int Height) {
  ray2d Result;
  Result.X = 0;
  Result.Y = (real32)Height;
  Result.Dx = 1;
  Result.Dy = 0;
  return Result;
}

/* This assumes a horizontal +X Vector. */
bool
HasIntersection(ray2d Ray, line_segment Line) {
  int32 A = Line.EndY - Ray.Y;
  int32 B = Line.StartY - Ray.Y;
  return ((A ^ B) < 0) || Line.EndY == Ray.Y || Line.StartY == Ray.Y;
}

real32
Intersect(ray2d Ray, line_segment Line) {
  vector2d Slope = Subtract(Line.End, Line.Start);
  bool IsNegative = (Slope.X < 0 || Slope.Y < 0);

  real32 DeltaY = abs(Line.StartY - Ray.Y);

  real32 Term2 = (DeltaY * (Slope.X / Slope.Y));

  if (IsNegative) {
    return Line.StartX - Term2;
  }
  else {
    return Line.StartX + Term2;
  }
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
GenerateScanlines(triangle Triangles[], int NumTriangles, scanline Scanlines[]) {
#define RENDER_HITS 0
#define RENDER_TRIANGLE_POINTS 1
#define RENDER_INTERSECTIONS 1

#if RENDER_TRIANGLE_POINTS != 0
  char* TrianglePointRender = new char[sizeof(ConstData)];
  strcpy(TrianglePointRender, ConstData);
#endif

#if RENDER_HITS != 0
  char* HitRender = new char[sizeof(ConstData)];
  strcpy(HitRender, ConstData);
#endif

#if RENDER_INTERSECTIONS != 0
  char* IntersectionRender = new char[sizeof(ConstData)];
  strcpy(IntersectionRender, ConstData);
#endif

  for (int h = 0; h < DISPLAY_HEIGHT; h++) {
    scanline& Scanline = Scanlines[h];
    Scanline.NumIntersections = 0;

    ray2d Ray = PositiveXVectorAtHeight(h);

    // NOTE(AARON):
    // Triangles should be sorted front-to-back.
    for (int t = 0; t < NumTriangles; t++) {
      triangle& Triangle = Triangles[t];
      triangle_edges Edges = FromTriangle(Triangle);

      for (int i=0; i < 3; ++i) {
        line_segment Edge = FromPoints(Edges.Edges[i].Start, Edges.Edges[i].End);

#if RENDER_TRIANGLE_POINTS != 0
        if (h == 0) {
          printf("Edge %i starts at (%i,%i) and ends at (%i,%i)\n", i, (int)Edge.StartX, (int)Edge.StartY, (int)Edge.EndX, (int)Edge.EndY);
          Plot(TrianglePointRender, 'X', Edge.StartX, Edge.StartY);
        }
#endif

        if (HasIntersection(Ray, Edge)) {
#if RENDER_HITS != 0
          for (int j=0; j < DATA_WIDTH - (DATA_WIDTH_LEAD + 1); j++) {
            Plot(HitRender, 'X', j, h);
          }
#endif
          real32 X = Intersect(Ray, Edge);

          if (X > 100) {
            printf("X is: %i\n", (int)X);
          }

#if RENDER_INTERSECTIONS != 0
          Plot(IntersectionRender, 'X', X, h);
#endif

          scanline_intersection& Intersection = Scanline.Intersections[Scanline.NumIntersections];
          Intersection.Triangle = &Triangle;
          Intersection.X = X;
          Scanline.NumIntersections++;
        }
      }
    }

    qsort(Scanline.Intersections,
          Scanline.NumIntersections,
          sizeof(scanline_intersection),
          ScanlineIntersectionSort);
  }
#if RENDER_TRIANGLE_POINTS != 0
  printf("%s", TrianglePointRender);
  printf("\n\n\n");
#endif
#if RENDER_HITS != 0
  printf("%s", HitRender);
  printf("\n\n\n");
#endif
#if RENDER_INTERSECTIONS != 0
  printf("%s", IntersectionRender);
  printf("\n\n\n");
#endif
}

void
PutPixel(int* DisplayBuffer, int X, int Y, int Pixel) {
  DisplayBuffer[(Y * DISPLAY_WIDTH) + X] = Pixel;
}

void
Rasterize(int* DisplayBuffer, scanline* Scanlines) {
  stack CurrentTriangleStack = {};
  stack CurrentMaterialStack = {};

  // TODO(AARON):
  // ArrayCount(Scanlines) fails.  It returns 8 for some reason.
  // Should be using ArrayCount instead of DISPLAY_HEIGHT, if possible.
  for (int h=0; h < DISPLAY_HEIGHT; ++h) {
    scanline& Scanline = Scanlines[h];

    map TriangleMap = {};
    for (int x=0; x<DISPLAY_WIDTH; x++) {
      for (int x2=0; x2<Scanline.NumIntersections; ++x2) {
        scanline_intersection &Intersection = Scanline.Intersections[x2];
        if (Intersection.X == x) {
          Add(TriangleMap, x, Intersection.Triangle);
        }
      }

      triangle* Triangle;
      if ((Triangle = Lookup(TriangleMap, x))) {
        if ((triangle*)Top(CurrentTriangleStack) == Triangle) {
          Pop(CurrentTriangleStack);
        }
        else {
          Push(CurrentTriangleStack, (void*)&Triangle);
        }
      }

      Triangle = (triangle*)Top(CurrentTriangleStack);
      int32 Color = COLOR_OPAQUE;

      if (Triangle) Color = COLOR_RED;
      if (!Triangle) Color = COLOR_OPAQUE;

      PutPixel(DisplayBuffer, x, h, Color);
    }
  }
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
  Triangle.Point[0].X = 10;
  Triangle.Point[0].Y = 10;
  Triangle.Point[1].X = 55;
  Triangle.Point[1].Y = 17;
  Triangle.Point[2].X = 80;
  Triangle.Point[2].Y = 48;

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

  //SDL_LockTexture(Texture, NULL, (void**)&DisplayBuffer, &Pitch);

  GenerateScanlines(&Triangle, 1, Scanlines);

  // for (int i=0; i<DISPLAY_HEIGHT; i++) {
  //   if (Scanlines[i].NumIntersections > 0) {
  //     printf("Scanline %i has %i intersections.\n", i, Scanlines[i].NumIntersections);
  //   }
  // }

  //Rasterize(DisplayBuffer, Scanlines);

  SDL_UnlockTexture(Texture);

  bool32 Running = true;
  while(Running) {
    SDL_Event Event;

    while(SDL_PollEvent(&Event)) {
      switch(Event.type) {
        case SDL_QUIT: {
          Running = false;
        } break;

        case SDL_KEYDOWN:
        case SDL_KEYUP: {
          SDL_Keycode KeyCode = Event.key.keysym.sym;

          if(Event.key.repeat == 0) {
            if(KeyCode == SDLK_ESCAPE) {
              Running = false;
            }
          }
        } break;
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
