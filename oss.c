#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <limits.h>
#include <wait.h>
#include <sys/types.h>

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
void signalHandler(int);

void timeToSchedule(SimulatedClock*);
void incrementSimClock(SimulatedClock*, int);
void timeToCreateProcess(SimulatedClock*, int, int);

int findEmptyPCB();
int findIndex(int);

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

	signal(SIGINT, signalHandler);

	allocation();
	printf("Message Queues and Shared Memory are set\n");
	
	init();
	printf("Process Control Block variables have been initialized\n");
	
	printf("Simulated process scheduling has begun\n");
	scheduler();
	printf("Simulated process scheduling has ended\n");

	//sleep(2);

	fclose(fp);
	releaseSharedMemory();
	deleteMessageQueues();
	printf("Message Queues and Shared Memory have been released and deleted\n");

        return EXIT_SUCCESS;

}

void scheduler() {

	int activeProcesses = 0;
	int status;
	pid_t pid;	
	
	shared->simTime.sec = 0;
	shared->simTime.ns = 0;

	SimulatedClock totalCPU = {0,0};				//total time spent on CPU
	SimulatedClock totalSystem = {0,0};				//total time in the system
	SimulatedClock idleTime = {0,0};				//total time spent idle
	SimulatedClock totalBlocked = {0,0};				//time spent being blocked

	while(1) {
	
		if(locPID > 4) {
			break;
		}
	
		int procIncSecs = (rand() % (maxTimeBetweenNewProcsSecs + 1));
		int procIncNS = (rand() % (maxTimeBetweenNewProcsNS +1));
		timeToCreateProcess(&(shared->simTime), procIncNS, procIncSecs);				//time to create a process
		printf("procIncSEC: %d\tprocIncNANO: %d\n", procIncSecs, procIncNS);
		
		timeToSchedule(&(shared->simTime));								//time required to put in ready queue
		printf("simClock SEC: %d\nsimClock NANO: %d\n", shared->simTime.sec, shared->simTime.ns);
	
		int position = findEmptyPCB();
	
		shared->table[position].localPID = locPID;
		
		//timeToSchedule(&(shared->simTime));
		fprintf(fp, "OSS: GENERATING PROCESS [LOCAL PID: %d] PUT IN READY QUEUE [TIME ENTERED READY QUEUE: [%d:%d]]\n", shared->table[position].localPID,
			shared->simTime.sec, shared->simTime.ns);

		if(position != -1) {
			spawnUser(position);
			activeProcesses++;
		}
		
		enqueue(readyQ, locPID);
		
	
		//sleep(1);
		int q = 0;
		while(q < 10000000) q++;

		msgrcv(cMsgQID, &msg, sizeof(Message), shared->table[position].userPID, 0);
	
		if(strcmp(msg.mtext, "COMPLETE") == 0) {
	
			dequeue(readyQ);
			//activeProcesses--;

			printf("%s\n", msg.mtext);
			
			msgrcv(cMsgQID, &msg, sizeof(Message), shared->table[position].userPID, 0);
			printf("used all: %s\n", msg.mtext);
			
			int temp = atoi(msg.mtext);
			int adj = (int) ((double) 10000000 * ((double) temp / (double) 100));
			
			incrementSimClock(&(shared->simTime), adj);
			
			shared->table[position].leaveTime.sec = shared->simTime.sec;
			shared->table[position].leaveTime.sec = shared->simTime.ns;
	
		
			fprintf(fp, "OSS: [LOCAL PID: %d] [PID: %d] USED ENTIRE TIMESLICE [10 ms]\n\t[TIME FINISHED: [%d:%d]] REMOVED FROM READY QUEUE\n",
				shared->table[position].localPID, shared->table[position].userPID, shared->simTime.sec, shared->simTime.ns);
				
			//shared->table[position].localPID = -1;
				
		}
		else if(strcmp(msg.mtext, "BLOCKED") == 0) {
		
			dequeue(readyQ);
			enqueue(blockedQ, locPID);
			//activeProcesses--;
			printf("%s\n", msg.mtext);
		
			msgrcv(cMsgQID, &msg, sizeof(Message), shared->table[position].userPID, 0);
			printf("%s\n", msg.mtext);
			
			int temp = atoi(msg.mtext);
			int adj = (int) ((double) 10000000 * ((double) temp / (double) 100));
			
			incrementSimClock(&(shared->simTime), adj);
		
			fprintf(fp, "OSS: [LOCAL PID: %d] [PID: %d] PROCESS BLOCKED [(ADDVAR)% USED]\n \t [TIME ENTERED BLOCKED QUEUE: [%d:%d]]\n \t [UNBLOCK AT TIME: (ADDVAR:ADDVAR)]\n",
				shared->table[position].localPID, shared->table[position].userPID, shared->simTime.sec, shared->simTime.ns);		
		}
		else if(strcmp(msg.mtext, "TERMINATE") == 0) {	
			dequeue(readyQ);
			//activeProcesses--;
			printf("%s\n", msg.mtext);
		
			msgrcv(cMsgQID, &msg, sizeof(Message), shared->table[position].userPID, 0);
			printf("percentage: %s\n", msg.mtext);
			
			int temp = atoi(msg.mtext);
			int adj =  (int) ((double) 10000000 * ((double) temp / (double) 100));
			
			printf("adj: %d\n", adj);
			
			incrementSimClock(&(shared->simTime), adj);
			
			shared->table[position].leaveTime.sec = shared->simTime.sec;
			shared->table[position].leaveTime.sec = shared->simTime.ns;
			
			//printf("TERMINATE: simClock SEC: %d\tsimClock NANO: %d\n", shared->simTime.sec, shared->simTime.ns);
		
			fprintf(fp, "OSS: [LOCAL PID: %d] [PID: %d] TERMINATED STATE\n\t[USED %d% OF TIMESLICE] [TIME FINISHED: [%d:%d]] REMOVED FROM READY QUEUE\n", 
				shared->table[position].localPID, shared->table[position].userPID, temp, shared->simTime.sec, shared->simTime.ns);
				
			//shared->table[position].localPID = -1;
		}
		
		if(isEmpty(blockedQ) == 0) {
			incrementSimClock(&(shared->simTime), 5000000);
			
			int j;
			for(j = 0; j < sizeQ(blockedQ); j++) {
				int blockedLocalPID = dequeue(blockedQ);
				int PIDinPCB = findIndex(blockedLocalPID);
				if(msgrcv(cMsgQID, &msg, sizeof(Message), shared->table[PIDinPCB].userPID, IPC_NOWAIT) > -1) {
					if(strcmp(msg.mtext, "UNBLOCKED") == 0) {
						printf("\n\n\n\n%s\n\n\n\n\n", msg.mtext);
						enqueue(readyQ, shared->table[PIDinPCB].userPID);
					}
				}
				else {
					enqueue(blockedQ, PIDinPCB);
				}
			}
		}
		
		if(isEmpty(readyQ) == 0) {
		
			int resumeLocalPID = dequeue(readyQ);
			int rPIDinPCB = findIndex(resumeLocalPID);
			
		
		}
		
		pid = waitpid(-1, &status, WNOHANG);
		//source stack overflow: https://stackoverflow.com/questions/28840215/when-and-why-should-you-use-wnohang-with-waitpid
		if(pid > 0) {
			if(WIFEXITED(status)) {
				if(WEXITSTATUS(status) == 0 || WEXITSTATUS(status) == 2) {
					activeProcesses--;
					if(position != -1) {
						shared->table[position].localPID = -1;
					}
				}
			}
		}
/*		
		if(locPID > 10){
			break;
		}
*/		
		if(totalProcess < 0) {
			break;
		}
		locPID++;
		totalProcess--;
		
	} // main loop
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

void timeToCreateProcess(SimulatedClock* procSched, int nanoSec, int segundos) {
	procSched->sec += segundos;
	
	int nSec = procSched->ns + nanoSec;
	while(nSec >= 1000000000) {
		nSec -= 1000000000;
		(procSched->sec)++;
	}
	procSched->ns = nSec;

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

int findIndex(int index) {
	int i = 0;
	for(i = 0; i < MAX_USER_PROCESS; i++) {
		if(shared->table[i].localPID == index) {
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

void signalHandler(int signal) {

	printf("oss.c: terminating: signalHandler\n");
	
	int i;
	for(i = 0; i < MAX_USER_PROCESS; i++) {
		if(shared->table[i].userPID != -1) {
			kill(shared->table[i].userPID, SIGTERM);
		}
	}
	
	fclose(fp);
	
	releaseSharedMemory();
	deleteMessageQueues();

	kill(getpid(), SIGTERM);	

}
