#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef int bool;
#define false 0
#define true !false

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
