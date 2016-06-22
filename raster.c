#include "util.c"

enum DISPLAY_SIZE
{
        DISPLAY_WIDTH = 1024,
        DISPLAY_HEIGHT = 768
};

struct point2d
{
        real32 X;
        real32 Y;
};
typedef struct point2d point2d;
typedef point2d vector2d;

struct ray2d
{
        union
        {
                struct
                {
                        point2d Pos;
                        point2d Dir;
                };
                struct
                {
                        real32 X;
                        real32 Y;
                        real32 Dx;
                        real32 Dy;
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
                        point2d Start;
                        point2d End;
                };
                struct
                {
                        real32 StartX;
                        real32 StartY;
                        real32 EndX;
                        real32 EndY;
                };
        };
};
typedef struct line_segment line_segment;
typedef line_segment edge;

struct triangle
{
        union
        {
                point2d Point[3];
                struct
                {
                        point2d A;
                        point2d B;
                        point2d C;
                };
        };
};
typedef struct triangle triangle;

struct triangle_edges
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
typedef struct triangle_edges triangle_edges;

struct materials
{
        triangle* Triangles[DISPLAY_WIDTH];
        color Colors[DISPLAY_WIDTH];
        int Count;
};
typedef struct materials materials;

struct scanline_intersection
{
        uint32 X;
        triangle* Triangle;
};
typedef struct scanline_intersection scanline_intersection;

// TODO(AARON): Export this structure as public.
struct scanline
{
        // At most we can have pixel_width intersections, so we can have a linear array initialized to that size.
        scanline_intersection Intersections[DISPLAY_WIDTH];
        uint32 NumIntersections;
};
typedef struct scanline scanline;

struct stack
{
        triangle* Stack[DISPLAY_WIDTH];
        uint32 Head;
};
typedef struct stack stack;

//------------------------------------------------------------------------------
// Material Operations
//------------------------------------------------------------------------------

color
FromMaterial(materials *Materials, triangle *Triangle)
{
        for(int i=0; i < Materials->Count; ++i)
        {
                if(Materials->Triangles[i] == Triangle)
                {
                        return(Materials->Colors[i]);
                }
        }

        return(0xDEADBEEF);
}

void
InitMaterials(materials *Materials, triangle Triangles[], color Colors[], int Count)
{
        Materials->Count = Count;
        for(int i=0; i< Count; ++i)
        {
                Materials->Triangles[i] = &Triangles[i];
                Materials->Colors[i] = Colors[i];
        }
}

//------------------------------------------------------------------------------
// Stack Operations
//------------------------------------------------------------------------------

void
Init(stack *Stack)
{
        Stack->Head = 0;
}

bool
IsEmpty(stack *Stack)
{
        return(Stack->Head == 0);
}

uint32
Size(stack *Stack)
{
        return(Stack->Head);
}

bool
Push(stack *Stack, triangle *Object)
{
        ++Stack->Head;
        Stack->Stack[Stack->Head] = Object;
        return(true);
}

triangle *
Pop(stack *Stack)
{
        if(Stack->Head <= 0) return(NULL);

        triangle *Result = Stack->Stack[Stack->Head];
        --Stack->Head;

        return(Result);
}

triangle *
Top(stack *Stack)
{
        return(Stack->Stack[Stack->Head]);
}

bool
Find(stack *Stack, triangle *Object)
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
Remove(stack *Stack, triangle *Object)
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

point2d
Add(point2d First, point2d Second)
{
        point2d Result;
        Result.X = First.X + Second.X;
        Result.Y = First.Y + Second.Y;
        return(Result);
}

point2d
Apply(point2d Point, real32 T)
{
        point2d Result;
        Result.X = Point.X * T;
        Result.Y = Point.Y * T;
        return(Result);
}

point2d
Evaluate(ray2d Ray, real32 T)
{
        vector2d Applied = Apply(Ray.Dir, T);
        point2d Result = Add(Ray.Pos, Applied);
        return(Result);
}

point2d
Subtract(point2d Minuend, point2d Subtrahend)
{
        point2d Result;
        Result.X = Minuend.X - Subtrahend.X;
        Result.Y = Minuend.Y - Subtrahend.Y;
        return(Result);
}

line_segment
FromPoints(point2d Start, point2d End)
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
PutPixel(int *DisplayBuffer, int X, int Y, int Pixel)
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
