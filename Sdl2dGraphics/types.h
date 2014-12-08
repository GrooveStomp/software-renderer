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
