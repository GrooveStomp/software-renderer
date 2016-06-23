/* NOTE(AARON): Assumes util.c is included beforehand. */

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

struct triangle_intersection
{
        uint32 X;
        triangle *Triangle;
};
typedef struct triangle_intersection triangle_intersection;

struct scanline
{
        triangle_intersection *Intersections;
        int NumIntersections; /* Actual number we have stored. */
        int Capacity; /* Total number we can store. */
};
typedef struct scanline scanline;

int
SizeRequiredForScanlines(int NumScanlines, int Capacity)
{
        int Result = (sizeof(scanline) + Capacity) * NumScanlines;
        return(Result);
}

/* Memory can be NULL.  If NULL, allocate on the heap. If not NULL, then use
   Memory. */
void
InitScanlines(scanline **Scanlines, int NumScanlines, int Capacity, void *Memory)
{
        if(Memory == NULL)
        {
                int Size = SizeRequiredForScanlines(NumScanlines, Capacity);
                Memory = malloc(Size);
        }
        *Scanlines = (scanline *)Memory;

        for(int Index = 0; Index < NumScanlines; Index++)
        {
                scanline *CurrentScanline = &(*Scanlines)[Index];
                CurrentScanline->Capacity = Capacity;
                CurrentScanline->NumIntersections = 0;

                int SizeOfScanlines = sizeof(scanline) * NumScanlines;
                char *Intersections = (char *)Memory + SizeOfScanlines;
                int Offset = Capacity * Index;

                CurrentScanline->Intersections = (triangle_intersection *)(Intersections + Offset);
        }
}

struct triangle_stack
{
        triangle **Stack;
        int Head;
};
typedef struct triangle_stack triangle_stack;

//------------------------------------------------------------------------------
// Material Operations
//------------------------------------------------------------------------------

color
ColorForTriangle(triangle *Triangles, color *Colors, int Count, triangle *Triangle)
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
TriangleStackInit(triangle_stack **Stack, int Capacity, void *Memory)
{
        if(Memory == NULL)
        {
                int PointerArraySize = sizeof(triangle *) * Capacity;
                Memory = malloc(sizeof(triangle_stack) + PointerArraySize);
        }
        *Stack = (triangle_stack *)Memory;

        triangle_stack *StackPointer = *Stack;
        void *MemoryOffset = (char *)Memory + sizeof(triangle_stack);
        StackPointer->Stack = (triangle **)MemoryOffset;
        StackPointer->Head = 0;
}

bool
TriangleStackIsEmpty(triangle_stack *Stack)
{
        return(Stack->Head == 0);
}

uint32
TriangleStackSize(triangle_stack *Stack)
{
        return(Stack->Head);
}

bool
TriangleStackPush(triangle_stack *Stack, triangle *Object)
{
        ++Stack->Head;
        Stack->Stack[Stack->Head] = Object;
        return(true);
}

triangle *
TriangleStackPop(triangle_stack *Stack)
{
        if(Stack->Head <= 0) return(NULL);

        triangle *Result = Stack->Stack[Stack->Head];
        --Stack->Head;

        return(Result);
}

triangle *
TriangleStackTop(triangle_stack *Stack)
{
        return(Stack->Stack[Stack->Head]);
}

bool
TriangleStackFind(triangle_stack *Stack, triangle *Object)
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
TriangleStackRemove(triangle_stack *Stack, triangle *Object)
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
TriangleIntersectionSort(const void *Left, const void *Right)
{
        triangle_intersection First  = *((triangle_intersection *)Left);
        triangle_intersection Second = *((triangle_intersection *)Right);

        if(First.X > Second.X) return(1);
        if(First.X < Second.X) return(-1);

        return(0);
}

/*
 * Scanlines must be initialized to contain NumScanlines scanlines.
 * Triangles must be an initialized array of triangles.
 */
void
GenerateScanlines(triangle *Triangles, int NumTriangles, scanline *Scanlines, int NumScanlines)
{
        for(int Row = 0; Row < NumScanlines; Row++)
        {
                scanline *Scanline = Scanlines + Row;
                ray2d Ray = PositiveXVectorAtHeight(Row);

                for(int Index = 0; Index < NumTriangles; Index++)
                {
                        triangle *Triangle = &Triangles[Index];
                        triangle_edges Edges = FromTriangle(*Triangle);

                        int NumTriangleIntersections = 0;

                        for(int Index=0; Index < 3; ++Index)
                        {
                                line_segment Edge = FromPoints(Edges.Edges[Index].Start, Edges.Edges[Index].End);

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
                                        triangle_intersection *Intersection = &Scanline->Intersections[Scanline->NumIntersections];
                                        Intersection->Triangle = Triangle;
                                        Intersection->X = X;
                                        Scanline->NumIntersections++;
                                }
                        }
                }

                qsort(Scanline->Intersections,
                      Scanline->NumIntersections,
                      sizeof(triangle_intersection),
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
Rasterize(int *Pixels, int Width, int Height, scanline *Scanlines, triangle Triangles[], color Colors[], int NumTriangles)
{
        triangle_stack *CurrentTriangle;
        TriangleStackInit(&CurrentTriangle, Width, NULL);

        for(int Row=0; Row<Height; Row++)
        {
                scanline *Scanline = &Scanlines[Row];

                for(int Col=0; Col<Width; Col++)
                {

                        for(int s=0; s<Scanline->NumIntersections; ++s)
                        {
                                triangle_intersection *Intersection = &(Scanline->Intersections[s]);

                                if(Intersection->X == Col)
                                {
                                        triangle *Triangle;

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
                        triangle *Triangle = TriangleStackTop(CurrentTriangle);
                        int32 Color = 0x00000000;
                        if(Triangle)
                        {
                                Color = ColorForTriangle(Triangles, Colors, NumTriangles, Triangle);
                        }

                        PutPixel(Pixels, Width, Height, Col, Row, Color);
                }
        }
}
