#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <limits.h>

#include "shared.h"
#include "queue.h"

#define MAX_USER_PROCESS 18
#define TIME_QUANTUM 10

SharedMemory *shared = NULL;
Message msg;
FILE * fp;

Queue * blockedQ;
Queue * readyQ;

int totalProcess = 100;
const int maxTimeBetweenNewProcsSecs = 1;
const int maxTimeBetweenNewProcsNS = 100000;

static int pMsgQID;
static int cMsgQID;
int locPID = 0;

void allocation();
void spawnUser(int);
void init();
void usage();
void scheduler();

void timeToSchedule(SimulatedClock*);
void incrementSimClock(SimulatedClock*, int);

int findEmptyPCB();

int main(int argc, char* argv[]) {

	int opt;
	int s = 3;
	char * lVar = "output.log";
	
	while((opt = getopt(argc, argv, "hs:l:")) != -1) {
		switch(opt) {
			case 'h':
				usage();
				exit(EXIT_SUCCESS);
			case 's':
				if(!isdigit(*optarg) || (s = atoi(optarg)) < 0) {
					perror("oss.c: error: invalid termination time");
					usage();
					exit(EXIT_FAILURE);
				}
				break;
			case 'l':
				lVar = optarg;
				if(isdigit(*optarg)) {
					perror("oss.c: error: [-l f] must be a string");
					usage();
					exit(EXIT_FAILURE);
				}
				break;
			default:
				perror("oss.c: error: invalid argument");
				usage();
				exit(EXIT_FAILURE);
		}
	}
	
	fp = fopen(lVar, "w");
	
	if(fp == NULL) {
		perror("oss.c: error: failed to open log file");
	}

	srand(time(NULL));

	allocation();
	printf("Message Queues and Shared Memory are set\n");
	
	init();
	printf("Process Control Block variables have been initialized\n");
	
	printf("Simulated process scheduling has begun\n");
	scheduler();
	printf("Simulated process scheduling has ended\n");

	sleep(2);

	releaseSharedMemory();
	deleteMessageQueues();
	printf("Message Queues and Shared Memory have been released and deleted\n");

        return EXIT_SUCCESS;

}

void scheduler() {

	shared->simTime.sec = 0;
	shared->simTime.ns = 0;

	SimulatedClock totalCPU = {0,0};
	SimulatedClock totalSystem = {0,0};
	SimulatedClock schedulingTime = {0,0};
	SimulatedClock totalBlocked = {0,0};
	
	//incrementSimClock(&(shared->simTime));

	//while(1) {
	
		timeToSchedule(&(shared->simTime));
		printf("simClock SEC: %d\nsimClock NANO: %d\n", shared->simTime.sec, shared->simTime.ns);
	
		int position = findEmptyPCB();
	
		shared->table[position].localPID = locPID;

		spawnUser(position);
	
		sleep(1);

		msgrcv(cMsgQID, &msg, sizeof(Message), shared->table[position].userPID, 0);
	
		if(strcmp(msg.mtext, "COMPLETE") == 0) {

			printf("%s\n", msg.mtext);
		
			fprintf(fp, "OSS: [LOCAL PID: %d] [PID: %d] USED ALL TIME QUANTUM\n", shared->table[position].localPID, shared->table[position].userPID);
				
		}
		/*
		else if(strcmp(msg.mtext, "BLOCKED") == 0) {
			printf("%s\n", msg.mtext);
		
			msgrcv(cMsgQID, &msg, sizeof(Message), 1, 0);
			printf("%s\n", msg.mtext);
		
			sleep(2);
		
			releaseSharedMemory();
			deleteMessageQueues();
		
			return EXIT_SUCCESS;	
		}
		*/
		else if(strcmp(msg.mtext, "TERMINATE") == 0) {
			printf("%s\n", msg.mtext);
		
			msgrcv(cMsgQID, &msg, sizeof(Message), shared->table[position].userPID, 0);
			printf("percentage: %s\n", msg.mtext);
			
			int temp = atoi(msg.mtext);
			
			int adj =  (int) ((double) 10000000 * ((double) temp / (double) 100));
			
			printf("adj: %d\n", adj);
			
			incrementSimClock(&(shared->simTime), adj);
			
			//printf("TERMINATE: simClock SEC: %d\tsimClock NANO: %d\n", shared->simTime.sec, shared->simTime.ns);
		
			fprintf(fp, "OSS: [LOCAL PID: %d] [PID: %d] TERMINATED STATE\n \t [TIME FINISHED: [%d:%d]]\n", shared->table[position].localPID, shared->table[position].userPID, shared->simTime.sec, shared->simTime.ns);
		}
	//}
}

void timeToSchedule(SimulatedClock* schedInc) {
	int lower = 100;
	int upper = 10000;

	int rng = (rand() % (upper - lower + 1)) + lower;	
	//printf("timeToSchedule RNG: %d\n", rng);
	
	incrementSimClock(schedInc, rng);	
}

void incrementSimClock(SimulatedClock* timeStruct, int increment) {
	int nanoSec = timeStruct->ns + increment;
	
	while(nanoSec >= 1000000000) {
		nanoSec -= 1000000000;
		(timeStruct->sec)++;
	}
	timeStruct->ns = nanoSec;
}

void allocation() {
	allocateSharedMemory();
	allocateMessageQueues();

	shared = shmemPtr();
	
	pMsgQID = parentMsgQptr();
	cMsgQID = childMsgQptr();
	
	blockedQ = createQueue(MAX_USER_PROCESS);
	readyQ = createQueue(MAX_USER_PROCESS);
	
}

void init() {
	int i;
	for(i = 0; i < MAX_USER_PROCESS; i++) {
		shared->table[i].userPID = -1;
		shared->table[i].localPID = -1;
		//printf("oss: %d: %d\n", i, shared->table[i].userPID);
	}
}

void spawnUser(int index) {	
	pid_t pid = fork();
	
	if(pid == -1) {
		perror("oss.c: error: failed to fork");\
		exit(EXIT_FAILURE);
	}
	
	if(pid == 0) {
		int length = snprintf(NULL, 0, "%d", index);
		char* xx = (char*)malloc(length + 1);
		snprintf(xx, length + 1, "%d", index);
	
		execl("./user", xx, (char*)NULL);
		
		free(xx);
		xx = NULL;
	}
}

int findEmptyPCB() {
	int i;
	for(i = 0; i < MAX_USER_PROCESS; i++) {
		if(shared->table[i].localPID == -1) {
			return i;
		}
	}
	
	return -1;
}

void usage() {
	printf("======================================================================\n");
        printf("\t\t\t\tUSAGE\n");
        printf("======================================================================\n");
        printf("./oss -h [-s t] [-l f]\n");
        printf("run as: ./oss [options]\n");
        printf("if [-s i] is not defined it is set as default 3\n");
        printf("if [-l f] is not defined it is set as default output.log\n");
        printf("======================================================================\n");
        printf("-h              :       Describe how project should be run and then, terminate\n");
        printf("-s t            :       Indicate how many maximum seconds before the system terminates\n");
        printf("-l f            :       Specify a particular name for the log file\n");
        printf("======================================================================\n");
}
