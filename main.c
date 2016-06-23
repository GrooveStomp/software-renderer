#include "SDL.h"
#include "raster.h"

#include <alloca.h>
#include <stdio.h>
#include <stdlib.h> /* EXIT_SUCCESS, EXIT_FAILURE */

typedef int bool;
#define false 0
#define true !false

/******************************************************************************
 * Strings
 ******************************************************************************/

bool
StringEqual(char *LeftString, char *RightString, int MaxNumToMatch)
{
        int NumMatched = 0;

        for(;
            *LeftString == *RightString && NumMatched < MaxNumToMatch;
            LeftString++, RightString++, MaxNumToMatch++)
        {
                if(*LeftString == '\0') return(true);
        }
        return(false);
}

int
StringLength(char *String)
{
        char *P = String;
        while(*P != '\0') P++;
        return(P - String);
}

/******************************************************************************
 * Streams
 ******************************************************************************/

struct buffer
{
        char *Start;
        char *Cursor;
        size_t Capacity;
        size_t Length;
};
typedef struct buffer buffer;

void
CopyBuffer(buffer *Source, buffer *Dest)
{
        Dest->Start = Source->Start;
        Dest->Cursor = Source->Cursor;
        Dest->Length = Source->Length;
        Dest->Capacity = Source->Capacity;
}

buffer *
BufferSet(buffer *Buffer, char *Cursor, size_t Length, size_t Capacity)
{
        Buffer->Start = Cursor;
        Buffer->Cursor = Cursor;
        Buffer->Length = Length;
        Buffer->Capacity = Capacity;
        return(Buffer);
}

bool
CopyFileIntoBuffer(char *FileName, buffer *Mem)
{
        FILE *File = fopen(FileName, "r");
        if(File)
        {
                fseek(File, 0, SEEK_END);
                size_t FileSize = ftell(File);
                if(FileSize + 1 > Mem->Capacity)
                {
                        return(false);
                }

                fseek(File, 0, SEEK_SET);
                fread(Mem->Cursor, 1, FileSize, File);
                Mem->Cursor[FileSize] = 0;
                Mem->Length = FileSize;

                fclose(File);
        }

        return(true);
}

size_t  /* Returns size of file in bytes plus one for trailing '\0'. */
FileSize(char *FileName)
{
        FILE *File = fopen(FileName, "r");
        if(File)
        {
                fseek(File, 0, SEEK_END);
                size_t FileSize = ftell(File);
                fclose(File);
                return(FileSize + 1); /* +1 for trailing null byte. */
        }

        return(0);
}

void
BufferNextLine(buffer *Buffer)
{
        while((Buffer->Cursor - Buffer->Start) < Buffer->Length)
        {
                if(*(Buffer->Cursor) == '\n')
                {
                        ++Buffer->Cursor;
                        break;
                }
                ++Buffer->Cursor;
        }
}

bool
IsEndOfBuffer(buffer *Buffer)
{
        bool Result = (Buffer->Cursor - Buffer->Start) >= Buffer->Length;
        return(Result);
}

/******************************************************************************
 * Main section for client code to raster library.
 ******************************************************************************/

enum DISPLAY_SIZE
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
CreateRasterDatastructuresFromFile(char *Filename, gs_raster_triangle **Triangles, gs_raster_color **Colors, int *Count)
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

        *Triangles = (gs_raster_triangle *)malloc(sizeof(gs_raster_triangle) * NumTriangles);
        *Colors = (gs_raster_color *)malloc(sizeof(gs_raster_color) * NumTriangles);

        for(int i=0; i<NumTriangles; i++)
        {
                gs_raster_triangle *Triangle = &(*Triangles)[i];

                sscanf(FileContents.Cursor, "%f,%f %f,%f %f,%f %x",
                       &(Triangle->X1), &(Triangle->Y1),
                       &(Triangle->X2), &(Triangle->Y2),
                       &(Triangle->X3), &(Triangle->Y3),
                       &(*Colors)[i]);

                GsRasterReorderTriangle(Triangle);
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

        gs_raster_scanline *Scanlines;
        gs_raster_triangle *Triangles;
        gs_raster_color *Colors;
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
        GsRasterInitScanlines(&Scanlines, DISPLAY_HEIGHT, DISPLAY_WIDTH, NULL);
        GsRasterGenerateScanlines(Triangles, NumTriangles, Scanlines, DISPLAY_HEIGHT);

        SDL_LockTexture(Texture, NULL, (void**)&DisplayBuffer, &DisplayBufferPitch);
        {
                GsRasterRasterize(DisplayBuffer, DISPLAY_WIDTH, DISPLAY_HEIGHT, Scanlines, Triangles, Colors, NumTriangles);
        }
        SDL_UnlockTexture(Texture);

        bool Running = true;
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
