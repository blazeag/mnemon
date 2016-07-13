// =======================================================
// procedures.h
// -------------------------------------------------------
// Procedural library
// =======================================================



// Prototypes
// *******************************************************

void show_help();
int check_cli_arguments(int argc, char **argv, struct _parameters *parameters);
int is_file_valid(struct dirent *file);
int is_path_excluded(char *path, char **excluded_paths);
int display_final_report(struct _counters *counters);
int counters_initialization(struct _counters *counters);
int write_date_into_db(sqlite3 **db, char *current_date, unsigned long timestamp, struct _counters *counters);
int get_latest_backup_dir(sqlite3 **db, char **string);
void before_exit();
int delete_temp_directory(char *path);
int exists_in_db(sqlite3 **db, char *path, char *latest_backup_root, struct stat stats, char **old_path);
int save_paths(char **paths, char *filename);

// *******************************************************



// Deletes temporary directory, if exists
// -------------------------------------------------------
int delete_temp_directory(char *path)
{
	DIR *Dp;

	join_strings(&path, 2, path, _TEMP_BACKUP_DIR);

	Dp = opendir(path);
	if (Dp == NULL)
	{
		free(path);
		return -1;
	}

	closedir(Dp);
	delete_directory(path);
	free(path);

	return 0;
}



// Store included backup paths into a array
// -------------------------------------------------------
int save_paths (char **paths, char *filename)
{
	FILE *Fp;
	int i, j;
	char c;

	// Check for errors
	if (! file_exists(filename)) return -1;
	Fp = fopen(filename, "r");
	if (Fp == NULL) return -2;

	// Lettura e salvataggio dati
	i = 0; j = 0;

	while ((c = fgetc(Fp)) != EOF)
	{
		// If \r skips
		if (c == '\r') continue;

		// If return not found, adds character to string
		if (c != '\n')
		{
			paths[i][j] = c;
			j++;
		}

		// If return found, skip to next line
		else
		{
			// Adds a final / to path if needed
			correct_path(&paths[i]);
			
			i++;	// Next string
			j = 0;	// Go to string first character
		}
	}


	return 0;
}



// Prints a new empty line before exit
// -------------------------------------------------------
void before_exit()
{
	printf("\n\n");
}



// Counters initialization
// -------------------------------------------------------
int counters_initialization(struct _counters *counters)
{
	time(&counters->timestamp);
	counters->fresh.number = 0;
	counters->fresh.size = 0;
	counters->modified.number = 0;
	counters->modified.size = 0;
	counters->unmodified.number = 0;
	counters->unmodified.size = 0;
	counters->errors.number = 0;
	counters->errors.size = 0;
	counters->moved.number = 0;
	counters->moved.size = 0;

	return 0;
}



// Displays final report
// -------------------------------------------------------
int display_final_report(struct _counters *counters)
{
	unsigned long total_number, added_total_number;
	double total_size, added_total_size;
	
	total_number = 
		counters->errors.number
		+ counters->fresh.number
		+ counters->modified.number
		+ counters->moved.number
		+ counters->unmodified.number;

	total_size =
		counters->errors.size +
		counters->fresh.size +
		counters->modified.size +
		counters->moved.size +
		counters->unmodified.size;

	added_total_number = counters->modified.number + counters->fresh.number;
	added_total_size = counters->modified.size + counters->fresh.size;

	printf("\nBACKUP CONCLUDED - Elapsed time: %s\n", seconds_to_time(time(NULL) - counters->timestamp));
	printf("---------------------------------------------------------------------\n");
	printf("Unmodified       %15d%s\n", counters->unmodified.number, scale_bytes(counters->unmodified.size));
	printf("New              %15d%s\n", counters->fresh.number, scale_bytes(counters->fresh.size));
	printf("Modified         %15d%s\n", counters->modified.number, scale_bytes(counters->modified.size));
	printf("Moved            %15d%s\n", counters->moved.number, scale_bytes(counters->moved.size));
	printf("Errors           %15d%s\n", counters->errors.number, scale_bytes(counters->errors.size));
	printf("---------------------------------------------------------------------\n");
	printf("Total new files  %15ld%s\n", added_total_number, scale_bytes(added_total_size) );
	printf("Total files      %15ld%s", total_number, scale_bytes(total_size) );
}



// Check if a file is valid
// -------------------------------------------------------
int is_file_valid(struct dirent *file)
{
	if (strcmp(file->d_name, ".") == 0) return false;
	if (strcmp(file->d_name, "..") == 0) return false;
	if ((file->d_type != 8) && (file->d_type != 4)) return false;		// If it is not a file nor a directory, skip

	// TEMPORARY - Remove when a solution is found
	if (strpos(file->d_name, '$', 0) != -1) return false;

	// File types:
	//  0 - DT_UNKNOWN
	//  1 - DT_FIFO
	//  2 - DT_CHR
	//  4 - DT_DIR
	//  6 - DT_BLK
	//  8 - DT_REG
	// 10 - DT_LNK
	// 12 - DT_SOCK
	// 14 - DT_WHT	

	return true;
}



// Check if a path must be excluded
// -------------------------------------------------------
int is_path_excluded(char *path, char **excluded_paths)
{
	int i = 0;

	while (strlen(excluded_paths[i]) > 0)
	{
		printf("Checking %s against %s\n", path, excluded_paths[i]);
		if (strcmp(path, excluded_paths[i]) == 0) return true;
		
		i++;
	}

	return false;
}



// Open DB
// -------------------------------------------------------
int db_initialization(sqlite3 **db, char **db_filename, char **db_temp_filename, char *path)
{
	char *error;
	struct stat stats;

	join_strings(db_filename, 2, path, _DB_FILE);
	join_strings(db_temp_filename, 2, path, _TEMP_DB_FILE);

	if (file_exists(*db_temp_filename))
		unlink(*db_temp_filename);

	if (file_exists(*db_filename))
	{
		stat(*db_filename, &stats);
		file_copy(*db_filename, *db_temp_filename, stats);
	}

	sqlite3_open(*db_temp_filename, db);

	sqlite3_exec(*db, "CREATE TABLE IF NOT EXISTS file ( inode INT NOT NULL PRIMARY KEY, path TEXT NOT NULL, mtime VARCHAR(255) )", NULL, NULL, &error);
	if (error != NULL)
	{
		printf("%s\n", error);
		sqlite3_free(error);
		exit(-1);
	}

	sqlite3_exec(*db, "CREATE TABLE IF NOT EXISTS backup ( timestamp INT NOT NULL PRIMARY KEY, directory VARCHAR(255) NOT NULL, total_files INT NOT NULL, total_bytes INT NOT NULL, added_files INT NOT NULL, added_bytes INT NOT NULL )", NULL, NULL, &error);
	if (error != NULL)
	{
		printf("%s\n", error);
		sqlite3_free(error);
		exit(-1);
	}

	return 0;
}



// Checks in DB if exists the record of a file which points to the same inode,
// and makes the necessary insert/update operations.
// Returns true if file is present into DB, false otherwise
// -------------------------------------------------------
int exists_in_db(sqlite3 **db, char *path, char *latest_backup_root, struct stat stats, char **old_path)
{
	char *error;
	char *query;
	char *values;
	char *where;
	char **dati;
	char *inode_str;
	int row_number, col_number;
	long old_mtime;

	values = (char*) malloc( 5000 * sizeof(char) );
	where = (char*) malloc( 500 * sizeof(char) );
	inode_str = (char*) malloc( 50 * sizeof(char) );

	sprintf(inode_str, "%ld", stats.st_ino);
	clean_string(&path);

	join_strings(&query, 3, "SELECT path, mtime FROM file WHERE inode='", inode_str, "'");
	sqlite3_get_table(*db, query, &dati, &row_number, &col_number, &error);
	if (error != NULL)
	{
		printf("SQLite SELECT error: %s\n", error);
		sqlite3_free(error);
		exit(-1);
	}
	free(query);

	// If it is a new entry, insert new inode
	if (row_number == 0)
	{
		sprintf(values, "'%ld', '%s', '%ld'", stats.st_ino, path, stats.st_mtime);
	
		join_strings(&query, 3, "INSERT INTO file (inode, path, mtime) VALUES (", values, ")");
		free(values);
	
		sqlite3_exec(*db, query, NULL, NULL, &error);
		if (error != NULL)
		{
			printf("SQLite INSERT error:\n%s\n%s\n", query, error);
			sqlite3_free(error);
			free(query);
			exit(-1);
		}
		free(query);

		return false;
	}

	// If it is not a new entry, update inode path
	else
	{
		sscanf(dati[3], "%ld", &old_mtime);
		join_strings(old_path, 2, latest_backup_root, dati[2]);
		join_strings(&query, 5, "UPDATE file SET path = '", path, "' WHERE inode = '", inode_str, "'");
		sqlite3_exec(*db, query, NULL, NULL, &error);

		free(query);

		if (error != NULL)
		{
			printf("SQLite UPDATE error: %s\n", error);
			sqlite3_free(error);
			exit(-1);
		}
		
		// If last modification date is unmodified, hard link only
		if (stats.st_mtime == old_mtime) return true;

		// If last modification date is changed, make a new full copy of the file
		else return false;
	}
}




// Close and rename DB file
// -------------------------------------------------------
int close_db(sqlite3 **db, char *db_filename, char *db_temp_filename, char *path)
{
	char *new_filename;
	struct stat db_stats;

	unlink(db_filename);				// Delete old DB
	rename(db_temp_filename, db_filename);		// Rename temp DB to new DB
	chmod(db_filename, 0777);			// Set permissions
	
	join_strings(&new_filename, 3, path, "/", _DB_FILE);

	stat(db_filename, &db_stats);			// Set DB file stats
	file_copy(db_filename, new_filename, db_stats);	// Copy DB file to current backup directory

	free(new_filename);				// Free malloc() allocated memory

	sqlite3_close(*db);

	return 0;
}



// Write latest backup date into SQLite DB
// -------------------------------------------------------
int write_date_into_db (sqlite3 **db, char *current_date, unsigned long timestamp, struct _counters *counters)
{
	char *error;
	char **data;
	char *query;
	int row_number, col_number;
	unsigned long total_files, added_files;
	int64_t total_bytes, added_bytes;

	added_files = counters->fresh.number + counters->modified.number;
	total_files = added_files + counters->unmodified.number + counters->moved.number;
	added_bytes = counters->fresh.size + counters->modified.size;
	total_bytes = added_bytes + counters->unmodified.size + counters->moved.size;

	query = (char*) malloc(500 * sizeof(char));

	sprintf(query, "INSERT INTO backup (timestamp, directory, total_files, total_bytes, added_files, added_bytes) VALUES ('%ld', '%s', '%ld', '%ld', '%ld', '%ld')", timestamp, current_date, total_files, total_bytes, added_files, added_bytes);

	sqlite3_get_table(*db, query, &data, &row_number, &col_number, &error);
	if (error != NULL)
	{
		printf("error INSERT SQLite: %s", error);
		sqlite3_free(error);
		exit(-1);
	}

	return 0;
}




// Open latest backup DB and get backup directory
// -------------------------------------------------------
int get_latest_backup_dir (sqlite3 **db, char **string)
{
	char **data;
	int row_number, col_number;
	char *error;

	sqlite3_get_table(*db, "SELECT directory FROM backup WHERE 1 ORDER BY timestamp DESC LIMIT 1", &data, &row_number, &col_number, &error);
	if (error != NULL)
	{
		printf("error SELECT SQLite: %s", error);
		sqlite3_free(error);
		exit(-1);
	}

	if (row_number == 0)
	{
		join_strings(string, 0);
		return 0;
	}
	
	join_strings(string, 1, data[1]);

	return 0;
}



// Shows usage and help
// -------------------------------------------------------
void show_help()
{
	printf("SYNOPSIS\n");
	printf("\tmnemon <backup_dir> -i inclusions_file [-e exclusions_file]\n\n");

	printf("OPTIONS\n");
	printf("\t-i, --inclusions-file <filename>\n");
	printf("\t-e, --exclusions-file <filename>");
}



// Check shell parameters
// -------------------------------------------------------
int check_cli_arguments(int argc, char **argv, struct _parameters *parameters)
{
	int i;
	
	parameters->backup_dir = NULL;
	parameters->inclusions_filename = NULL;
	
	
	
	// Checks parameters count
	if (argc < 2)
	{
		show_help();
		exit(-1);
	}
	
	// If parameters come before backup dir, stops
	if (argv[1][0] == '-')
	{
		show_help();
		exit(-1);
	}
	
	// Stores backup dir in parameters structure
	join_strings(&parameters->backup_dir, 1, argv[1]);
	
	// Adds a final / to backup dir if needed
	correct_path(&parameters->backup_dir);

	
	
	// Searches for parameters
	for (i = 2; i < argc; i++)
	{
		// Inclusions file
		if (
			strcmp(argv[i], "-i") == 0
			|| strcmp(argv[i], "--inclusions-file") == 0
		)
		{
			if ((i + 1) >= argc)
			{
				printf("--inclusions-file parameter needs a filename");
				exit(-1);
			}
			else
			{
				join_strings(&parameters->inclusions_filename, 1, argv[i + 1]);
				
				// Checks if file exists
				if (! file_exists(parameters->inclusions_filename))
				{
					printf("Inclusions file %s doesn't exist", parameters->inclusions_filename);
					exit(-1);
				}
				
				i++;
			}
		}
		
		
		// Exclusions file
		if (
			strcmp(argv[i], "-e") == 0
			|| strcmp(argv[i], "--exclusions-file") == 0
		)
		{
			if ((i + 1) >= argc)
			{
				printf("--exclusions-file parameter needs a filename");
				exit(-1);
			}
			else
			{
				join_strings(&parameters->exclusions_filename, 1, argv[i + 1]);
				
				// Checks if file exists
				if (! file_exists(parameters->exclusions_filename))
				{
					printf("Exclusions file %s doesn't exist", parameters->exclusions_filename);
					exit(-1);
				}
				
				i++;
			}
		}
	}
	
	// Check inclusions file existence
	if (parameters->inclusions_filename == NULL)
	{
		printf("You must specify an inclusions list with -i or --inclusions-file parameter");
		
		exit(-1);
	}


	DIR *Dp;

	// Check if backup directory exists
	if ((Dp = opendir(parameters->backup_dir)) == NULL)
	{
		printf("[Error  ]\tBackup directory %s doesn't exist!", parameters->backup_dir);
		exit(-1);
	}
	else
	{
		closedir(Dp);
	}

	return 0;
}
