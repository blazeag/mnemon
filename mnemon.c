/*****************************************************************************
 * mnemon - Yet another hard link based snapshot backup system
 *****************************************************************************
 * (c) 2009-2016 Andrea Gardoni <andrea.gardonitwentyfour@gmail.com> minus 24
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program;
 * if not, see <http://www.gnu.org/licenses/lgpl-2.1.html>.
 *****************************************************************************/

// Compile instructions:
// gcc mnemon.c -o mnemon -lsqlite3
// If sqlite3.h error: apt-get install libsqlite3-dev

#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64
#define _COPY_BLOCK_SIZE 1024 * 1024

#include <sqlite3.h>
#include <time.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <stdarg.h>

#define true 1
#define false !true

// Paths and filenames
#define _TEMP_ROOT_FILE "/_mnemontemp_"
#define _TEMP_BACKUP_DIR "_mnemontempdir_/"
#define _DB_FILE "mnemon.db"
#define _TEMP_DB_FILE "_mnemon.db"

// Struct to store file data
struct _file_data
{
	int number;
	double size;
};

// Struct to store overall statistics
struct _counters
{
	time_t timestamp;
	struct _file_data fresh;
	struct _file_data modified;
	struct _file_data unmodified;
	struct _file_data errors;
	struct _file_data moved;
};

// Struct to store global parameters
struct _parameters
{
	char *backup_dir;
	
	int verbose_mode;
	
	char *inclusions_filename;
	char **included_paths;

	char *exclusions_filename;
	char **excluded_paths;
};



#include "include/string_functions.h"
#include "include/date_functions.h"
#include "include/file_functions.h"
#include "include/procedures.h"
#include "include/backup.h"



// MAIN
// -------------------------------------------------------
int main( int argc, char **argv )
{
	char *str_current_date;		// String containing latest backup date
	unsigned long timestamp;	// Latest backup timestamp
	char *latest_backup_timestamp; 	// Previous backup timestamp
	char *temp_backup_root;		// Current backup temporary root directory
	char *latest_backup_root;	// Latest backup root directory
	char *current_backup_root;	// Final current backup directory name
	char *latest_backup_dir;	// Previous backup directory
	char *current_backup_dir;	// Directory to store current backup
	struct stat backup_root_stats;	// Stats of all backups root
	struct _counters counters;	// Counters
	struct _parameters parameters;	// Parameters
	sqlite3 *db;			// DB pointer
	char *db_filename;		// SQLite database file name
	char *temp_db_filename;		// Temporary SQLite database file name
	int i;

	printf("\n");			
	atexit(before_exit);		// At exit, output a new empty line

	// Check to be root
	if (! is_root() )
	{
		printf("[Error  ]\tYou must be root");
		exit(-1);
	}


	// Check cli arguments
	check_cli_arguments(argc, argv, &parameters);
	
	// Allocate memory for paths arrays
	parameters.included_paths = (char**) malloc(100 * sizeof(char*));
	parameters.excluded_paths = (char**) malloc(100 * sizeof(char*));

	for (i = 0; i < 100; i++)
	{
		parameters.included_paths[i] = (char*) malloc(100 * sizeof(char));
		parameters.excluded_paths[i] = (char*) malloc(100 * sizeof(char));
	}

	// Get included paths and store them into parameters array
	save_paths(parameters.included_paths, parameters.inclusions_filename);

	// Get excluded paths and store them into parameters array
	save_paths(parameters.excluded_paths, parameters.exclusions_filename);

	// If last backup interrupted before finish, delete temporary directory
	delete_temp_directory(parameters.backup_dir);

	// SQLite DB initialization
	db_initialization(&db, &db_filename, &temp_db_filename, parameters.backup_dir);

	// Store current GMT date into a specified string
	get_current_date(&str_current_date, &timestamp);
	get_latest_backup_dir(&db, &latest_backup_timestamp);
	join_strings(&latest_backup_root, 2, parameters.backup_dir, latest_backup_timestamp);
	
	// Create and initialize strucutre containing files counters
	counters_initialization(&counters);
	
	i = 0;

	while (strlen(parameters.included_paths[i]) > 0)
	{
		// Generate paths to latest backup directory and current backup directory
		join_strings(&temp_backup_root, 2, parameters.backup_dir, _TEMP_BACKUP_DIR);
		join_strings(&latest_backup_dir, 3, parameters.backup_dir, latest_backup_timestamp, parameters.included_paths[i]);
		join_strings(&current_backup_dir, 3, parameters.backup_dir, _TEMP_BACKUP_DIR, parameters.included_paths[i]);
	
		// Execute backup as indicated in parameters
		directory_backup(&db, &parameters, &counters, parameters.included_paths[i], latest_backup_dir, current_backup_dir, parameters.backup_dir, latest_backup_root);

		i++;
	}

	// Set same mother directory attributes to temporary one and rename it
	stat(parameters.backup_dir, &backup_root_stats);
	copy_attributes(temp_backup_root, 4, backup_root_stats);
	join_strings(&current_backup_root, 2, parameters.backup_dir, str_current_date);
	rename(temp_backup_root, current_backup_root);

	write_date_into_db (&db, str_current_date, timestamp, &counters);	// Write current backup data to DB
	close_db(&db, db_filename, temp_db_filename, current_backup_root);

	display_final_report(&counters);					// Display final report

	// Free malloc() allocated memory
	free(temp_backup_root);
	free(latest_backup_root);
	free(current_backup_root);
	free(latest_backup_dir);
	free(current_backup_dir);
	free(str_current_date);
	free(latest_backup_timestamp);

	return 0;
}
