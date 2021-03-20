//source: https://www.geeksforgeeks.org/queue-set-1introduction-and-array-implementation/

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <sys/shm.h>

#include "shared.h"

//note to self: static variables stay in memory while program is running

//Keys for message queues and shared memory
static key_t shmKey;
static key_t parentMsgKey;
static key_t childMsgKey;

//IDs for message queues and shared memory
static int shmID;
static int parentMsgQID;
static int childMsgQID;

SharedMemory * shmem = NULL;

void allocateSharedMemory() {
	if((shmKey = ftok("./makefile", 'p')) == -1) {
		perror("shared.c: error: shmKey ftok failed");
		exit(EXIT_FAILURE);
	}

	if((shmID = shmget(shmKey, sizeof(SharedMemory), IPC_CREAT | S_IRUSR | S_IWUSR)) < 0) {
		perror("shared.c: error: failed to allocate shared memory");
		exit(EXIT_FAILURE);
	}
	else {
		shmem = (SharedMemory*)shmat(shmID, NULL, 0);
	}
}

void releaseSharedMemory() {
	if(shmem != NULL) {
		if(shmdt(shmem) == -1) {
			perror("shared.c: error: failed to release shared memory");
			exit(EXIT_FAILURE);
		}
	}
	deleteSharedMemory();
}

void deleteSharedMemory() {
	if(shmID > 0) {
		if(shmctl(shmID, IPC_RMID, NULL) < 0) {
			perror("shared.c: error: failed to delete shared memory");
			exit(EXIT_FAILURE);
		}
	}
}

void allocateMessageQueues() {

	if((childMsgKey = ftok("./makefile", 's')) == -1) {
		perror("shared.c: error: childMsgKey");
		exit(EXIT_FAILURE);
	}

	if((parentMsgKey = ftok("./makefile", 't')) == -1) {
		perror("shared.c: error: parentMsgKey");
		exit(EXIT_FAILURE);
	}

	if((childMsgQID = msgget(childMsgKey, 0666 | IPC_CREAT)) == -1) {
		perror("shared.c: error: child message queue allocation failed");
		exit(EXIT_FAILURE);
	}

	if((parentMsgQID = msgget(parentMsgKey, 0666 | IPC_CREAT)) == -1) {
		perror("shared.c: error: parent message queue allocation failed");
		exit(EXIT_FAILURE);
	}
}

void deleteMessageQueues() {

	if(childMsgQID > 0) {
		if(msgctl(childMsgQID, IPC_RMID, NULL) == -1) {
			perror("shared.c: error: failed to delete child message queue");
			exit(EXIT_FAILURE);
		}
	}

	if(parentMsgQID > 0) {
		if(msgctl(parentMsgQID, IPC_RMID, NULL) == -1) {
			perror("shared.c: error: failed to delete parent message queue");
			exit(EXIT_FAILURE);
		}
	}
}

SharedMemory* shmemPtr() {
	return shmem;
}

int childMsgQptr() {
	return childMsgQID;
}

int parentMsgQptr() {
	return parentMsgQID;
}
