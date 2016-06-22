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
