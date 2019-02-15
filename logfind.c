#include <stdio.h>
#include <stdlib.h>
#include <dbg.h>

#define HELP "./help"
#define HELP_FLAG "--help"

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

int main(int argc, char *argv[])
{
	show_help();
	return 0;
error:
	
	return -1;
}
