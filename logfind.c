#include <stdio.h>
#include <stdlib.h>
#include <dbg.h>
#include <string.h>
#include <unistd.h>

#define HELP "./help"
#define HELP_FLAG "--help"
#define AND_FLAG "-a"
#define OR_FLAG "-o"
#define LOGFIND "/home/pi/.logfind"

enum Flag
{
	And, Or
};

char* read_file(char *filename, size_t size, int *returncode)
{
	int rc;
	FILE *file = fopen(filename, "r");
	// make sure file opened succesfully
	check(file != NULL, "Failed to open file %s", filename);
	// get the length of the file by moving cursor to end then reading cursor position
	fseek(file, 0, SEEK_END);
	long int length = (ftell(file) / size);
	// make sure to move cursor back to start
	rewind(file);
	
	// allocate memory for file and read
	char *out = calloc(length + 1, size); // + 1 for null terminator
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

int show_help()
{
	int *rc = calloc(1, sizeof(int));
	char *help = read_file(HELP, sizeof(char), rc);
	printf("\n%s\n", help);
	free(help);
	int stackrc = *rc;
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
	int *rc = calloc(1, sizeof(int));
	char *logfind = read_file(LOGFIND, sizeof(char), rc);
	check(logfind != NULL, "Make sure to create file ~/.logfind");
	printf("%s contents:\n%s", LOGFIND, logfind);
	
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
	
	// now that excess is trimmed, print values
	printf("Processed args: \n");
	for (i = 0; i < searches_count; i++)
	{
		printf("\t%s\n", searches[i]);
	}
	// add final newline
	printf("\n");
	
	// print what logic flag is
	if (flag == And)
	{
		printf("Logic flag is and.\n");
	}
	if (flag == Or)
	{
		printf("Logic flag is or.\n");
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
