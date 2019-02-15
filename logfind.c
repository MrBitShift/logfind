#include <stdio.h>
#include <stdlib.h>
#include <dbg.h>

#define HELP "./help"

void show_help()
{
	int rc;
	FILE *helpf = fopen(HELP, "r");
	// make sure file opened succesfully
	check(helpf != NULL, "Failed to open help file.");
	// get the length of the file by moving cursor to end then reading cursor position
	fseek(helpf, 0, SEEK_END);
	long int length = ftell(helpf);
	rewind(helpf);
	
	// allocate help file and read
	char *help = calloc(length, sizeof(char));
	rc = fread(help, sizeof(char), length, helpf);
	// make sure expected number of bytes were read
	check(rc == length, "Failed to read help file.");
	
	printf(help);
	free(help);

error:
	fflush(helpf);
	fclose(helpf);
}

int main(int argc, char *argv[])
{
	show_help();
	return 0;
error:
	
	return -1;
}
