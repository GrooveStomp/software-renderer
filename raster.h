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

/*
 * Returns the total size, in bytes, required to allocate scanlines for the
 * given specifications.
 *
 * NumScanlines:
 *         The number of scanlines to be generated. Normally this matches the
 *         vertical resolution of the destination pixel grid.
 *
 * Capacity:
 *         Each scanline contains a list of intersections with the triangle
 *         rasterization list - that is, the list of triangles intended for
 *         rendition to the destination pixel grid.  `Capacity` denotes the
 *         maximum number of intersections that can occur per row in the
 *         destination pixel grid.
 */
int
GsRasterSizeRequiredForScanlines(int NumScanlines, int Capacity);

/*
 * Initializes the scanline list according to the given specifications.
 *
 * Scanlines:
 *         Unallocated pointer to gs_raster_scanlines.
 *
 * NumScanlines:
 *         Normally the number of rows in the pixel grid you are rendering to.
 *
 * Capacity:
 *         The maximum number of intersections that can occur per scanline.
 *
 * Memory:
 *         Optional char buffer to use for scanline storage.
 *         If using this, determine size with GsRasterSizeRequiredForScanlines.
 *         Set to NULL to allow this function to allocate on the heap with
 *         malloc.
 *
 * Example usage:
 *         int DisplayWidth = 1280;
 *         int DisplayHeight = 720;
 *         gs_raster_scanline *Scanlines;
 *         GsRasterInitScanlines(&Scanlines, DisplayHeight, DisplayWidth, NULL);
 */
void
GsRasterInitScanlines(
        gs_raster_scanline **Scanlines,
        int NumScanlines,
        int Capacity,
        void *Memory);

/*
 * Calculates all triangle intersections for the given scanlines and triangles.
 */
void
GsRasterGenerateScanlines(
        gs_raster_triangle *Triangles,
        int NumTriangles,
        gs_raster_scanline *Scanlines,
        int NumScanlines);

/*
 * Translate the given triangle list into pixels in the destination pixel grid.
 */
void
GsRasterRasterize(
        int *Pixels,
        int Width,
        int Height,
        gs_raster_scanline *Scanlines,
        gs_raster_triangle Triangles[],
        gs_raster_color Colors[],
        int NumTriangles);

/*
 * Reorder the given triangle vertices to work with GsRaster rasterization.
 */
void
GsRasterReorderTriangle(
        gs_raster_triangle *Unordered);

#endif /* GS_RASTER */
