// =======================================================
// string_functions.h
// -------------------------------------------------------
// String-related functions
// =======================================================



// Prototypes
// *******************************************************

int join_strings(char **final_string, int string_number, ...);
int occurrence_count(char *string, char c);
int strpos (char *string, char c, int offset);
int clean_string(char **out);
int string_match(char *search, char* string);

// *******************************************************



// Add a \ before each ' to all occurrences in the input string
// -------------------------------------------------------
int clean_string(char **out)
{
	char *string;
	char *temp;
	int i, j;

	join_strings(&string, 1, *out);

	if (strpos(string, '\'', 0) == -1) return 0;

	temp = (char*) malloc(strlen(string) * 2 * sizeof(char));

	for (i = 0, j = 0; i < (int)strlen(string); i++, j++)
	{
		if (string[i] == '\'')
		{
			temp[j] = '\'';
			j++;
		}

		temp[j] = string[i];
	}

	temp[j] = '\0';

	join_strings(out, 1, temp);

	return 0;
}



// Searches for occurrence of a string into another one
// -------------------------------------------------------
int string_match(char *search, char* string)
{
	int i,j;
	int different;


	if (strlen(search) > strlen(string)) return -1;		// If search string is longer than guest one, then it cannot be contained
	if (strlen(search) == 0) return -1;			// If search string is empty, then it cannot be contained

	for (i = 0; i < (int) strlen(string) - (int) strlen(search) + 1; i++)
	{
		different = false;

		for (j = 0; j < (int)strlen(search); j++)
		{
			if (string[i + j] != search[j])
			{
				different = true;
				break;
			}
		}

		if (! different )
		{
			return i;
		}
	}

	return -1;
}



// Returns first occurrence index of a character in the given string
// -------------------------------------------------------
int strpos (char *string, char c, int offset)
{
	int i;

	if (strlen(string) == 0) return -1;

	for (i = offset; i < (int)strlen(string); i++)
	{
		if (string[i] == c)
		{
			return i;
		}
	}

	return -1;
}



// Join two or more strings
// -------------------------------------------------------
int join_strings(char **final_string, int string_number, ...)
{
	int i;
	va_list vl;
	int length;

	length = 0;
	va_start(vl, string_number);

	for (i = 0; i < string_number; i++)
	{
		length += strlen(va_arg(vl,char*));
	}

	va_end(vl);
		

	if (( *final_string = (char*) malloc((length + 1) * sizeof(char)) ) == NULL)
	{
		printf("[Error  ]\tInsufficient physical memory");
		exit(-1);
	}

	*final_string[0] = '\0';

	va_start(vl, string_number);

	for (i=0; i<string_number; i++)
	{
		strcat(*final_string, va_arg(vl, char*));
	}

	va_end(vl);

	return length;
}



// Counts how many times a given character is contained in a string
// -------------------------------------------------------
int occurrence_count(char *string, char c)
{
	int i, count;

	if (strlen(string) == 0) return 0;

	count = 0;

	for (i = 0; i < (int) strlen(string); i++)
	{
		if ( string[i] == c ) count++;
	}

	return count;
}
