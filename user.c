#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#include "shared.h"

#define PERCENT_BLOCKED 20
#define PERCENT_USE_ALL 90
#define CHANCE_CPU 60
#define BUFFER_SIZE 10

SharedMemory * shared = NULL;
static int pMsgQID;
static int cMsgQID;
Message msg;

int pid;

void allocation();
void simProcessComplete();
void simProcessBlock();
void simProcessTerminate();

int main(int argc, char* argv[]) {

	allocation();
	
	srand(getpid());
	
	pid = getpid();
	
	int index = atoi(argv[0]);
	
	printf("user: index: %d\n", index);
	
	if(rand() % 100 <= CHANCE_CPU) {
		shared->table[index].processTypeFlag = 0;			//sets flag for process to be CPU bound
	}
	else {
		shared->table[index].processTypeFlag = 1;			//sets flag for process to be IO bound
	}
	
	printf("user: processTypeFlag: %d\n", shared->table[index].processTypeFlag);
	
	shared->table[index].userPID = getpid();
	
	int rp = (rand() % 100);
	
	if(shared->table[index].processTypeFlag == 0) {
		if(rp <= PERCENT_BLOCKED) {
			printf("user.c: %d\n", rp);
			simProcessBlock();
		}
		else if(rp <= PERCENT_USE_ALL) {
			printf("user.c: %d\n", rp);
			simProcessComplete();
		}
		else {
			printf("user.c: %d\n", rp);
			simProcessTerminate();
		}
	}
	else if(shared->table[index].processTypeFlag == 1) {
		if(rp <= PERCENT_BLOCKED + 5) {
			printf("increased chance to block\n");
			printf("user.c: %d\n", rp);
			simProcessBlock();
		}
		else if(rp <= PERCENT_USE_ALL) {
			printf("user.c: %d\n", rp);
			simProcessComplete();
		}
		else {
			printf("user.c: %d\n", rp);
			simProcessTerminate();
		}
	}
	

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

	msg.mtype = pid;
	strcpy(msg.mtext, "COMPLETE");
	if(msgsnd(cMsgQID, &msg, sizeof(Message), 0)) {
		perror("user.c: error: simProcessComplete msgsnd failed");
		exit(EXIT_FAILURE);
	}
	
}

void simProcessBlock() {

	srand(getpid());
	
	msg.mtype = pid;
	strcpy(msg.mtext, "BLOCKED");
	if(msgsnd(cMsgQID, &msg, sizeof(Message), 0)) {
		perror("user.c: error: simProcessBlock msgsnd failed");
		exit(EXIT_FAILURE);
	}

	int rng = (rand() % 99) + 1;
	char temp[BUFFER_SIZE];
	snprintf(temp, BUFFER_SIZE, "%d", rng);
	
	msg.mtype = pid;	
	strcpy(msg.mtext, temp);
	if(msgsnd(cMsgQID, &msg, sizeof(Message), 0)) {
		perror("user.c: error: simProcessBlock msgsnd failed");
		exit(EXIT_FAILURE);
	}	
	

}

void simProcessTerminate() {

	srand(getpid());
	
	msg.mtype = pid;
	strcpy(msg.mtext, "TERMINATE");
	if(msgsnd(cMsgQID, &msg, sizeof(Message), 0)) {
		perror("user.c: error: simProcessTerminate msgsnd failed");
		exit(EXIT_FAILURE);
	}

	int rng = (rand() % 99) + 1;
	char temp[BUFFER_SIZE];
	snprintf(temp, BUFFER_SIZE, "%d", rng);
	
	msg.mtype = pid;	
	strcpy(msg.mtext, temp);
	if(msgsnd(cMsgQID, &msg, sizeof(Message), 0)) {
		perror("user.c: error: simProcessTerminate msgsnd failed");
		exit(EXIT_FAILURE);
	}	

}


