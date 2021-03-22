#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#include "shared.h"

#define PERCENT_TERMINATE 10
#define PERCENT_INTERRUPT 20
#define PERCENT_USE_ALL 70

SharedMemory * shared = NULL;
static int pMsgQID;
static int cMsgQID;
Message msg;

void allocation();
void simProcessComplete();
void simProcessBlock();
void simProcessTerminate();

int main(int argc, char* argv[]) {

	allocation();
	
	//printf("user: parentMsgQID: %d\n", pMsgQID);
	
	msgrcv(pMsgQID, &msg, sizeof(Message), 1, 0);
	printf("from oss: %s\n", msg.mtext);
	fflush(stdout);
	/*parent msg q end */
	
	//printf("childMsgQID: %d\n", cMsgQID);
	
	printf("from oss 2: %s\n", msg.mtext);
	
	//child msg q send	
	strcpy(msg.mtext, "hello from user.c");
	
	if(msgsnd(cMsgQID, &msg, sizeof(Message), IPC_NOWAIT) == -1) {
		perror("msgsnd failed");
	}

	printf("\nuser.c: %d\n", shared->simTime.sec);

	simProcessComplete();

	return EXIT_SUCCESS;

}

//attaches to shared memory/message queues
void allocation() {
	allocateSharedMemory();
	allocateMessageQueues();
	
	shared = shmemPtr();
	
	pMsgQID = parentMsgQptr();
	cMsgQID = childMsgQptr();
}

void simProcessComplete() {

	srand(getpid());
	printf("%d\n", (rand() % 99 + 1));

}

void simProcessBlock() {

	srand(getpid());

}

void simProcessTerminate() {

	srand(getpid());

}
