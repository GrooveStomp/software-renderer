#if !defined(_GS_RASTER_H)
#define _GS_RASTER_H

#include <stdint.h>

typedef uint32_t color;

struct point2d
{
        float X;
        float Y;
};
typedef struct point2d point2d;
typedef point2d vector2d;

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
		struct
		{
			float X1;
			float Y1;
			float X2;
			float Y2;
			float X3;
			float Y3;
		};
        };
};
typedef struct triangle triangle;

struct triangle_intersection;
typedef struct triangle_intersection triangle_intersection;

struct scanline
{
        triangle_intersection *Intersections;
        int NumIntersections; /* Actual number we have stored. */
        int Capacity; /* Total number we can store. */
};
typedef struct scanline scanline;

void
InitScanlines(scanline **Scanlines, int NumScanlines, int Capacity, void *Memory);

void
GenerateScanlines(triangle *Triangles, int NumTriangles, scanline *Scanlines, int NumScanlines);

void
Rasterize(int *Pixels, int Width, int Height, scanline *Scanlines, triangle Triangles[], color Colors[], int NumTriangles);

void
OrderForRaster(triangle *Unordered);

#endif /* _GS_RASTER_H */
