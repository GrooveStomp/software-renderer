#if !defined(GS_RASTER)
#define GS_RASTER

#include <stdint.h>

typedef uint32_t gs_raster_color;

struct gs_raster_point2d
{
        float X;
        float Y;
};
typedef struct gs_raster_point2d gs_raster_point2d;
typedef gs_raster_point2d gs_raster_vector2d;

struct gs_raster_triangle
{
        union
        {
                gs_raster_point2d Point[3];
                struct
                {
                        gs_raster_point2d A;
                        gs_raster_point2d B;
                        gs_raster_point2d C;
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
typedef struct gs_raster_triangle gs_raster_triangle;

struct gs_raster_triangle_intersection;
typedef struct gs_raster_triangle_intersection gs_raster_triangle_intersection;

struct gs_raster_scanline
{
        gs_raster_triangle_intersection *Intersections;
        int NumIntersections; /* Actual number we have stored. */
        int Capacity; /* Total number we can store. */
};
typedef struct gs_raster_scanline gs_raster_scanline;

void
GsRasterInitScanlines(gs_raster_scanline **Scanlines, int NumScanlines, int Capacity, void *Memory);

void
GsRasterGenerateScanlines(gs_raster_triangle *Triangles, int NumTriangles, gs_raster_scanline *Scanlines, int NumScanlines);

void
GsRasterRasterize(int *Pixels, int Width, int Height, gs_raster_scanline *Scanlines, gs_raster_triangle Triangles[], gs_raster_color Colors[], int NumTriangles);

void
GsRasterReorderTriangle(gs_raster_triangle *Unordered);

#endif /* GS_RASTER */
