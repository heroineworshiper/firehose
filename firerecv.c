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

#include "firehose.h"
#include <stdio.h>
#include <stdlib.h>

#define BUFFER_SIZE 0x100000

static void receiver_loop(void *ptr)
{

	firehose_receiver_t *receiver = (firehose_receiver_t*)ptr;
	char *buffer = calloc(1, BUFFER_SIZE);
	int i, result = 0;
	FILE *out;

// Read filename
	i = 0;
	do
	{
		result = !firehose_read_data(receiver, 
			buffer + i, 
			1);
		i++;
	}while(buffer[i - 1] != 0 && !result);

// Open output file
	if(!result && !(out = fopen(buffer, "wb")))
	{
		perror("fopen");
		result = 1;
	}

// Read off the pipe
	if(!result)
	{
		printf("Receiving %s...", buffer);
		fflush(stdout);
		while(!firehose_eof(receiver) && !result)
		{
			long bytes_read;
			bytes_read = firehose_read_data(receiver, 
				buffer, 
				BUFFER_SIZE);
			result = !fwrite(buffer, bytes_read, 1, out);
		}

		fclose(out);
		printf("Done.\n");
	}

// Stop reading
	firehose_delete_receiver(receiver);
	free(buffer);
}




int main(int argc, char *argv[])
{
	int i;
	firehose_t *hose;
	int ports[argc];
	int total_ports = 0;
	int result = 0;

	for(i = 1; i < argc; i++)
	{
		if(!strcmp(argv[i], "-i"))
		{
			if(i + 1 < argc)
			{
				i++;
				ports[total_ports] = atol(argv[i]);
				total_ports++;
			}
			else
			{
				printf("-i needs a port number you Mor*on!\n");
				exit(1);
			}
		}
	}


	if(!total_ports)
	{
		printf(
"Usage: %s -i port1 -i port2 ...\n",
argv[0]
		);
		exit(1);
	}

	hose = firehose_new();


// Set up ports

	for(i = 0; i < total_ports; i++)
	{
		firehose_add_receiver(hose, ports[i]);
	}





	while(!result)
	{
       pthread_attr_t  attr;
		firehose_receiver_t *receiver;
		pthread_t tid;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

// Wait for connection
		result = firehose_open_read(hose, &receiver);

// Start receiving in background
		if(!result) 
			pthread_create(&tid, &attr, (void*)receiver_loop, receiver);
	}




	firehose_delete(hose);
	return 0;
}

