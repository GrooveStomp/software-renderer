#ifndef _TYPES_H
#define _TYPES_H

#include <math.h>
#include <stdint.h>

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

typedef uint8 byte;

typedef float real32;
typedef double real64;

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

enum COLOR {
  COLOR_BLUE = 0x0000FF00,
  COLOR_RED = 0xFF000000,
  COLOR_GREEN = 0x00FF0000,
  COLOR_BLACK = 0x000000FF,
  COLOR_OPAQUE = 0x000000FF
};

enum DISPLAY_SIZE {
  DISPLAY_WIDTH = 640,
  DISPLAY_HEIGHT = 480
};

struct point2d{
  real32 X;
  real32 Y;
};
typedef point2d vector2d;

struct ray2d {
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
};

struct line_segment {
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
};
typedef line_segment edge;

struct triangle {
  point2d Point[3];
};

struct triangle_edges {
  edge Edges[3];
};

struct Material {
  byte r, g, b, a;
};

struct intersection_point {
  bool32 IsIntersection;
  point2d Intersection;
};

struct scanline_intersection {
  uint32 X;
  triangle* Triangle;
};

struct scanline {
  // At most we can have pixel_width intersections, so we can have a linear array initialized to that size.
  scanline_intersection Intersections[DISPLAY_WIDTH];
  uint32 NumIntersections;
};

struct stack {
  void* Stack[DISPLAY_WIDTH];
  uint32 Head;
};

struct map {
  uint32 Keys[DISPLAY_WIDTH];
  void* Values[DISPLAY_WIDTH];
  uint32 Count;
};


//------------------------------------------------------------------------------
// Stack Operations
//
void Init(stack* Stack) {
  Stack->Head = 0;
}

bool IsEmpty(stack* Stack) {
  return (Stack->Head == 0);
}

uint32 Size(stack* Stack) {
  return Stack->Head;
}

bool Push(stack* Stack, void* Object) {
  if (Stack->Head >= DISPLAY_WIDTH) return false;

  Stack->Stack[Stack->Head] = Object;
  Stack->Head++;
  return true;
}

void* Pop(stack* Stack) {
  if (Stack->Head <= 0) return NULL;

  void* Result = Stack->Stack[Stack->Head];
  Stack->Head--;
  return Result;
}

void* Top(stack* Stack) {
  if(Stack->Head <= 0) return NULL;

  return Stack->Stack[Stack->Head];
}

//------------------------------------------------------------------------------
// Map operations
//
bool Add(map* Map, uint32 Key, triangle* Triangle) {
  if (Map->Count >= DISPLAY_WIDTH) return false;

  Map->Keys[Map->Count] = Key;
  Map->Values[Map->Count] = (void*)Triangle;
  Map->Count++;
  return true;
}

triangle* Lookup(map* Map, uint32 Key) {
  for (int i=0; i < Map->Count; ++i) {
    if (Map->Keys[i] == Key) {
      return (triangle*)Map->Values[i];
    }
  }
  return NULL;
}


//------------------------------------------------------------------------------
// Ray, Point and other similar operations
//
point2d Add(point2d First, point2d Second) {
  point2d Result;
  Result.X = First.X + Second.X;
  Result.Y = First.Y + Second.Y;
  return Result;
}

point2d Apply(point2d Point, real32 T) {
  point2d Result;
  Result.X = Point.X * T;
  Result.Y = Point.Y * T;
  return Result;
}

point2d Evaluate(ray2d Ray, real32 T) {
  return Add(Ray.Pos, Apply(Ray.Dir, T));
}

point2d Subtract(point2d Minuend, point2d Subtrahend) {
  point2d Result;
  Result.X = Minuend.X - Subtrahend.X;
  Result.Y = Minuend.Y - Subtrahend.Y;
  return Result;
}

line_segment FromPoints(point2d Start, point2d End) {
  line_segment Result;
  Result.Start = Start;
  Result.End = End;
  return Result;
}

ray2d FromLineSegment(line_segment Segment) {
  ray2d Result;
  Result.Pos = Segment.Start;
  Result.Dir = Subtract(Segment.End, Segment.Start);
  return Result;
}

line_segment FromEdge(edge Edge) {
  return FromPoints(Edge.Start, Edge.End);
}

#endif // ifndef _TYPES_H
