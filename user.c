#include <stdio.h>
#include <stdlib.h>

#include "shared.h"

extern SharedMemory * shmem;

int main(int argc, char* argv[]) {

	allocateSharedMemory();

        printf("user.c: %d\n", shmem->simTime.sec);

        return EXIT_SUCCESS;


}

