#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "shared.h"

extern SharedMemory * shmem;

int main(int argc, char* argv[]) {

	allocateSharedMemory();

	shmem->simTime.sec = 4;
	
	if(fork() == 0)
		execl("./user", "user", (char*)NULL);

	sleep(2);

	releaseSharedMemory();

        return EXIT_SUCCESS;


}
