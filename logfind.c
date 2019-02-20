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
#define LOGFIND_UNFORMATTED "~/.logfind"

char* LOGFIND;

int glob_error(const char *path, int number)
{
	printf("The path \"%s\" does not exist or cannot be expanded. (errno: \"%s\")\n", path, strerror(number));
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
	if (out == GLOB_NOMATCH)
	{
		printf("No filenames match the expression \"%s\". Either it cannot be read, or does not exist.\n\n", patterns[0]);
	}
	else
	{
		check(out == 0, "Glob failed on pattern %s. (error: %s)", patterns[0], ((out == GLOB_NOSPACE) ? "GLOB_NOSPACE" :
			(out == GLOB_ABORTED) ? "GLOB_ABORTED" : "Unidentified error code."
			));
	}
	for (i = 1; patterns[i] != NULL; i++)
	{
		out = glob(patterns[i], flags | GLOB_APPEND, glob_error, buffer);
		if (out == GLOB_NOMATCH)
		{
			printf("No filenames match the expression \"%s\". Either it cannot be read, or does not exist.\n\n", patterns[i]);
			continue;
		}
		check(out == 0, "Glob failed on pattern %s. (error: %s)", patterns[i], ((out == GLOB_NOSPACE) ? "GLOB_NOSPACE" :
			(out == GLOB_ABORTED) ? "GLOB_ABORTED" : "Unidentified error code."
			));
	}
	
	return 0;
error:
	return 1;
}

int get_logfind()
{
	// initialize stuff
	int rc;
	int i;
	glob_t *result;
	char **unformatted;
	
	// set values
	result = calloc(1, sizeof(glob_t)); // we calloc so that we are not throwing in a null pointer to get_glob()
	unformatted = calloc(2, sizeof(char*)); // 2 so that there is space for null terminator
	unformatted[0] = LOGFIND_UNFORMATTED;
	
	rc = get_glob(unformatted, result);
	check(rc == 0, "Error in get_logfind() while trying to expand the path %s.", unformatted[0]);
	
	// get length of result
	for (i = 0; result->gl_pathv[0][i] != '\0'; i++);
	
	// assign output to LOGFIND through strcpy
	LOGFIND = calloc(i + 1, sizeof(char)); // + 1 for null terminator
	strcpy(LOGFIND, result->gl_pathv[0]);
	
	free(unformatted);
	globfree(result);
	
	return 0;
error:
	if (result != NULL)
	{
		globfree(result);
	}
	if (unformatted != NULL)
	{
		free(unformatted);
	}
	return 1;
}

// initializes global vars.
int initialize()
{
	return get_logfind();
}

// cleans global vars
void clean()
{
	free(LOGFIND);
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
	check(file != NULL, "Failed to open file \"%s\"", filename);
	// get the length of the file by moving cursor to end then reading cursor position
	fseek(file, 0, SEEK_END);
	long int length = (ftell(file) / size);
	// make sure to move cursor back to start
	rewind(file);
	
	// allocate memory for file and read
	out = calloc(length + 1, size); // + 1 for null terminator
	rc = fread(out, size, length, file);
	// make sure expected number of bytes were read
	check(rc == length, "Failed to read file \"%s\"", filename);
	
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

int get_logfind_files(char ***out)
{
	int i; // used for loops (duh)
	int b; // used to find length of null terminated strings;
	char *logfind; // used to store contents of LOGFIND
	char *filename; // used to store individual filenames extracted from logfind
	char *rest; // used to store the rest of the contents of logfind for strtok_r
	glob_t *glob_result = calloc(1, sizeof(glob_t)); // store result of glob
	
	// read file, check its good and then print
	logfind = read_file(LOGFIND, sizeof(char));
	check(logfind != NULL, "Either \"%s\" does not exist or cannot be read.", LOGFIND);
	
	// change working directory to / so that relative paths are not handled.
	chdir("/");
	
	// set rest to logfind for strtok_r
	rest = logfind;
	
	// now loop through it line by line using strtok_r and search in each line
	for (i = 1, filename = strtok_r(rest, "\n", &rest); filename != NULL; i++, filename = strtok_r(rest, "\n", &rest))
	{
		*out = realloc(*out, (i + 1) * sizeof(char*)); // + 1 for null. don't worry on first loop this will just act as normal malloc
		// get filename length
		for (b = 0; filename[b] != '\0'; b++);
		(*out)[i - 1] = calloc(b + 1, sizeof(char*)); // + 1 for null
		strcpy((*out)[i - 1], filename); // strcpy because its safer
		(*out)[i] = NULL; // make sure to assign null because realloc doesn't clear mem.
	}
	
	// now we have the filenames, glob them
	check(get_glob(*out, glob_result) == 0, "Error expanding patterns from \"%s\".", LOGFIND);
	
	*out = glob_result->gl_pathv;
	
	free(logfind);
	
	return 0;
	
error:
	free(logfind);
	
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
	int rc; // used to store return codes
	char **filenames; // used to store filenames returned from get_logfind_files
	//char *file;
	
	filenames = calloc(1, sizeof(char*)); // allocate so we don't pass in a null pointer
	check(get_logfind_files(&filenames) == 0, "Could not get list of files from \"%s\".", LOGFIND); // get the filenames and check it succeeded
	
	// change working directory to / so that relative paths are not handled.
	chdir("/");
	
	// now loop through each file
	for (i = 0; filenames[i] != NULL; i++)
	{
		char *filename = filenames[i];
		char *file = read_file(filename, sizeof(char));
		if (file == NULL)
		{
			printf("The file \"%s\" does not exist or cannot be read. Make sure paths are absolute. \nSkipping to next file.\n\n", filename);
			continue;
		}
		// begin search
		printf("Beginning search in file \"%s\"\n", filename);
		
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
		printf("Ending search in file \"%s\"\n\n", filename);
		free(file);
	}
	
	printf("Search completed.\n");
	
	return 0;
	
error:
	//if (file != NULL)
	//{
	//	free(file);
	//}
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
	check(initialize() == 0, "Operation failed. Exiting.\n");
	check(process_args(argc, argv) == 0, "Operation failed. Exiting.\n");
	clean();
	return 0;
error:
	
	return 1;
}
 
