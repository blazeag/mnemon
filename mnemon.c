/***********************************************************************
 mnemon v0.1.0 - 2016-07-09 11:48
 ----------------------------------------------------------------------
 Andrea Gardoni
 ***********************************************************************/

// gcc mnemon.c -o mnemon -lsqlite3
// If sqlite3.h error: apt-get install libsqlite3-dev

#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64

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
#define _TEMPORARY_ROOT_FILE "/_mnemontemp_"
#define _TEMP_BACKUP_DIR "_mnemontempdir_/"
#define _DB_FILE "mnemon.db"
#define _TEMP_DB_FILE "_mnemon.db"

struct _dati
{
	int number;
	double size;
};

struct _counters
{
	time_t timestamp;
	struct _dati fresh;
	struct _dati modified;
	struct _dati unmodified;
	struct _dati errors;
	struct _dati moved;
};

#include "libs/functions.h"
#include "libs/procedures.h"
#include "libs/backup.h"



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
	sqlite3 *db;			// DB pointer
	char *db_filename;		// SQLite database file name
	char *temp_db_filename;		// Temporary SQLite database file name
	char *includes_filename;	// Included path list file name
	char **included_paths;		// Array containing included paths
	int i;

	printf("\n");			
	atexit(before_exit);		// At exit, output a new empty line

	// Check to be root
	if (! is_root() )
	{
		printf("[Error  ]\tYou must be root");
		exit(-1);
	}

	// Check parameters count
	if (argc != 2)
	{
		printf("\t$ %s <backup dir>", argv[0]);
		exit(-1);
	}

	// Save and allocate memory for included paths
	included_paths = (char**) malloc(100 * sizeof(char*));

	for (i=0; i<100; i++)
	{
		included_paths[i] = (char*) malloc(100 * sizeof(char));
	}

	join_strings(&includes_filename, 1, "included_paths.txt");
	save_included_paths(included_paths, includes_filename);

	// Corrects paths if needed
	correct_path(&argv[1]);

	// Checks cli arguments
	check_cli_arguments(argv);

	// If last backup interrupted before finish, deletes temporary directory
	delete_temp_directory(argv[1]);

	// SQLite DB initialization
	db_initialization(&db, &db_filename, &temp_db_filename, argv[1]);

	// Stores current GMT date into a specified string
	save_current_date(&str_current_date, &timestamp);
	get_latest_backup_dir(&db, &latest_backup_timestamp);
	join_strings(&latest_backup_root, 2, argv[1], latest_backup_timestamp);
	
	// Creates and initializes strucutre containing files counters
	counters_initialization(&counters);
	
	i = 0;

	while (strlen(included_paths[i]) > 0)
	{
		// Generate paths to latest backup directory and current backup directory
		join_strings(&temp_backup_root, 2, argv[1], _TEMP_BACKUP_DIR);
		join_strings(&latest_backup_dir, 3, argv[1], latest_backup_timestamp, included_paths[i]);
		join_strings(&current_backup_dir, 3, argv[1], _TEMP_BACKUP_DIR, included_paths[i]);
	
		// Execute backup as indicated in parameters
		directory_backup(&db, &counters, included_paths[i], latest_backup_dir, current_backup_dir, argv[1], latest_backup_root);

		i++;
	}

	// Sets same mother directory attributes to temporary one and rename it
	stat(argv[1], &backup_root_stats);
	copy_attributes(temp_backup_root, 4, backup_root_stats);
	join_strings(&current_backup_root, 2, argv[1], str_current_date);
	rename(temp_backup_root, current_backup_root);

	write_date_into_db (&db, str_current_date, timestamp, &counters);	// Writes current backup data to DB
	close_db(&db, db_filename, temp_db_filename, current_backup_root);

	display_final_report(&counters);					// Displays final report

	// Frees malloc() allocated memory
	free(temp_backup_root);
	free(latest_backup_root);
	free(current_backup_root);
	free(latest_backup_dir);
	free(current_backup_dir);
	free(str_current_date);
	free(latest_backup_timestamp);

	return 0;
}
