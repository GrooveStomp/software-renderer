#include <stdio.h>

struct buffer {
	char *Start;
        char *Cursor;
	size_t Capacity;
	size_t Length;
};

void
CopyBuffer(struct buffer *Source, struct buffer *Dest)
{
	Dest->Start = Source->Start;
	Dest->Cursor = Source->Cursor;
	Dest->Length = Source->Length;
	Dest->Capacity = Source->Capacity;
}

struct buffer *
BufferSet(struct buffer *Buffer, char *Cursor, size_t Length, size_t Capacity)
{
	Buffer->Start = Cursor;
	Buffer->Cursor = Cursor;
	Buffer->Length = Length;
	Buffer->Capacity = Capacity;
	return(Buffer);
}

bool
CopyFileIntoBuffer(char *FileName, struct buffer *Mem)
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
BufferNextLine(struct buffer *Buffer)
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
IsEndOfBuffer(struct buffer *Buffer)
{
	bool Result = (Buffer->Cursor - Buffer->Start) >= Buffer->Length;
	return(Result);
}
