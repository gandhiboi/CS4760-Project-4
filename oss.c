#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <unistd.h>

#include "shared.h"

SharedMemory *shared = NULL;
Message msg;


static int pMsgQID;
static int cMsgQID;


int main(int argc, char* argv[]) {

	allocateSharedMemory();
	allocateMessageQueues();
	
	shared = shmemPtr();
	
	pMsgQID = parentMsgQptr();
	cMsgQID = childMsgQptr();

	shared->simTime.sec = 4;

	msg.mtype = 1;
	
	printf("parentMsgQID: %d\n", pMsgQID);
	
	strcpy(msg.mtext, "hello from oss.c");

	if(msgsnd(pMsgQID, &msg, sizeof(Message), IPC_NOWAIT) == -1) {
		perror("msgsnd failed");
	}
			
	if(fork() == 0) {
		execl("./user", "user", (char*)NULL);
	}

	printf("childMsgQID: %d\n", cMsgQID);

	fflush(stdout);	
	msgrcv(cMsgQID, &msg, sizeof(Message), 1, 0);
	printf("from user: %s\n", msg.mtext);

	sleep(2);

	releaseSharedMemory();
	deleteMessageQueues();

        return EXIT_SUCCESS;

}

