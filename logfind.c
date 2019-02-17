#include <stdio.h>
#include <stdlib.h>
#include <dbg.h>
#include <string.h>
#include <unistd.h>

#define HELP "./help"
#define INCORRECT "./incorrect"
#define HELP_FLAG "--help"
#define AND_FLAG "-a"
#define OR_FLAG "-o"
#define LOGFIND "/home/pi/.logfind"

enum Flag
{
	And, Or
};

void* read_file(char *filename, size_t size, int *returncode)
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
	*returncode = 0;
	return out;

error:
	
	if (file != NULL)
	{
		fflush(file);
		fclose(file);
	}
	*returncode = 1;
	return NULL;
}

int show_incorrect()
{
	int *rc = calloc(1, sizeof(int));
	char *incorrect = read_file(INCORRECT, sizeof(char), rc);
	int stackrc = *rc;
	check(stackrc == 0, "Cannot open incorrect usage file.");
	
	printf("\n%s\n", incorrect);
	free(incorrect);
	free(rc);
	return stackrc;
	
error:
	free(incorrect);
	free(rc);
	return 1;
}

int show_help()
{
	int *rc = calloc(1, sizeof(int));
	char *help = read_file(HELP, sizeof(char), rc);
	int stackrc = *rc;
	check(stackrc == 0, "Cannot open help file.");
	
	printf("\n%s\n", help);
	free(help);
	free(rc);
	return stackrc;

error:
	free(help);
	free(rc);
	return 1;
}

int search(char **terms, enum Flag logic)
{
	// initialize stuff
	int i;
	int *rc = calloc(1, sizeof(int));
	char *logfind;
	char *filename;
	
	// print stuff for debugging
	printf("Processed args: \n");
	for (i = 0; terms[i] != NULL; i++)
	{
		printf("\t%s\n", terms[i]);
	}
	if (logic == And)
	{
		printf("Logic flag is and.\n");
	}
	if (logic == Or)
	{
		printf("Logic flag is or.\n");
	}
	printf("\n");
	
	// read file, check its good and then print
	logfind = read_file(LOGFIND, sizeof(char), rc);
	check(logfind != NULL, "Make sure to create file %s", LOGFIND);
	printf("%s contents:\n%s\n", LOGFIND, logfind);
	
	// now loop through it line by line using strtok and search in each line
	filename = strtok(logfind, "\n");
	for (i = 0; filename != NULL; i++)
	{
		check(access(filename, R_OK), "File %s in %s does not exist or cannot be read.\n", filename, LOGFIND);
		// begin search
		printf("Beginning search in file %s\n", filename);
		
		// end of search
		printf("Ending search in file %s\n", filename);
		filename = strtok(NULL, "\n");
	}
	
	// clean up
	free(rc);
	free(logfind);
	return 0;
	
error:
	free(rc);
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
		rc = show_help();
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
	process_args(argc, argv);
	return 0;
error:
	
	return 1;
}
