#include "raster.h"
#include <stdlib.h> /* NULL */
#include <alloca.h>

typedef int bool;
#define false 0
#define true !false

#define internal static
#define local_persist static
#define global_variable static

#ifdef GS_GRAPHICS_DEBUG
#define Assert(Expression) if(!(Expression)) {*(int *)0 = 0;}
#else
#define Assert(Expression)
#endif

typedef gs_raster_point2d vector2d;

struct ray2d
{
        union
        {
                struct
                {
                        gs_raster_point2d Pos;
                        gs_raster_point2d Dir;
                };
                struct
                {
                        float X;
                        float Y;
                        float Dx;
                        float Dy;
                };
        };
};
typedef struct ray2d ray2d;

struct line_segment
{
        union
        {
                struct
                {
                        gs_raster_point2d Start;
                        gs_raster_point2d End;
                };
                struct
                {
                        float StartX;
                        float StartY;
                        float EndX;
                        float EndY;
                };
        };
};
typedef struct line_segment line_segment;
typedef line_segment edge;

struct gs_raster_triangle_edges
{
        union
        {
                edge Edges[3];
                struct
                {
                        edge AB;
                        edge BC;
                        edge CA;
                };
        };
};
typedef struct gs_raster_triangle_edges gs_raster_triangle_edges;

struct gs_raster_triangle_intersection
{
        unsigned int X;
        gs_raster_triangle *Triangle;
};
typedef struct gs_raster_triangle_intersection gs_raster_triangle_intersection;

int
GsRasterSizeRequiredForScanlines(int NumScanlines, int Capacity)
{
        int Result = (sizeof(gs_raster_scanline) + Capacity) * NumScanlines;
        return(Result);
}

/* Memory can be NULL.  If NULL, allocate on the heap. If not NULL, then use
   Memory. */
void
GsRasterInitScanlines(gs_raster_scanline **Scanlines, int NumScanlines, int Capacity, void *Memory)
{
        if(Memory == NULL)
        {
                int Size = GsRasterSizeRequiredForScanlines(NumScanlines, Capacity);
                Memory = malloc(Size);
        }
        *Scanlines = (gs_raster_scanline *)Memory;

        for(int Index = 0; Index < NumScanlines; Index++)
        {
                gs_raster_scanline *CurrentScanline = &(*Scanlines)[Index];
                CurrentScanline->Capacity = Capacity;
                CurrentScanline->NumIntersections = 0;

                int SizeOfScanlines = sizeof(gs_raster_scanline) * NumScanlines;
                char *Intersections = (char *)Memory + SizeOfScanlines;
                int Offset = Capacity * Index;

                CurrentScanline->Intersections = (gs_raster_triangle_intersection *)(Intersections + Offset);
        }
}

struct gs_raster_triangle_stack
{
        gs_raster_triangle **Stack;
        int Head;
};
typedef struct gs_raster_triangle_stack gs_raster_triangle_stack;

//------------------------------------------------------------------------------
// Material Operations
//------------------------------------------------------------------------------

gs_raster_color
ColorForTriangle(gs_raster_triangle *Triangles, gs_raster_color *Colors, int Count, gs_raster_triangle *Triangle)
{
        for(int Index = 0; Index < Count; Index++)
        {
                if(&Triangles[Index] == Triangle)
                {
                        return(Colors[Index]);
                }
        }

        /* TODO(AARON): Default to fuchsia or some obviously "wrong" color? */
        return(0x00000000); /* Default to the color black. */
}

//------------------------------------------------------------------------------
// Triangle Stack Operations
//------------------------------------------------------------------------------

/* Memory can be NULL.  If NULL, allocate on the heap. If not NULL, then use
   Memory. */
void
TriangleStackInit(gs_raster_triangle_stack **Stack, int Capacity, void *Memory)
{
        if(Memory == NULL)
        {
                int PointerArraySize = sizeof(gs_raster_triangle *) * Capacity;
                Memory = malloc(sizeof(gs_raster_triangle_stack) + PointerArraySize);
        }
        *Stack = (gs_raster_triangle_stack *)Memory;

        gs_raster_triangle_stack *StackPointer = *Stack;
        void *MemoryOffset = (char *)Memory + sizeof(gs_raster_triangle_stack);
        StackPointer->Stack = (gs_raster_triangle **)MemoryOffset;
        StackPointer->Head = 0;
}

bool
TriangleStackIsEmpty(gs_raster_triangle_stack *Stack)
{
        return(Stack->Head == 0);
}

unsigned int
TriangleStackSize(gs_raster_triangle_stack *Stack)
{
        return(Stack->Head);
}

bool
TriangleStackPush(gs_raster_triangle_stack *Stack, gs_raster_triangle *Object)
{
        ++Stack->Head;
        Stack->Stack[Stack->Head] = Object;
        return(true);
}

gs_raster_triangle *
TriangleStackPop(gs_raster_triangle_stack *Stack)
{
        if(Stack->Head <= 0) return(NULL);

        gs_raster_triangle *Result = Stack->Stack[Stack->Head];
        --Stack->Head;

        return(Result);
}

gs_raster_triangle *
TriangleStackTop(gs_raster_triangle_stack *Stack)
{
        return(Stack->Stack[Stack->Head]);
}

bool
TriangleStackFind(gs_raster_triangle_stack *Stack, gs_raster_triangle *Object)
{
        for(int i=0; i<=Stack->Head; ++i)
        {
                if(Stack->Stack[i] == Object)
                {
                        return(true);
                }
        }

        return(false);
}

bool
TriangleStackRemove(gs_raster_triangle_stack *Stack, gs_raster_triangle *Object)
{
        int index = -1;
        for(int i=0; i <= Stack->Head; ++i)
        {
                if(Stack->Stack[i] == Object)
                {
                        index = i;
                        break;
                }
        }

        if(index < 0)
        {
                return(false);
        }

        for(int i=index; i < Stack->Head; ++i)
        {
                Stack->Stack[i] = Stack->Stack[i+1];
        }
        --Stack->Head;

        return(true);
}

//------------------------------------------------------------------------------
// Ray, Point and other similar operations
//------------------------------------------------------------------------------

gs_raster_point2d
Add(gs_raster_point2d First, gs_raster_point2d Second)
{
        gs_raster_point2d Result;
        Result.X = First.X + Second.X;
        Result.Y = First.Y + Second.Y;
        return(Result);
}

gs_raster_point2d
Apply(gs_raster_point2d Point, float T)
{
        gs_raster_point2d Result;
        Result.X = Point.X * T;
        Result.Y = Point.Y * T;
        return(Result);
}

gs_raster_point2d
Evaluate(ray2d Ray, float T)
{
        vector2d Applied = Apply(Ray.Dir, T);
        gs_raster_point2d Result = Add(Ray.Pos, Applied);
        return(Result);
}

gs_raster_point2d
Subtract(gs_raster_point2d Minuend, gs_raster_point2d Subtrahend)
{
        gs_raster_point2d Result;
        Result.X = Minuend.X - Subtrahend.X;
        Result.Y = Minuend.Y - Subtrahend.Y;
        return(Result);
}

line_segment
FromPoints(gs_raster_point2d Start, gs_raster_point2d End)
{
        line_segment Result;
        Result.Start = Start;
        Result.End = End;
        return(Result);
}

ray2d
FromLineSegment(line_segment Segment)
{
        ray2d Result;
        Result.Pos = Segment.Start;
        Result.Dir = Subtract(Segment.End, Segment.Start);
        return(Result);
}

line_segment
FromEdge(edge Edge)
{
        line_segment Result = FromPoints(Edge.Start, Edge.End);
        return(Result);
}


void
GsRasterReorderTriangle(gs_raster_triangle *Unordered)
{
        // Counter-clockwise.
        // Bottom-left starting.

        gs_raster_triangle Temp;

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

gs_raster_triangle_edges
FromTriangle(gs_raster_triangle Triangle)
{
        gs_raster_triangle_edges Result;

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
        Result.Y = (float)Height;
        Result.Dx = 1;
        Result.Dy = 0;
        return(Result);
}

/* This assumes a horizontal +X Vector. */
bool
HasIntersection(ray2d Ray, line_segment Line)
{
        int A = Line.EndY - Ray.Y;
        int B = Line.StartY - Ray.Y;
        return(((A ^ B) < 0) || Line.EndY == Ray.Y || Line.StartY == Ray.Y);
}

float
Intersect(ray2d Ray, line_segment Line)
{
        vector2d Slope = Subtract(Line.End, Line.Start);
        bool IsNegative = (Slope.X < 0 || Slope.Y < 0);

        float DeltaY = abs(Line.StartY - Ray.Y);

        float Term2 = (DeltaY * (Slope.X / Slope.Y));

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
TriangleIntersectionSort(const void *Left, const void *Right)
{
        gs_raster_triangle_intersection First  = *((gs_raster_triangle_intersection *)Left);
        gs_raster_triangle_intersection Second = *((gs_raster_triangle_intersection *)Right);

        if(First.X > Second.X) return(1);
        if(First.X < Second.X) return(-1);

        return(0);
}

/*
 * Scanlines must be initialized to contain NumScanlines scanlines.
 * Triangles must be an initialized array of gs_raster_triangles.
 */
void
GsRasterGenerateScanlines(gs_raster_triangle *Triangles, int NumTriangles, gs_raster_scanline *Scanlines, int NumScanlines)
{
        for(int Row = 0; Row < NumScanlines; Row++)
        {
                gs_raster_scanline *Scanline = Scanlines + Row;
                ray2d Ray = PositiveXVectorAtHeight(Row);

                for(int Index = 0; Index < NumTriangles; Index++)
                {
                        gs_raster_triangle *Triangle = &Triangles[Index];
                        gs_raster_triangle_edges Edges = FromTriangle(*Triangle);

                        int NumTriangleIntersections = 0;

                        for(int Index=0; Index < 3; ++Index)
                        {
                                line_segment Edge = FromPoints(Edges.Edges[Index].Start, Edges.Edges[Index].End);

                                if(HasIntersection(Ray, Edge))
                                {
                                        // NOTE(AARON):
                                        // For a given gs_raster_triangle we expect to only have two intersections, max.
                                        // More than this causes rendering artifacts because any two edges
                                        // probably share a vertex. When a vertex is shared, then then the
                                        // intersection points get recalculated, which will cause problems
                                        // at rasterization.
                                        ++NumTriangleIntersections;
                                        if(NumTriangleIntersections > 2)
                                        {
                                                continue;
                                        }

                                        float X = Intersect(Ray, Edge);
                                        gs_raster_triangle_intersection *Intersection = &Scanline->Intersections[Scanline->NumIntersections];
                                        Intersection->Triangle = Triangle;
                                        Intersection->X = X;
                                        Scanline->NumIntersections++;
                                }
                        }
                }

                qsort(Scanline->Intersections,
                      Scanline->NumIntersections,
                      sizeof(gs_raster_triangle_intersection),
                      TriangleIntersectionSort);
        }
}

void
PutPixel(int *Pixels, int Width, int Height, int X, int Y, int NewPixel)
{
        int Index = Y * Width + X;
        Assert(Index < (Width * Height));
        Pixels[Index] = NewPixel;
}

/*
 * Pixels is a Width * Height grid of pixels intended for display somewhere.
 * scanline_intersectio
 */
void
GsRasterRasterize(int *Pixels, int Width, int Height, gs_raster_scanline *Scanlines, gs_raster_triangle Triangles[], gs_raster_color Colors[], int NumTriangles)
{
        gs_raster_triangle_stack *CurrentTriangle;
        TriangleStackInit(&CurrentTriangle, Width, NULL);

        for(int Row=0; Row<Height; Row++)
        {
                gs_raster_scanline *Scanline = &Scanlines[Row];

                for(int Col=0; Col<Width; Col++)
                {

                        for(int s=0; s<Scanline->NumIntersections; ++s)
                        {
                                gs_raster_triangle_intersection *Intersection = &(Scanline->Intersections[s]);

                                if(Intersection->X == Col)
                                {
                                        gs_raster_triangle *Triangle;

                                        if(TriangleStackFind(CurrentTriangle, Intersection->Triangle))
                                        {
                                                TriangleStackRemove(CurrentTriangle, Intersection->Triangle);
                                        }
                                        else
                                        {
                                                TriangleStackPush(CurrentTriangle, Intersection->Triangle);
                                        }

                                }
                        }

                        // TODO(AARON): This may be a NULL pointer.
                        gs_raster_triangle *Triangle = TriangleStackTop(CurrentTriangle);
                        gs_raster_color Color = 0x00000000;
                        if(Triangle)
                        {
                                Color = ColorForTriangle(Triangles, Colors, NumTriangles, Triangle);
                        }

                        PutPixel(Pixels, Width, Height, Col, Row, Color);
                }
        }
}
