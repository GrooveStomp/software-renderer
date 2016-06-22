#include "SDL.h"
#include <alloca.h>

#include "util.c"
#include "types.c"

global_variable scanline Scanlines[DISPLAY_HEIGHT];

void
OrderForRaster(triangle *Unordered)
{
        // Counter-clockwise.
        // Bottom-left starting.

        triangle Temp;

        if(Unordered->A.Y > Unordered->B.Y && Unordered->A.Y > Unordered->C.Y)
        {
                ;
        }
        else if(Unordered->B.Y > Unordered->A.Y && Unordered->B.Y > Unordered->C.Y)
        {
                Temp.A = Unordered->B;
                Temp.B = Unordered->C;
                Temp.C = Unordered->A;
                Unordered->A = Temp.A;
                Unordered->B = Temp.B;
                Unordered->C = Temp.C;
        }
        else
        {
                Temp.A = Unordered->C;
                Temp.B = Unordered->A;
                Temp.C = Unordered->B;
                Unordered->A = Temp.A;
                Unordered->B = Temp.B;
                Unordered->C = Temp.C;
        }
}

triangle_edges
FromTriangle(triangle Triangle)
{
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
PositiveXVectorAtHeight(int Height)
{
        ray2d Result;
        Result.X = 0;
        Result.Y = (real32)Height;
        Result.Dx = 1;
        Result.Dy = 0;
        return(Result);
}

/* This assumes a horizontal +X Vector. */
bool
HasIntersection(ray2d Ray, line_segment Line)
{
        int32 A = Line.EndY - Ray.Y;
        int32 B = Line.StartY - Ray.Y;
        return(((A ^ B) < 0) || Line.EndY == Ray.Y || Line.StartY == Ray.Y);
}

real32
Intersect(ray2d Ray, line_segment Line)
{
        vector2d Slope = Subtract(Line.End, Line.Start);
        bool IsNegative = (Slope.X < 0 || Slope.Y < 0);

        real32 DeltaY = abs(Line.StartY - Ray.Y);

        real32 Term2 = (DeltaY * (Slope.X / Slope.Y));

        if(IsNegative)
        {
                return(Line.StartX - Term2);
        }
        else
        {
                return(Line.StartX + Term2);
        }
}

int
ScanlineIntersectionSort(const void *Left, const void *Right)
{
        scanline_intersection First  = *((scanline_intersection *)Left);
        scanline_intersection Second = *((scanline_intersection *)Right);

        if(First.X > Second.X) return(1);
        if(First.X < Second.X) return(-1);

        return(0);
}

void
GenerateScanlines(triangle Triangles[], int NumTriangles, scanline Scanlines[])
{
        for(int h = 0; h < DISPLAY_HEIGHT; h++)
        {
                scanline *Scanline = &Scanlines[h];
                Scanline->NumIntersections = 0;

                ray2d Ray = PositiveXVectorAtHeight(h);

                for(int t = 0; t < NumTriangles; t++)
                {
                        triangle *Triangle = &Triangles[t];
                        triangle_edges Edges = FromTriangle(*Triangle);

                        int NumTriangleIntersections = 0;

                        for(int i=0; i < 3; ++i)
                        {
                                line_segment Edge = FromPoints(Edges.Edges[i].Start, Edges.Edges[i].End);

                                if(HasIntersection(Ray, Edge))
                                {
                                        // NOTE(AARON):
                                        // For a given triangle we expect to only have two intersections, max.
                                        // More than this causes rendering artifacts because any two edges
                                        // probably share a vertex. When a vertex is shared, then then the
                                        // intersection points get recalculated, which will cause problems
                                        // at rasterization.
                                        ++NumTriangleIntersections;
                                        if(NumTriangleIntersections > 2)
                                        {
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
PutPixel(int DisplayBuffer[], int X, int Y, int Pixel)
{
        DisplayBuffer[(Y * DISPLAY_WIDTH) + X] = Pixel;
}

void
Rasterize(int DisplayBuffer[], scanline Scanlines[], materials Materials)
{
        stack CurrentTriangle;
        Init(&CurrentTriangle);

        for(int h=0; h < DISPLAY_HEIGHT; ++h)
        {
                scanline *Scanline = &Scanlines[h];

                for(int x=0; x<DISPLAY_WIDTH; ++x)
                {

                        for(int s=0; s<Scanline->NumIntersections; ++s)
                        {
                                scanline_intersection *Intersection = &(Scanline->Intersections[s]);

                                if(Intersection->X == x)
                                {
                                        triangle *Triangle;

                                        if(Find(&CurrentTriangle, Intersection->Triangle))
                                        {
                                                Remove(&CurrentTriangle, Intersection->Triangle);
                                        }
                                        else
                                        {
                                                Push(&CurrentTriangle, Intersection->Triangle);
                                        }

                                }
                        }

                        // NOTE(AARON): This may be a NULL pointer.
                        triangle *Triangle = Top(&CurrentTriangle);
                        int32 Color = 0x00000000;
                        if(Triangle)
                        {
                                Color = FromMaterial(&Materials, Triangle);
                        }

                        PutPixel(DisplayBuffer, x, h, Color);
                }
        }
}

/******************************************************************************
 * Entrypoint, Interface and File I/O.
 ******************************************************************************/

void
AbortWithMessage(const char *msg)
{
        fprintf(stderr, "%s\n", msg);
        exit(EXIT_FAILURE);
}

void
GetTrianglesFromFile(char *Filename, triangle **Triangles, color **Colors, int *Count)
{
        size_t AllocSize = FileSize(Filename);
        buffer FileContents;

        /* Allocate space on the stack. */
        BufferSet(&FileContents, (char *)alloca(AllocSize), 0, AllocSize);
        if(!CopyFileIntoBuffer(Filename, &FileContents))
        {
                AbortWithMessage("Couldn't copy entire file to buffer");
        }

        /* Determine how many triangles we need to create. */
        int NumTriangles = 0;
        buffer Reader;
        CopyBuffer(&FileContents, &Reader);
        for(int i=0; i<FileContents.Length; i++)
        {
                if(*(Reader.Cursor) == '\n') NumTriangles++;
                Reader.Cursor++;
        }
        *Count = NumTriangles;

        *Triangles = (triangle *)malloc(sizeof(triangle) * NumTriangles);
        *Colors = (color *)malloc(sizeof(color) * NumTriangles);

        for(int i=0; i<NumTriangles; i++)
        {
                triangle *Triangle = &(*Triangles)[i];

                sscanf(FileContents.Cursor, "%f,%f %f,%f %f,%f %x",
                       &(Triangle->Point[0].X),
                       &(Triangle->Point[0].Y),
                       &(Triangle->Point[1].X),
                       &(Triangle->Point[1].Y),
                       &(Triangle->Point[2].X),
                       &(Triangle->Point[2].Y),
                       &(*Colors)[i]);

                OrderForRaster(Triangle);
                BufferNextLine(&FileContents);
        }
}

void
Usage()
{
        printf("Usage: program definitions_file\n");
        printf("  definitions_file: file in current directory defining triangle coordinates.\n");
        printf("                    See: triangles.def.example\n");
        printf("  Specify '-h' or '--help' for this help text.\n");
        exit(EXIT_SUCCESS);
}

int
main(int ArgCount, char** Args)
{
        for(int i=0; i<ArgCount; i++)
        {
                if(StringEqual(Args[i], "-h", StringLength("-h")) ||
                   StringEqual(Args[i], "--help", StringLength("--help")))
                {
                        Usage();
                }
        }
        if(ArgCount != 2) Usage();

        SDL_Window* Window;
        SDL_Renderer* Renderer;
        SDL_Texture* Texture;
        int DisplayBufferPitch;
        int* DisplayBuffer;
        int NumTriangles;

        triangle *Triangles;
        color *Colors;
        GetTrianglesFromFile(Args[1], &Triangles, &Colors, &NumTriangles);
        materials Materials;
        InitMaterials(&Materials, Triangles, Colors, NumTriangles);

        DisplayBuffer = (int*)malloc(DISPLAY_WIDTH * DISPLAY_HEIGHT * 4);

        if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS) != 0)
        {
                AbortWithMessage(SDL_GetError());
        }

        Window = SDL_CreateWindow("My Awesome Window", 100, 100, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0);
        if(Window == NULL) AbortWithMessage(SDL_GetError());

        Renderer = SDL_CreateRenderer(Window, -1, SDL_RENDERER_SOFTWARE);
        if(Renderer == NULL) AbortWithMessage(SDL_GetError());

        Texture = SDL_CreateTexture(Renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, DISPLAY_WIDTH, DISPLAY_HEIGHT);
        if(Texture == NULL) AbortWithMessage(SDL_GetError());

        SDL_LockTexture(Texture, NULL, (void**)&DisplayBuffer, &DisplayBufferPitch);

        GenerateScanlines(Triangles, NumTriangles, Scanlines);
        Rasterize(DisplayBuffer, Scanlines, Materials);

        SDL_UnlockTexture(Texture);

        bool32 Running = true;
        while(Running)
        {
                SDL_Event Event;

                while(SDL_PollEvent(&Event))
                {
                        switch(Event.type)
                        {
                                case SDL_QUIT:
                                {
                                        Running = false;
                                } break;

                                case SDL_KEYDOWN:
                                case SDL_KEYUP:
                                {
                                        SDL_Keycode KeyCode = Event.key.keysym.sym;

                                        if(Event.key.repeat == 0)
                                        {
                                                if(KeyCode == SDLK_ESCAPE)
                                                {
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
