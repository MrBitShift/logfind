#include <stdio.h>
#include <stdlib.h>
#include <dbg.h>
#include <string.h>
#include <unistd.h>
#include <glob.h>

#define HELP "./.help"
#define INCORRECT "./.incorrect"
#define HELP_FLAG "--help"
#define AND_FLAG "-a"
#define OR_FLAG "-o"

char* LOGFIND;

void get_logfind()
{
	char *homedir = getenv("HOME");
	char *file = "/.logfind";
	LOGFIND = calloc(strlen(homedir) + strlen(file) + 1, sizeof(char)); // + 1 for null terminator
	strcpy(LOGFIND, homedir);
	strcat(LOGFIND, file);
}

// initializes global vars.
int initialize()
{
	get_logfind();
	return 0;
}

enum Flag
{
	And, Or
};

void* read_file(char *filename, size_t size)
{
	// initialize stuff
	int rc;
	void *out;
	
	FILE *file = fopen(filename, "r");
	// make sure file opened succesfully
	check(file != NULL, "Failed to open file %s", filename);
	// get the length of the file by moving cursor to end then reading cursor position
	fseek(file, 0, SEEK_END);
	long int length = (ftell(file) / size);
	// make sure to move cursor back to start
	rewind(file);
	
	// allocate memory for file and read
	out = calloc(length + 1, size); // + 1 for null terminator
	rc = fread(out, size, length, file);
	// make sure expected number of bytes were read
	check(rc == length, "Failed to read file %s", filename);
	
	fflush(file);
	fclose(file);
	return out;

error:
	
	if (file != NULL)
	{
		fflush(file);
		fclose(file);
	}
	return NULL;
}

int show_incorrect()
{
	char *incorrect = read_file(INCORRECT, sizeof(char));
	check(incorrect != NULL, "Cannot open incorrect usage file.");
	
	printf("\n%s\n", incorrect);
	free(incorrect);
	return 0;
	
error:
	free(incorrect);
	return 1;
}

int show_help()
{
	char *help = read_file(HELP, sizeof(char));
	check(help != NULL, "Cannot open help file.");
	
	printf("\n%s\n", help);
	free(help);
	return 0;

error:
	free(help);
	return 1;
}

int glob_error(const char *path, int number)
{
	printf("The path %s does not exist or cannot be expanded. (errno: %s)\n", path, strerror(number));
	return 0; // return 0 so glob doesn't quit and continues on other paths.
}

int get_glob(char **patterns, glob_t *buffer)
{
	// initialize stuff
	int flags; // option flags for glob
	int out;
	int i;
	
	// set values
	flags = GLOB_TILDE_CHECK;
	
	out = glob(patterns[0], flags, glob_error, buffer);
	for (i = 1; patterns[i] != NULL; i++)
	{
		check(out == 0, "Error in get_glob.");
		out = glob(patterns[i], flags | GLOB_APPEND, glob_error, buffer);
	}
	
	return 0;
error:
	globfree(buffer);
	return 1;
}

int contains_and(char *text, char **terms)
{
	// initialize stuff
	int i;
	
	// remember this is and logic so you'll have to know all terms
	for (i = 0; terms[i] != '\0'; i++)
	{
		if (strstr(text, terms[i]) == NULL)
		{
			goto error;
		}
	}
	// success
	return 0;
error:
	// does not match
	return 1;
}

int contains_or(char *text, char **terms)
{
	// initialize stuff
	int i;
	
	// remember this is or logic so you don't have to know all terms are in this text
	for (i = 0; terms[i] != '\0'; i++)
	{
		if (strstr(text, terms[i]) != NULL)
		{
			goto success;
		}
	}
	
	// none found: error
	return 1;
success:
	// success
	return 0;
}

int search_and(char *text, char **terms)
{
	// initialize stuff
	int i; // used to loop through terms
	int l; // used to loop through lines
	char *line; // used to store contents of each line
	char *rest; // used to store remaining portion of text for strtok_r
	
	// assign rest
	rest = text;
	
	if (contains_and(text, terms) != 0)
	{
		goto error;
	}
	
	// File has all terms. Now loop through one term at a time and split by line and search
	for (i = 0; terms[i] != '\0'; i++)
	{
		for (l = 1, line = strtok_r(rest, "\n", &rest); line != NULL; l++, line = strtok_r(rest, "\n", &rest))
		{
			char *substring;
			substring = strstr(line, terms[i]);
			if (substring == NULL)
			{
				continue;
			}
			size_t column = (size_t)substring - (size_t)line;
			printf("Found \"%s\" on line %d column %d\n", terms[i], l, column);
		}
		// reinitialize
		rest = text;
	}
	
	// success
	return 0;
error:
	// failure
	return 1;
}

int search_or(char *text, char **terms)
{
	// initialize stuff
	int i; // used to loop through terms
	int l; // used to loop through lines
	char *line; // used to store contents of each line
	char *rest; // used to store remaining portion of text for strtok_r
	
	// assign rest
	rest = text;
	
	if (contains_or(text, terms) != 0)
	{
		goto error;
	}
	
	// now just loop. REMEMBER: it doesn't need all terms so you don't have to check that it does.
	for (i = 0; terms[i] != '\0'; i++)
	{
		for (l = 1, line = strtok_r(rest, "\n", &rest); line != NULL; l++, line = strtok_r(rest, "\n", &rest))
		{
			char *substring;
			substring = strstr(line, terms[i]);
			if (substring == NULL)
			{
				continue;
			}
			// it was found. print it
			size_t column = (size_t)substring - (size_t)line;
			printf("Found \"%s\" on line %d column %d\n", terms[i], l, column);
		}
		// reinitialize
		rest = text;
	}
	
	// success
	return 0;
error:
	return 1;
}

int search(char **terms, enum Flag logic)
{
	// initialize stuff
	int i; // used for loops (duh)
	char *logfind; // used to store contents of LOGFIND
	char *filename; // used to store filenames extracted from logfind
	char *rest; // used to store the rest of the contents of logfind for strtok_r
	char *file; // used to store contents of files from logfind
	int rc; // used to store return codes
	
	// read file, check its good and then print
	logfind = read_file(LOGFIND, sizeof(char));
	check(logfind != NULL, "Either %s does not exist or cannot be read.", LOGFIND);
	
	// change working directory to / so that relative paths are not handled.
	chdir("/");
	
	// set rest to logfind for strtok_r
	rest = logfind;
	
	// now loop through it line by line using strtok_r and search in each line
	for (i = 0, filename = strtok_r(rest, "\n", &rest); filename != NULL; i++, filename = strtok_r(rest, "\n", &rest))
	{
		file = read_file(filename, sizeof(char));
		if (file == NULL)
		{
			printf("The file %s does not exist or cannot be read. Make sure paths are absolute. \nSkipping to next file.\n\n", filename);
			continue;
		}
		// begin search
		printf("Beginning search in file %s\n", filename);
		
		if (logic == And)
		{
			rc = search_and(file, terms);
		}
		else if (logic == Or)
		{
			rc = search_or(file, terms);
		}
		
		if (rc == 1)
		{
			printf("File did not match the search.\n");
		}
		
		// end of search in this file
		printf("Ending search in file %s.\n\n", filename);
		free(file);
	}
	
	printf("Search completed.\n");
	
	// clean up
	free(logfind);
	return 0;
	
error:
	if (file != NULL)
	{
		free(file);
	}
	free(logfind);
	return 1;
}

int process_args(int argc, char *argv[])
{
	int i;
	int rc;
	int searches_count = 0;
	// default search logic is and
	enum Flag flag = And;
	// allocate
	char **searches = calloc(argc, sizeof(char*));
	for (i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], HELP_FLAG) == 0)
		{
			int rc;
			rc = show_help();
			free(searches);
			return rc;
			break;
		}
		
		else if (strcmp(argv[i], OR_FLAG) == 0)
		{
			flag = Or;
			i++;
			break;
		}
		
		else if (strcmp(argv[i], AND_FLAG) == 0)
		{
			flag = And;
			i++;
			break;
		}
		
		else
		{
			// increment searches_count and assign value to searches
			searches_count++;
			searches[searches_count - 1] = argv[i];
		}
	}
	
	// check to make sure that there aren't extra args trailing after logic switch
	// and check to make sure that there were search terms given
	if ((i != argc) || (searches_count < 1)) {
		rc = show_incorrect();
		free(searches);
		return rc;
	}
	
	// call search
	search(searches, flag);
	
	free(searches);
	return 0;
error:
	free(searches);
	return 1;
}

int main(int argc, char *argv[])
{
	initialize();
	chdir("/");
	glob_t *results = calloc(1, sizeof(glob_t));
	char *pattern[] = {"~/.logfind", "~/*.png"};
	get_glob(pattern, results);
	int i;
	for (i = 0; i < results->gl_pathc; i++)
	{
		printf("Result %d: %s\n", i, results->gl_pathv[i]);
	}
	return 0;
error:
	
	return 1;
}
