#include "SDL.h"
#include <alloca.h>

#include "util.c"
#include "raster.c"

enum MAIN_DISPLAY_SIZE
{
        DISPLAY_WIDTH = 1024,
        DISPLAY_HEIGHT = 768,
};

void
AbortWithMessage(const char *msg)
{
        fprintf(stderr, "%s\n", msg);
        exit(EXIT_FAILURE);
}

void
CreateRasterDatastructuresFromFile(char *Filename, triangle **Triangles, color **Colors, int *Count)
{
        size_t AllocSize = FileSize(Filename);
        buffer FileContents;

        /* Allocate space on the stack. */
        BufferSet(&FileContents, (char *)alloca(AllocSize), 0, AllocSize);
        if(!CopyFileIntoBuffer(Filename, &FileContents))
        {
                AbortWithMessage("Couldn't copy entire file to buffer");
        }

        /* Determine how many triangles we need to create. */
        int NumTriangles = 0;
        buffer Reader;
        CopyBuffer(&FileContents, &Reader);
        for(int i=0; i<FileContents.Length; i++)
        {
                if(*(Reader.Cursor) == '\n') NumTriangles++;
                Reader.Cursor++;
        }
        *Count = NumTriangles;

        *Triangles = (triangle *)malloc(sizeof(triangle) * NumTriangles);
        *Colors = (color *)malloc(sizeof(color) * NumTriangles);

        for(int i=0; i<NumTriangles; i++)
        {
                triangle *Triangle = &(*Triangles)[i];

                sscanf(FileContents.Cursor, "%f,%f %f,%f %f,%f %x",
                       &(Triangle->Point[0].X),
                       &(Triangle->Point[0].Y),
                       &(Triangle->Point[1].X),
                       &(Triangle->Point[1].Y),
                       &(Triangle->Point[2].X),
                       &(Triangle->Point[2].Y),
                       &(*Colors)[i]);

                OrderForRaster(Triangle);
                BufferNextLine(&FileContents);
        }
}

void
Usage()
{
        printf("Usage: program definitions_file\n");
        printf("  definitions_file: file in current directory defining triangle coordinates.\n");
        printf("                    See: triangles.def.example\n");
        printf("  Specify '-h' or '--help' for this help text.\n");
        exit(EXIT_SUCCESS);
}

int
main(int ArgCount, char **Args)
{
        for(int i=0; i<ArgCount; i++)
        {
                if(StringEqual(Args[i], "-h", StringLength("-h")) ||
                   StringEqual(Args[i], "--help", StringLength("--help")))
                {
                        Usage();
                }
        }
        if(ArgCount != 2) Usage();

        SDL_Window *Window;
        SDL_Renderer *Renderer;
        SDL_Texture *Texture;
        int DisplayBufferPitch;
        int *DisplayBuffer;

	scanline *Scanlines;
        triangle *Triangles;
        color *Colors;
        int NumTriangles;

        DisplayBuffer = (int*)malloc(DISPLAY_WIDTH * DISPLAY_HEIGHT * 4);

        if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS) != 0)
        {
                AbortWithMessage(SDL_GetError());
        }

        Window = SDL_CreateWindow("My Awesome Window", 100, 100, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0);
        if(Window == NULL) AbortWithMessage(SDL_GetError());

        Renderer = SDL_CreateRenderer(Window, -1, SDL_RENDERER_SOFTWARE);
        if(Renderer == NULL) AbortWithMessage(SDL_GetError());

        Texture = SDL_CreateTexture(Renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, DISPLAY_WIDTH, DISPLAY_HEIGHT);
        if(Texture == NULL) AbortWithMessage(SDL_GetError());

        CreateRasterDatastructuresFromFile(Args[1], &Triangles, &Colors, &NumTriangles);
        InitScanlines(&Scanlines, DISPLAY_HEIGHT, DISPLAY_WIDTH, NULL);
        GenerateScanlines(Triangles, NumTriangles, Scanlines, DISPLAY_HEIGHT);

        SDL_LockTexture(Texture, NULL, (void**)&DisplayBuffer, &DisplayBufferPitch);
	{
		Rasterize(DisplayBuffer, DISPLAY_WIDTH, DISPLAY_HEIGHT, Scanlines, Triangles, Colors, NumTriangles);
	}
        SDL_UnlockTexture(Texture);

        bool32 Running = true;
        while(Running)
        {
                SDL_Event Event;

                while(SDL_PollEvent(&Event))
                {
                        switch(Event.type)
                        {
                                case SDL_QUIT:
                                {
                                        Running = false;
                                } break;

                                case SDL_KEYDOWN:
                                case SDL_KEYUP:
                                {
                                        SDL_Keycode KeyCode = Event.key.keysym.sym;

                                        if(Event.key.repeat == 0)
                                        {
                                                if(KeyCode == SDLK_ESCAPE)
                                                {
                                                        Running = false;
                                                }
                                        }
                                } break;
                        }
                }

                SDL_RenderClear(Renderer);
                SDL_RenderCopy(Renderer, Texture, 0, 0);
                SDL_RenderPresent(Renderer);
        }

        SDL_DestroyTexture(Texture);
        SDL_DestroyRenderer(Renderer);
        SDL_DestroyWindow(Window);
        SDL_Quit();

        return(0);
}
