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

struct point2i{
  int32 X;
  int32 Y;
};
typedef point2i vertex2i;
typedef point2i vector2i;

struct point2f {
  real32 X;
  real32 Y;
};
typedef point2f vertex2f;
typedef point2f vector2f;

struct ray2d {
  union {
    struct {
      point2f Pos;
      point2f Dir;
    };
    struct {
      point2f Start;
      point2f End;
    };
    struct {
      struct {
        real32 X;
        real32 Y;
      };
      struct {
        real32 Dx;
        real32 Dy;
      };
    };
  };
};

typedef ray2d line_segment;
typedef ray2d edge;

struct triangle {
  point2i Point[3];
};

struct triangle_edges {
  edge Edges[3];
};

struct Material {
  byte r, g, b, a;
};

struct intersection_point {
  bool32 IsIntersection;
  point2f Intersection;
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
point2f Evaluate(ray2d Ray, float T) {
  point2f Result = { 0 };
  Result.X = Ray.X + Ray.Dx * T;
  Result.Y = Ray.Y + Ray.Dy * T;
  return Result;
}

point2f Subtract(point2f Minuend, point2f Subtrahend) {
  point2f Result = { 0 };
  Result.X = Minuend.X - Subtrahend.X;
  Result.Y = Minuend.Y - Subtrahend.Y;
  return Result;
}

line_segment FromPoints(point2f Start, point2f End) {
  line_segment Result;
  Result.X = Start.X;
  Result.Y = Start.Y;
  Result.Dx = End.X - Start.X;
  Result.Dy = End.Y - Start.Y;
  return Result;
}

line_segment FromEdge(edge Edge) {
  return FromPoints(Edge.Start, Edge.End);
}

#endif // ifndef _TYPES_H
