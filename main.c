#include "SDL.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "types.h"

/*
  NOTE(AARON):
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

global_variable scanline Scanlines[DISPLAY_HEIGHT];

triangle
OrderForRaster(triangle Unordered) {
        // Counter-clockwise.
        // Bottom-left starting.

        triangle Result;

        if (Unordered.A.Y > Unordered.B.Y && Unordered.A.Y > Unordered.C.Y) {
                return(Unordered);
        }
        else if (Unordered.B.Y > Unordered.A.Y && Unordered.B.Y > Unordered.C.Y) {
                Result.A = Unordered.B;
                Result.B = Unordered.C;
                Result.C = Unordered.A;
        }
        else {
                Result.A = Unordered.C;
                Result.B = Unordered.A;
                Result.C = Unordered.B;
        }
        return(Result);
}

triangle_edges
FromTriangle(triangle Triangle) {
        triangle_edges Result;

        Result.Edges[0].Start = Triangle.Point[0];
        Result.Edges[0].End = Triangle.Point[1];

        Result.Edges[1].Start = Triangle.Point[1];
        Result.Edges[1].End = Triangle.Point[2];

        Result.Edges[2].Start = Triangle.Point[2];
        Result.Edges[2].End = Triangle.Point[0];

        return(Result);
}

ray2d
PositiveXVectorAtHeight(int Height) {
        ray2d Result;
        Result.X = 0;
        Result.Y = (real32)Height;
        Result.Dx = 1;
        Result.Dy = 0;
        return(Result);
}

/* This assumes a horizontal +X Vector. */
bool
HasIntersection(ray2d Ray, line_segment Line) {
        int32 A = Line.EndY - Ray.Y;
        int32 B = Line.StartY - Ray.Y;
        return(((A ^ B) < 0) || Line.EndY == Ray.Y || Line.StartY == Ray.Y);
}

real32
Intersect(ray2d Ray, line_segment Line) {
        vector2d Slope = Subtract(Line.End, Line.Start);
        bool IsNegative = (Slope.X < 0 || Slope.Y < 0);

        real32 DeltaY = abs(Line.StartY - Ray.Y);

        real32 Term2 = (DeltaY * (Slope.X / Slope.Y));

        if (IsNegative) {
                return(Line.StartX - Term2);
        }
        else {
                return(Line.StartX + Term2);
        }
}

int
ScanlineIntersectionSort(const void *Left, const void *Right) {
        scanline_intersection First  = *((scanline_intersection *)Left);
        scanline_intersection Second = *((scanline_intersection *)Right);

        if (First.X > Second.X) return(1);
        if (First.X < Second.X) return(-1);
        return(0);
}

void
GenerateScanlines(triangle Triangles[], int NumTriangles, scanline Scanlines[]) {
        for (int h = 0; h < DISPLAY_HEIGHT; h++) {
                scanline *Scanline = &Scanlines[h];
                Scanline->NumIntersections = 0;

                ray2d Ray = PositiveXVectorAtHeight(h);

                for (int t = 0; t < NumTriangles; t++) {
                        triangle *Triangle = &Triangles[t];
                        triangle_edges Edges = FromTriangle(*Triangle);

                        int NumTriangleIntersections = 0;

                        for (int i=0; i < 3; ++i) {
                                line_segment Edge = FromPoints(Edges.Edges[i].Start, Edges.Edges[i].End);

                                if (HasIntersection(Ray, Edge)) {
                                        // NOTE(AARON):
                                        // For a given triangle we expect to only have two intersections, max.
                                        // More than this causes rendering artifacts because any two edges
                                        // probably share a vertex. When a vertex is shared, then then the
                                        // intersection points get recalculated, which will cause problems
                                        // at rasterization.
                                        ++NumTriangleIntersections;
                                        if (NumTriangleIntersections > 2) {
                                                continue;
                                        }

                                        real32 X = Intersect(Ray, Edge);
                                        scanline_intersection *Intersection = &(Scanline->Intersections[Scanline->NumIntersections]);
                                        Intersection->Triangle = Triangle;
                                        Intersection->X = X;
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
PutPixel(int DisplayBuffer[], int X, int Y, int Pixel) {
        DisplayBuffer[(Y * DISPLAY_WIDTH) + X] = Pixel;
}

void
Rasterize(int DisplayBuffer[], scanline Scanlines[], materials Materials) {
        stack CurrentTriangle;
        Init(&CurrentTriangle);

        for (int h=0; h < DISPLAY_HEIGHT; ++h) {
                scanline *Scanline = &Scanlines[h];

                for (int x=0; x<DISPLAY_WIDTH; ++x) {

                        for (int s=0; s<Scanline->NumIntersections; ++s) {
                                scanline_intersection *Intersection = &(Scanline->Intersections[s]);

                                // TODO(AARON):
                                // Will have to take into account the Z-depth to resolve how overdraw works.
                                if (Intersection->X == x) {
                                        triangle *Triangle;

                                        if (Find(&CurrentTriangle, Intersection->Triangle)) {
                                                Remove(&CurrentTriangle, Intersection->Triangle);
                                        }
                                        else {
                                                Push(&CurrentTriangle, Intersection->Triangle);
                                        }

                                }
                        }

                        // NOTE(AARON): This may be a NULL pointer.
                        triangle *Triangle = Top(&CurrentTriangle);
                        int32 Color = COLOR_BLACK;
                        if (Triangle) {
                                Color = FromMaterial(&Materials, Triangle);
                        }

                        PutPixel(DisplayBuffer, x, h, Color);
                }
        }
}

int
main(int argc, char** argv) {
        SDL_Window* Window;
        SDL_Renderer* Renderer;
        SDL_Texture* Texture;
        int DisplayBufferPitch;
        int* DisplayBuffer;

        triangle Triangle1;
        Triangle1.Point[0].X = 100;
        Triangle1.Point[0].Y = 200;
        Triangle1.Point[1].X = 750;
        Triangle1.Point[1].Y = 340;
        Triangle1.Point[2].X = 800;
        Triangle1.Point[2].Y = 640;
        Triangle1 = OrderForRaster(Triangle1);

        triangle Triangle2;
        Triangle2.Point[0].X = 0;
        Triangle2.Point[0].Y = 767;
        Triangle2.Point[1].X = 1020;
        Triangle2.Point[1].Y = 0;
        Triangle2.Point[2].X = 1020;
        Triangle2.Point[2].Y = 767;
        Triangle2 = OrderForRaster(Triangle2);

        int NumTriangles = 2;
        triangle Triangles[] = { Triangle2, Triangle1 };

        materials Materials;
        color Colors[] = { COLOR_RED, COLOR_BLUE };
        InitMaterials(&Materials, Triangles, Colors, NumTriangles);

        DisplayBuffer = (int*)malloc(DISPLAY_WIDTH * DISPLAY_HEIGHT * 4);

        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS) != 0) {
                fprintf(stderr, "\nUnable to initialize SDL: %s\n", SDL_GetError());
                return(1);
        }

        Window = SDL_CreateWindow("My Awesome Window", 100, 100, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0);
        if (Window == NULL) {
                fprintf(stderr, "\nCouldn't create window: %s\n", SDL_GetError());
                return(1);
        }

        Renderer = SDL_CreateRenderer(Window, -1, SDL_RENDERER_SOFTWARE);
        if (Renderer == NULL) {
                fprintf(stderr, "\nCouldn't create renderer: %s\n", SDL_GetError());
                return(1);
        }

        Texture = SDL_CreateTexture(Renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, DISPLAY_WIDTH, DISPLAY_HEIGHT);
        if (Texture == NULL) {
                fprintf(stderr, "\nCouldn't create texture: %s\n", SDL_GetError());
                return(1);
        }

        SDL_LockTexture(Texture, NULL, (void**)&DisplayBuffer, &DisplayBufferPitch);

        GenerateScanlines(Triangles, NumTriangles, Scanlines);
        Rasterize(DisplayBuffer, Scanlines, Materials);

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

        return(0);
}
