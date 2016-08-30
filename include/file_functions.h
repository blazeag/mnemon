// =======================================================
// file_functions.h
// -------------------------------------------------------
// File-related functions
// =======================================================



// Prototypes
// *******************************************************

int is_root();
int correct_path(char **path);
int file_copy(char *source, char *destination, struct stat stats);
int create_directory_tree(char *path);
int copy_attributes(char *destination, int type /*8-file, 4-dir*/, struct stat stats);
int delete_directory(char* path);
char *scale_bytes(double bytes);
int file_exists(char *path);

// *******************************************************



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



// Add / to a path, if it misses
// -------------------------------------------------------
int correct_path(char **path)
{
	char *temp = *path;

	if (temp[strlen(temp) - 1] == '/') return 0;
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



// Set file permissions, owner and group specified in stats
// -------------------------------------------------------
int copy_attributes(char *destination, int type /*8-file, 4-dir*/, struct stat stats)
{
	char *date_string;		// Date string in correct format
	char *command;			// Date modification command string
	struct tm *timestamp;
	time_t mtime;

	mtime = stats.st_mtime;		// Get last modification date
	
	chmod(destination, stats.st_mode);		// Change permissions
	chown(destination, stats.st_uid, stats.st_gid);	// Change owner/group

	// If it is a directory, that's all
	if (type == 4) return 0;

	// If it is a file, set last modification date as the same of source file one
	if ((date_string = (char*) malloc(18 * sizeof(char))) == NULL)
	{
		printf("[Error  ]\tInsufficient physical memory");
		exit(-1);
	}

	timestamp = localtime(&mtime);
	sprintf(date_string, "%d%02d%02d%02d%02d.%02d", (timestamp->tm_year + 1900), (timestamp->tm_mon + 1), timestamp->tm_mday, timestamp->tm_hour, timestamp->tm_min, timestamp->tm_sec);
	// Set the same permissions etc
	join_strings(&command, 4, "touch \"", destination, "\" -c -m -t ", date_string);
	system(command);

	// Free malloc() allocated memory
	free(date_string);
	free(command);

	return 0;
}





// Copy a file
// -------------------------------------------------------
int file_copy(char *source, char *destination, struct stat stats)
{
	FILE *Fp1;
	FILE *Fp2;
	char *buffer;
	int block_size;
	int64_t file_size;

	// Get file size
	file_size = stats.st_size;

	// Open files respectively as read and write
	Fp1 = fopen(source, "rb");
	Fp2 = fopen(destination, "wb");

	// If opening operation returns in a error, stop
	if (Fp1 == NULL) return -1;
	if (Fp2 == NULL) return -1;

	// If file is smaller than minimum block size, set block size as the file size
	if ( file_size >= _COPY_BLOCK_SIZE)
	{
		block_size = _COPY_BLOCK_SIZE;
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
		if ( file_size >= _COPY_BLOCK_SIZE )
		{
			block_size = _COPY_BLOCK_SIZE;
		}
		else
		{
			block_size = file_size;
		}

		fread(buffer, block_size, 1, Fp1);
		fwrite(buffer, block_size, 1, Fp2);

		file_size -= _COPY_BLOCK_SIZE;
	}

	// Closed source and destination files
	fclose(Fp1);
	fclose(Fp2);

	// Copy all attributes from source file to destination one
	copy_attributes(destination, 8, stats);

	// Free malloc() allocated memory
	free(buffer);

	return 0;
}
