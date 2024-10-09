#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#define BUFFER_SIZE 0x100000


int main(int argc, char *argv[])
{
	char input_path[1024];
	int64_t start, end, current;

	if(argc < 3)
	{
		fprintf(stderr, "Usage: extract filename start (decimal) end (decimal)\n");
		exit(1);
	}
	
	strcpy(input_path, argv[1]);
	sscanf(argv[2], "%lld", &start);
	sscanf(argv[3], "%lld", &end);

	FILE *fd;
	fd = fopen64(input_path, "r");
	if(!fd)
	{
		perror("fopen");
		exit(1);
	}

	char *buffer = malloc(BUFFER_SIZE);

	fprintf(stderr, "Extracting %s start=%lld end=%lld\n", 
		input_path, 
		start,
		end);
	fseeko64(fd, start, SEEK_SET);
	for(current = start; current < end; )
	{
		int fragment = BUFFER_SIZE;
		if(current + fragment > end) fragment = end - current;
		int bytes_read = fread(buffer, 1, fragment, fd);
		fwrite(buffer, bytes_read, 1, stdout);
		current += bytes_read;
	}
	fclose(fd);
	return 0;
}
