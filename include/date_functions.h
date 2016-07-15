// =======================================================
// date_functions.h
// -------------------------------------------------------
// Date-related functions
// =======================================================



// Prototypes
// *******************************************************

struct tm *get_time();
int get_current_date(char **string, unsigned long *timestamp);
char *seconds_to_time(time_t seconds);

// *******************************************************



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



// Returns current GMT human readable date string
// -------------------------------------------------------
int get_current_date(char **string, unsigned long *timestamp)
{
	struct tm *datetime;

	datetime = get_time();
	*timestamp = mktime(datetime);
	

	if ((*string = (char*) malloc(20 * sizeof(char))) == NULL)
	{
		printf("[Error  ]\tInsufficient physical memory");
		exit(-1);
	}

	sprintf(*string, "%d-%02d-%02d %02d.%02d.%02d", (datetime->tm_year + 1900), (datetime->tm_mon + 1), datetime->tm_mday, datetime->tm_hour, datetime->tm_min, datetime->tm_sec);

	return 0;
}
