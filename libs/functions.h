// =======================================================
// functions.h
// -------------------------------------------------------
// Generic functions
// =======================================================



#define _COPYBLOCKS 1024*1024



// Prototypes
// *******************************************************

int correct_path(char **path);
int file_copy(char *source, char *destination, struct stat stats);
struct tm *get_time();
int join_strings(char **final_string, int string_number, ...);
int save_current_date(char **string, unsigned long *timestamp);
int occurrence_count(char *string, char c);
int strpos (char *string, char c, int offset);
int create_directory_tree(char *path);
int copy_attributes(char *destination, int type /*8-file, 4-dir*/, struct stat stats);
int delete_directory(char* path);

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



// Converts seconds to human readable time string
// -------------------------------------------------------
char *seconds_to_time(time_t seconds)
{
	char *string;
	int sec, min, hrs, day;

	if ( (string = (char*) malloc(50 * sizeof(char)) ) == NULL )
	{
		printf("[Error  ]\tInsufficient physical memory");
		exit(-1);
	}

	if (seconds < 60)
	{
		sprintf(string, "%ld\"", seconds);
	}

	if (seconds < 3600)
	{
		sec = seconds % 60;
		min = (seconds - sec) / 60;
		sprintf(string, "%02d'%02d\"", min, sec);
	}

	if (seconds >= (3600))
	{
		sec = seconds % 60;
		min = ((seconds-sec) % 3600) / 60;
		hrs = (seconds - min - sec) / 3600;
		sprintf(string, "%dh%02d'%02d\"", hrs, min, sec);
	}

	return string;
}



// Check if execution is root
// -------------------------------------------------------
int is_root()
{
	FILE *Fp;

	Fp = fopen(_TEMP_ROOT_FILE, "w");

	if (Fp == NULL)
	{
		return false;
	}

	fclose(Fp);
	unlink(_TEMP_ROOT_FILE);

	return true;
}



// Generate a structure containing current GMT date
// -------------------------------------------------------
struct tm *get_time()
{
	time_t timestamp;
	struct tm *date;

	time(&timestamp);		// Get current timestmap
	date = gmtime(&timestamp);	// Get date structure
	
	return date;
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



// Return a string containing scaled bytes value
// -------------------------------------------------------
char *scale_bytes(double bytes)
{
	char *string;

	if ((string = (char*) malloc(50 * sizeof(char))) == NULL)
	{
		printf("[Error  ]\tInsufficient physical memory");
		exit(-1);
	}

	if (bytes < 1024) sprintf(string, "%15.0f B", bytes);
	else if (bytes < 1024 * 1024) sprintf(string, "%15.2f KiB", bytes / 1024);
	else if (bytes < 1024 * 1024 * 1024) sprintf(string, "%15.2f MiB", bytes /  1024 / 1024);
	else sprintf(string, "%15.2f GiB", bytes / 1024 / 1024 / 1024);

	return string;
}



// Returns current GMT human readable date string
// -------------------------------------------------------
int save_current_date(char **string, unsigned long *timestamp)
{
	struct tm *datetime;

	datetime = get_time();
	*timestamp = mktime(datetime);
	

	if ((*string = (char*) malloc(20 * sizeof(char))) == NULL)
	{
		printf("[Error  ]\tInsufficient physical memory");
		exit(-1);
	}

	sprintf(*string, "%d-%02d-%02d %02d.%02d.%02d", (datetime->tm_year+1900), (datetime->tm_mon+1), datetime->tm_mday, datetime->tm_hour, datetime->tm_min, datetime->tm_sec);

	return 0;
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



// Recursively delete a directory and all subdirectories
// -------------------------------------------------------
int delete_directory(char* path)
{
	struct dirent **eps;
	int i, n;
	char *new_dir, *current_file;

	n = scandir (path, &eps, NULL, alphasort);

	for (i=0; i<n; i++)
	{
		if ( strcmp(eps[i]->d_name, ".")==0 ) continue;
		if ( strcmp(eps[i]->d_name, "..")==0 ) continue;

		join_strings(&current_file, 2, path, eps[i]->d_name);

		if (eps[i]->d_type == 4)
		{
			join_strings(&new_dir, 2, current_file, "/");
			delete_directory(new_dir);
			free(new_dir);
			rmdir(new_dir);
		}
		else
			unlink(current_file);
	}

	if ( rmdir(path) != 0 )
	{
		printf("[Error  ]\t Can't delete directory %s\n", path);
	}
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



// Create all directory tree of a specified path
// -------------------------------------------------------
int create_directory_tree(char *path)
{
	int i;
	int offset;
	char *current_path;
	DIR *Dp;

	offset = 0;

	if ((current_path = (char*) malloc((strlen(path)) * sizeof(char*))) == NULL)
	{
		printf("[Error  ]\tInsufficient physical memory");
		exit(-1);
	}

	for (i = 0; i < (int) strlen(path); i++)
	{
		if (path[i] == '/' && i > 0)
		{
			if ((Dp = opendir(current_path)) == NULL)
			{
				mkdir(current_path, 0777);
			}
			else
			{
				closedir(Dp);
			}
		}

		current_path[i] = path[i];
		current_path[i + 1] = '\0';
	}

	return 0;
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
		

	if (( *final_string=(char*)malloc((length+1)*sizeof(char)) ) == NULL)
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



// Add / to a path, if it misses
// -------------------------------------------------------
int correct_path(char **path)
{
	char *temp = *path;

	if ( temp[strlen(temp)-1]=='/' ) return 0;
	join_strings(path, 2, temp, "/");

	return 0;
}



// Check if file exists
// -------------------------------------------------------
int file_exists(char *path)
{
	FILE *Fp;
	Fp = fopen(path, "r");
	if ( Fp==NULL ) return 0;
	fclose(Fp);
	return 1;
}



// Sets file permissions, owner and group specified in stats
// -------------------------------------------------------
int copy_attributes(char *destination, int type /*8-file, 4-dir*/, struct stat stats)
{
	char *date_string;		// Date string in correct format
	char *command;			// Date modification command string
	struct tm *timestamp;
	time_t mtime;

	mtime = stats.st_mtime;		// Gets last modification date
	
	chmod(destination, stats.st_mode);		// Changes permissions
	chown(destination, stats.st_uid, stats.st_gid);	// Changes owner/group

	// If it is a directory, that's all
	if (type == 4) return 0;

	// If it is a file, sets last modification date as the same of source file one
	if ((date_string = (char*) malloc(18 * sizeof(char))) == NULL)
	{
		printf("[Error  ]\tInsufficient physical memory");
		exit(-1);
	}

	timestamp = localtime(&mtime);
	sprintf(date_string, "%d%02d%02d%02d%02d.%02d", (timestamp->tm_year + 1900), (timestamp->tm_mon + 1), timestamp->tm_mday, timestamp->tm_hour, timestamp->tm_min, timestamp->tm_sec);
	// Sets the same permissions etc
	join_strings(&command, 4, "touch \"", destination, "\" -c -m -t ", date_string);
	system(command);

	// Free malloc() allocated memory
	free(date_string);
	free(command);

	return 0;
}





// Copies a file
// -------------------------------------------------------
int file_copy(char *source, char *destination, struct stat stats)
{
	FILE *Fp1;
	FILE *Fp2;
	char *buffer;
	int block_size;
	int64_t file_size;

	// Gets file size
	file_size = stats.st_size;

	// Opens files respectively as read and write
	Fp1 = fopen(source, "rb");
	Fp2 = fopen(destination, "wb");

	// If opening operation returns in a error, stops
	if (Fp1 == NULL) return -1;
	if (Fp2 == NULL) return -1;

	// If file is smaller than minimum block size, sets block size as the file size
	if ( file_size >= _COPYBLOCKS)
	{
		block_size = _COPYBLOCKS;
	}
	else
	{
		block_size = file_size;
	}

	// Allocated copy block memory
	buffer = (char*) malloc(block_size * sizeof(char));
	if (buffer == NULL)
	{
		printf("[Error  ]\tInsufficient physical memory!");
		exit(-1);
	}

	// Read and write every block till the end of file
	while (file_size > 0)
	{
		if ( file_size >= _COPYBLOCKS )
		{
			block_size = _COPYBLOCKS;
		}
		else
		{
			block_size = file_size;
		}

		fread(buffer, block_size, 1, Fp1);
		fwrite(buffer, block_size, 1, Fp2);

		file_size -= _COPYBLOCKS;
	}

	// Closed source and destination files
	fclose(Fp1);
	fclose(Fp2);

	// Copies all attributes from source file to destination one
	copy_attributes(destination, 8, stats);

	// Frees malloc() allocated memory
	free(buffer);

	return 0;
}
