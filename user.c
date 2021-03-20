#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <unistd.h>

#include "shared.h"

SharedMemory * shared = NULL;
static int pMsgQID;
static int cMsgQID;
Message msg;

int main(int argc, char* argv[]) {

	allocateSharedMemory();
	allocateMessageQueues();
	
	shared = shmemPtr();
	
	/*parent msg q begin */
	pMsgQID = parentMsgQptr();
	cMsgQID = childMsgQptr();	

	printf("user: parentMsgQID: %d\n", pMsgQID);
	
	msgrcv(pMsgQID, &msg, sizeof(Message), 1, 0);
	printf("from oss: %s\n", msg.mtext);
	fflush(stdout);
	/*parent msg q end */

	printf("childMsgQID: %d\n", cMsgQID);	

	//child msg q send	
	strcpy(msg.mtext, "hello from user.c");
	
	if(msgsnd(cMsgQID, &msg, sizeof(Message), IPC_NOWAIT) == -1) {
		perror("msgsnd failed");
	}

	printf("\nuser.c: %d\n", shared->simTime.sec);

	return EXIT_SUCCESS;


}

