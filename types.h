#ifndef _TYPES_H
#define _TYPES_H

#include <math.h>
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>

#define internal static
#define local_persist static
#define global_variable static

#define Pi32 3.14159265359f

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef uint32 color;
typedef uint8 byte;

typedef float real32;
typedef double real64;

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

uint32_t COLOR_NULL = 0x00000000;
uint32_t COLOR_BLUE = 0x0000FF00;
uint32_t COLOR_RED = 0xFF000000;
uint32_t COLOR_GREEN = 0x00FF0000;
uint32_t COLOR_BLACK = 0x000000FF;
uint32_t COLOR_WHITE = 0xFFFFFFFF;
uint32_t COLOR_YELLOW = 0xFFFF00FF;
uint32_t COLOR_OPAQUE = 0x000000FF;

enum DISPLAY_SIZE {
        DISPLAY_WIDTH = 1024,
        DISPLAY_HEIGHT = 768
};

typedef struct point2d{
        real32 X;
        real32 Y;
} point2d;
typedef point2d vector2d;

typedef struct ray2d {
        union {
                struct {
                        point2d Pos;
                        point2d Dir;
                };
                struct {
                        real32 X;
                        real32 Y;
                        real32 Dx;
                        real32 Dy;
                };
        };
} ray2d;

typedef struct line_segment {
        union {
                struct {
                        point2d Start;
                        point2d End;
                };
                struct {
                        real32 StartX;
                        real32 StartY;
                        real32 EndX;
                        real32 EndY;
                };
        };
} line_segment;
typedef line_segment edge;

typedef struct triangle {
        union {
                point2d Point[3];
                struct {
                        point2d A;
                        point2d B;
                        point2d C;
                };
        };
} triangle;

typedef struct triangle_edges {
        union {
                edge Edges[3];
                struct {
                        edge AB;
                        edge BC;
                        edge CA;
                };
        };
} triangle_edges;

typedef struct materials {
        triangle* Triangles[DISPLAY_WIDTH];
        color Colors[DISPLAY_WIDTH];
        int Count;
} materials;

typedef struct scanline_intersection {
        uint32 X;
        triangle* Triangle;
} scanline_intersection;

// TODO(AARON): Export this structure as public.
typedef struct scanline {
        // At most we can have pixel_width intersections, so we can have a linear array initialized to that size.
        scanline_intersection Intersections[DISPLAY_WIDTH];
        uint32 NumIntersections;
} scanline;

typedef struct stack {
        triangle* Stack[DISPLAY_WIDTH];
        uint32 Head;
} stack;

//------------------------------------------------------------------------------
// Material Operations
//------------------------------------------------------------------------------

color
FromMaterial(materials *Materials, triangle *Triangle) {
        for (int i=0; i < Materials->Count; ++i) {
                if (Materials->Triangles[i] == Triangle) {
                        return(Materials->Colors[i]);
                }
        }

        return(COLOR_NULL);
}

void
InitMaterials(materials *Materials, triangle Triangles[], color Colors[], int Count) {
        Materials->Count = Count;
        for (int i=0; i< Count; ++i) {
                Materials->Triangles[i] = &Triangles[i];
                Materials->Colors[i] = Colors[i];
        }
}

//------------------------------------------------------------------------------
// Stack Operations
//------------------------------------------------------------------------------

void
Init(stack *Stack) {
        Stack->Head = 0;
}

bool
IsEmpty(stack *Stack) {
        return(Stack->Head == 0);
}

uint32
Size(stack *Stack) {
        return(Stack->Head);
}

bool
Push(stack *Stack, triangle *Object) {
        ++Stack->Head;
        Stack->Stack[Stack->Head] = Object;
        return(true);
}

triangle *
Pop(stack *Stack) {
        if (Stack->Head <= 0) return(NULL);

        triangle *Result = Stack->Stack[Stack->Head];
        --Stack->Head;

        return(Result);
}

triangle *
Top(stack *Stack) {
        return(Stack->Stack[Stack->Head]);
}

bool
Find(stack *Stack, triangle *Object) {
        for (int i=0; i<=Stack->Head; ++i) {
                if (Stack->Stack[i] == Object) {
                        return(true);
                }
        }

        return(false);
}

bool
Remove(stack *Stack, triangle *Object) {
        int index = -1;
        for (int i=0; i <= Stack->Head; ++i) {
                if (Stack->Stack[i] == Object) {
                        index = i;
                        break;
                }
        }

        if (index < 0) {
                return(false);
        }

        for (int i=index; i < Stack->Head; ++i) {
                Stack->Stack[i] = Stack->Stack[i+1];
        }
        --Stack->Head;

        return(true);
}

//------------------------------------------------------------------------------
// Ray, Point and other similar operations
//------------------------------------------------------------------------------

point2d
Add(point2d First, point2d Second) {
        point2d Result;
        Result.X = First.X + Second.X;
        Result.Y = First.Y + Second.Y;
        return(Result);
}

point2d
Apply(point2d Point, real32 T) {
        point2d Result;
        Result.X = Point.X * T;
        Result.Y = Point.Y * T;
        return(Result);
}

point2d
Evaluate(ray2d Ray, real32 T) {
        vector2d Applied = Apply(Ray.Dir, T);
        point2d Result = Add(Ray.Pos, Applied);
        return(Result);
}

point2d
Subtract(point2d Minuend, point2d Subtrahend) {
        point2d Result;
        Result.X = Minuend.X - Subtrahend.X;
        Result.Y = Minuend.Y - Subtrahend.Y;
        return(Result);
}

line_segment
FromPoints(point2d Start, point2d End) {
        line_segment Result;
        Result.Start = Start;
        Result.End = End;
        return(Result);
}

ray2d
FromLineSegment(line_segment Segment) {
        ray2d Result;
        Result.Pos = Segment.Start;
        Result.Dir = Subtract(Segment.End, Segment.Start);
        return(Result);
}

line_segment
FromEdge(edge Edge) {
        line_segment Result = FromPoints(Edge.Start, Edge.End);
        return(Result);
}

#endif // ifndef _TYPES_H
