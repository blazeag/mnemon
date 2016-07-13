// =======================================================
// backup.h
// -------------------------------------------------------
// Backup function
// =======================================================



// Prototypes
// *********************************************************

int directory_backup (
	sqlite3 **db,				// SQLite DB pointer
	struct _parameters *parameters,		// Structure containing files counters
	struct _counters *counters,		// Structure containing files counters
	char *source_dir,			// Source directory
	char *latest_backup_dir,		// Latest backup directory
	char *destination_dir,			// New backup directory
	char *backup_dir,			// Backups mother directory
	char *latest_backup_root		// Latest backup root directory
);

// *********************************************************



// Single directory backup function
// -------------------------------------------------------
int directory_backup(sqlite3 **db, struct _parameters *parameters, struct _counters *counters, char *source_dir, char *latest_backup_dir, char *destination_dir, char *backup_dir, char *latest_backup_root)
{
	struct dirent **eps;			// Data about scanned directory files
	struct stat source_dir_stats;		// Current source directory properties
	struct stat source_stats;		// Source file properties
	struct stat latest_backup_stats;	// Latest backup file properties
	int i, n;				// Counters
	char *current_filename; 		// Current analyzing file name
	int64_t current_size;			// Current file size
	char *new_dir;				// New subdirectory path
	char *new_source_dir;			// Path to new subdirectory to be analyzed
	char *new_backup_dir;			// Path to same subdirectory in latest backup
	char *new_destination_dir;		// Path to same subdirectory in new backup
	char *source_file;			// Current file Path+Filename
	char *latest_backup_filename;		// Latest backup same file Path+Filename
	char *destination_filename;		// New backup same file Path+Filename
	char *old_path;				// Used to store moved file old path
	FILE *Fp;				// File pointer, used by file existence check function
	int fresh;				// Used as boolean to flag if a file exists in latest backup

	// Checks if current one is the path to backup directory
	if (strcmp(source_dir, backup_dir) == 0) return 0;

	// Checks if current one is an excluded path
	if (is_path_excluded(source_dir, parameters->excluded_paths)) return 0;

	// Creates destination dir and sets same permissions, owner and group of source one
	create_directory_tree(destination_dir);
	stat(source_dir, &source_dir_stats);
	copy_attributes(destination_dir, 4, source_dir_stats);

	// Stores into n the number of files/dirs and data into eps
	n = scandir (source_dir, &eps, NULL, alphasort);

	// Scan directory to list all files/dirs
	for (i = 0; i < n; i++)
	{
		// Stores current filename
		join_strings(&current_filename, 1, eps[i]->d_name);

		// If file is not valid (., .., links, etc), skips
		if (! is_file_valid(eps[i])) continue;

		// If it is a directory, repeat procedure recursively
		if (eps[i]->d_type == 4)
		{
			// Create new parameter strings for backup function
			join_strings(&new_dir, 2, current_filename, "/");
			join_strings(&new_source_dir, 2, source_dir, new_dir);
			join_strings(&new_backup_dir, 2, latest_backup_dir, new_dir);
			join_strings(&new_destination_dir, 2, destination_dir, new_dir);
			
			// Recursively calls backup function
			directory_backup(db, parameters, counters, new_source_dir, new_backup_dir, new_destination_dir, backup_dir, latest_backup_root);
			
			// Frees malloc() allocated strings
			free(new_dir);
			free(new_source_dir);
			free(new_backup_dir);
			free(new_destination_dir);
		}

		// If it is a file
		else
		{
			// Generate strings containing the paths to source/backup/destination files
			join_strings(&source_file, 2, source_dir, current_filename);
			join_strings(&latest_backup_filename, 2, latest_backup_dir, current_filename);
			join_strings(&destination_filename, 2, destination_dir, current_filename);
	
			// Generate stats for current file and same file in latest backup
			stat ( source_file, &source_stats ); time_t source_mtime = source_stats.st_mtime; current_size = source_stats.st_size;
			stat ( latest_backup_filename, &latest_backup_stats ); time_t latest_backup_mtime = latest_backup_stats.st_mtime;
	
			// Check if file exists in latest backup
			if ( file_exists(latest_backup_filename) )
			{
				fresh = false;
			}
			else
			{
				fresh = true;
			}

			// If file doesn't exists in latest backup, makes a full copy
			if (fresh)
			{
				// If in DB is present an entry with the same inode, then the file has been moved
				if (exists_in_db(db, source_file, latest_backup_root, source_stats, &old_path))
				{
					// Makes a new hard link between old path and new one
					link(old_path, destination_filename);
					printf("[Moved  ]\t%s\n", source_file);
					counters->moved.number++;
					counters->moved.size += current_size;

					free(old_path);
				}

				// Otherwise file is a new one, makes a full copy
				else
				{
					if ( file_copy(source_file,destination_filename,source_stats) == 0 )
					{
						printf("[Fresh  ]\t%s\n", source_file);
						counters->fresh.number++;
						counters->fresh.size += current_size;
					}
					else
					{
						printf("[Fail  N]\t%s\n", source_file);
						counters->errors.number++;
						counters->errors.size += current_size;
					}
				}
			}

			// If file is the same, but it has a different last modification timestamp, make a full copy
			else if (source_mtime != latest_backup_mtime)
			{
				if (file_copy(source_file, destination_filename, source_stats) == 0)
				{
					printf("[Changed]\t%s\n", source_file);
					counters->modified.number++;
					counters->modified.size += current_size;
				}
				else
				{
					printf("[Fail  D]\t%s\n", source_file);
					counters->errors.number++;
					counters->errors.size += current_size;
				}
			}

			// If file exists and it is exactly the same, only makes a hard link
			else
			{
				// Create hard link between latest backup file and new backup destination file
				if ( link (latest_backup_filename, destination_filename) == 0 )
				{
					// Outputs if file is not changed. Disabled to avoid flood
					//printf("[Unmodif]\t%s\n", source_file);
					counters->unmodified.number++;
					counters->unmodified.size += current_size;
				}
				else
				{
					printf("[Fail  L]\t%s\n", source_file);
					counters->errors.number++;
					counters->errors.size += current_size;
				}
			}

			// Free malloc() allocated memory
			free(source_file);
			free(latest_backup_filename);
			free(destination_filename);
		}

		// Free malloc() allocated memory
		free(current_filename);
	}

	return 0;
}
