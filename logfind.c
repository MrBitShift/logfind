#include <stdio.h>
#include <stdlib.h>
#include <dbg.h>
#include <string.h>

#define HELP "./help"
#define HELP_FLAG "--help"
#define AND_FLAG "-a"
#define OR_FLAG "-o"

enum Flag
{
	And, Or
};

char* read_file(char *filename, size_t size)
{
	int rc;
	FILE *file = fopen(filename, "r");
	// make sure file opened succesfully
	check(file != NULL, "Failed to open file %s", filename);
	// get the length of the file by moving cursor to end then reading cursor position
	fseek(file, 0, SEEK_END);
	long int length = ftell(file) / size;
	// make sure to move cursor back to start
	rewind(file);
	
	// allocate memory for file and read
	char *out = calloc(length, size);
	rc = fread(out, size, length, file);
	// make sure expected number of bytes were read
	check(rc == length, "Failed to read file %s", filename);
	
	fflush(file);
	fclose(file);
	return out;

error:
	
	fflush(file);
	fclose(file);
	return NULL;
}

int show_help()
{
	char *help = read_file(HELP, sizeof(char));
	printf(help);
	free(help);
	return 0;

error:
	free(help);
	return -1;
}

int process_args(int argc, char *argv[])
{
	int i;
	int searches_count = 0;
	// default search logic is and
	enum Flag flag = And;
	// allocate
	char **searches = calloc(argc, sizeof(char*));
	for (i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], HELP_FLAG) == 0)
		{
			show_help();
			i = argc;
			break;
		}
		
		else if (strcmp(argv[i], OR_FLAG) == 0)
		{
			flag = Or;
			i = argc;
			break;
		}
		
		else if (strcmp(argv[i], AND_FLAG) == 0)
		{
			flag = And;
			i = argc;
			break;
		}
		
		else
		{
			// increment searches_count and assign value to searches
			searches_count++;
			searches[searches_count - 1] = argv[i];
		}
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
	free(searches);
	return 0;
error:
	free(searches);
}

int main(int argc, char *argv[])
{
	int i;
	printf("All received args:\n");
	for (i = 0; i < argc; i++)
	{
		printf("\t%s\n", argv[i]);
	}
	// add final new line
	printf("\n");
	process_args(argc, argv);
	return 0;
error:
	
	return -1;
}
