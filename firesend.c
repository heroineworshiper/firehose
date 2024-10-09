// FIREHOSE stupid network utilities
// (c) 2003-2024 Adam Williams broadcast@earthling.net
// 
// This library is free software; you can redistribute it and/or modify it
// under the terms of the GNU Lesser General Public License as published
// by the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 
// USA


// gcc -O3 firesend.c firehose.c -o firesend -lpthread -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64


#include "firehose.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#define BUFFER_SIZE 0x100000





// Status information

typedef struct
{
	struct timeval start_time;
	struct timeval last_time;
	int64_t bytes_transferred;
	int64_t last_bytes_transferred;
	char filename[1024];
} status_t;


void init_status(status_t *status, char *filename)
{
	gettimeofday(&status->start_time, 0);
	gettimeofday(&status->last_time, 0);
	status->bytes_transferred = 0;
    status->last_bytes_transferred = 0;
	strcpy(status->filename, filename);
}

void update_status(status_t *status)
{
	struct timeval new_time;
	gettimeofday(&new_time, 0);
	if(new_time.tv_sec - status->last_time.tv_sec > 1)
	{
		printf("Sending %s %ld @ %ld bytes/sec           \r", 
			status->filename,
			(long)status->bytes_transferred,
			(long)status->last_bytes_transferred / 
			    (long)(new_time.tv_sec - status->last_time.tv_sec));
//			(int64_t)status->bytes_transferred / 
//			    (int64_t)(new_time.tv_sec - status->start_time.tv_sec));
		fflush(stdout);
		status->last_time = new_time;
        status->last_bytes_transferred = 0;
	};
}

void stop_status(status_t *status)
{
	struct timeval new_time;
	gettimeofday(&new_time, 0);
	printf("Sending %s Done.                              \n", status->filename);
	if(new_time.tv_sec - status->start_time.tv_sec > 1)
	{
		printf("%ld @ %ld bytes/sec           \n", 
			(long)status->bytes_transferred,
			(long)status->bytes_transferred / 
			    (int64_t)(new_time.tv_sec - status->start_time.tv_sec));
	};
}






int main(int argc, char *argv[])
{
	char *filenames[argc];
	char *sockets[argc];
	int result = 0;
	int i, j;
	char *buffer = calloc(1, BUFFER_SIZE);
	int total_sockets = 0;
	int total_filenames = 0;






// Parse args
	for(i = 1; i < argc; i++)
	{
		if(!strcmp(argv[i], "-i"))
		{
			if(i + 1 < argc)
			{
				i++;
				sockets[total_sockets] = malloc(strlen(argv[i]) + 1);
				strcpy(sockets[total_sockets], argv[i]);
				total_sockets++;
			}
			else
			{
				printf("-i needs an address of form hostname:port you Mor*on!\n");
				exit(1);
			}
		}
		else
		{
			filenames[total_filenames] = malloc(strlen(argv[i]) + 1);
			strcpy(filenames[total_filenames], argv[i]);
			total_filenames++;
		}
	}





// Usage
	if(!total_filenames || !total_sockets)
	{
		printf(
"Usage: %s -i host1:port1 -i host2:port2 ... <filename1> <filename2> ...\n",
argv[0]
		);
		exit(1);
	}




// Upload the files one by one
	for(i = 0; i < total_filenames; i++)
	{
		firehose_t *hose = firehose_new();
		FILE *in;

		for(j = 0; j < total_sockets; j++)
		{
			firehose_add_destination(hose, sockets[j]);
		}

		result = firehose_open_write(hose);

		if(!result)
		{
			if(!(in = fopen(filenames[i], "rb")))
			{
				perror("fopen");
				result = 1;
			}
		}
		
		if(!result)
		{
			status_t status;
			init_status(&status, filenames[i]);

// Send filename
			result = !firehose_write_data(hose, 
				filenames[i], 
				strlen(filenames[i]) + 1);

// Send data
			while(!feof(in) && !result)
			{
				long bytes_written = fread(buffer, 1, BUFFER_SIZE, in);
//printf("main 1 %d %02x\n", bytes_written, *(buffer));
				result = !firehose_write_data(hose, buffer, bytes_written);
				status.bytes_transferred += bytes_written;
                status.last_bytes_transferred += bytes_written;
				update_status(&status);
			}
			stop_status(&status);
		}

		firehose_delete(hose);
	}


	return 0;
}




