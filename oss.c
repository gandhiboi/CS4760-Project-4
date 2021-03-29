/*

	Kenan Krijestorac
	Project 4 - Process Scheduling
	Due: 4 April 2021

*/

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
#include <signal.h>
#include <sys/time.h>

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

SimulatedClock idle = {0,0};

static int pMsgQID;
static int cMsgQID;
int locPID = 0;

void allocation();
void spawnUser(int, int);
void init();
void usage();
void scheduler();
void signalHandler(int);
void setTimer(int);

void timeToSchedule(SimulatedClock*);
int timeToMoveQueues(SimulatedClock*);
void incrementSimClock(SimulatedClock*, int);
void timeToCreateProcess(SimulatedClock*, int, int);

void averageTime(SimulatedClock*);
void biggerAverageTime(SimulatedClock*, long);

void cleanup();

SimulatedClock * localSimClock(SimulatedClock*, int);

int findEmptyPCB();
int findIndex(int);

int main(int argc, char* argv[]) {

	signal(SIGINT, signalHandler);
	signal(SIGALRM, signalHandler);

	int opt;
	int s = 5;
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
		exit(EXIT_FAILURE);
	}

	srand(time(NULL));

	setTimer(s);

	allocation();
	printf("Message Queues and Shared Memory are set\n");
	
	init();
	printf("Process Control Block variables have been initialized\n");
	
	printf("Simulated process scheduling has begun\n");

	scheduler();
	printf("Simulated process scheduling has ended\n");
	
	printf("~~~~~~~~~~~~~~~~~~~~~SUMMARY~~~~~~~~~~~~~~~~~~~~~\n");
	printf("System Time: %d:%d\n", shared->simTime.sec, shared->simTime.ns);
	printf("CPU Bound Processes: %d\n", shared->cpuCount);
	printf("IO Bound Processes: %d\n", shared->ioCount);
	printf("~~~~~~~~~~~~~~~~~~~~~TOTALS~~~~~~~~~~~~~~~~~~~~~\n");
	printf("Total CPU Time: %d:%d\n", shared->totalCPU.sec, shared->totalCPU.ns);
	printf("Total IO Time: %d:%d\n", shared->totalIO.sec, shared->totalIO.ns);
	printf("Total Blocked Time: CPU: %d:%d\n", shared->totalBlockedCPU.sec, shared->totalBlockedCPU.ns);
	printf("Total Blocked Time: IO: %d:%d\n", shared->totalBlockedIO.sec, shared->totalBlockedIO.ns);
	printf("Total Idle Time: %d:%d\n", idle.sec, idle.ns);

	printf("~~~~~~~~~~~~~~~~~~~~~AVERAGES~~~~~~~~~~~~~~~~~~~~~\n");
	averageTime(&(shared->totalCPU));
	printf("Average CPU Time: %d:%d\n", shared->totalCPU.sec, shared->totalCPU.ns);
	
	averageTime(&(shared->totalIO));
	printf("Average IO Time: %d:%d\n", shared->totalIO.sec, shared->totalIO.ns);
	
	averageTime(&(shared->totalBlockedCPU));
	printf("Average Blocked Time: CPU: %d:%d\n", shared->totalBlockedCPU.sec, shared->totalBlockedCPU.ns);
	
	averageTime(&(shared->totalBlockedIO));
	printf("Average Blocked Time: IO: %d:%d\n", shared->totalBlockedIO.sec, shared->totalBlockedIO.ns);
	
	averageTime(&idle);
	printf("Average Idle Time: %d:%d\n", idle.sec, idle.ns);
	
	cleanup();

	fclose(fp);
	releaseSharedMemory();
	deleteMessageQueues();
	printf("Message Queues and Shared Memory have been released and deleted\n");

        return EXIT_SUCCESS;

}

void scheduler() {

	int cap = 0;
	int status;	
	int left = 0;
	int processing = 0;
	
	shared->simTime.sec = 0;
	shared->simTime.ns = 0;

	while(1) {
	
		int procIncSecs = (rand() % (maxTimeBetweenNewProcsSecs + 1));
		int procIncNS = (rand() % (maxTimeBetweenNewProcsNS +1));
		timeToCreateProcess(&(shared->simTime), procIncNS, procIncSecs);				//time to create a process
		//timeToCreateProcess(&idle, procIncNS, procIncSecs);						//reused function to increment idle time
		
		int position = findEmptyPCB();
		shared->table[position].localPID = locPID;
		printf("Local PID: %d\n", locPID);
		
		fprintf(fp, "OSS: GENERATING PROCESS [LOCAL PID: %d] [TIME GENERATED: %d:%d]\n", shared->table[position].localPID, shared->simTime.sec, shared->simTime.ns);

		
		timeToSchedule(&(shared->simTime));								//time required to put in ready queue
		//printf("simClock SEC: %d\nsimClock NANO: %d\n", shared->simTime.sec, shared->simTime.ns);
	
		fprintf(fp, "OSS: SCHEDULING PROCESS [LOCAL PID: %d] PLACED IN READY QUEUE [TIME ENTERED READY QUEUE: %d:%d]\n", shared->table[position].localPID, shared->simTime.sec, shared->simTime.ns);
	
		pid_t pid = fork();
		
		if(pid == -1) {
			perror("oss.c: error: failed to fork");\
			exit(EXIT_FAILURE);
		}
		
		if(pid == 0) {
			spawnUser(position, pid);
			cap++;
			exit(EXIT_SUCCESS);
		}
		
		if(position == -1) {
			kill(pid, SIGTERM);
		}
		
		enqueue(readyQ, pid);
		
		fprintf(fp, "OSS: DISPATCHING [LOCAL PID: %d] [PID: %d] [TIME DISPATCHED: %d:%d]\n",locPID, pid, shared->simTime.sec, shared->simTime.ns);
		
		if(isEmpty(readyQ) == 0) {
			processing = 1;
		
			//int resumeLocalPID = front(readyQ);
			//int rPIDinPCB = findIndex(resumeLocalPID);
				
			//source stack overflow: https://stackoverflow.com/questions/28840215/when-and-why-should-you-use-wnohang-with-waitpid
			int waitPID = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED);
			if(waitPID > 0) {
				if(WIFEXITED(status)) {
					if(WEXITSTATUS(status) == 0) {
						cap--;
						if(position > -1) {
							left++;
							shared->table[position].state = -1;
						}
					}
				}
			}
		
			msg.mtype = pid;
			strcpy(msg.mtext, "DISPATCH");
			msgsnd(pMsgQID, &msg, sizeof(Message), 0);
	
			int q = 0;
			while(q < 19909990) q++;

			msgrcv(cMsgQID, &msg, sizeof(Message), shared->table[position].userPID, 0);
	
			if(strcmp(msg.mtext, "COMPLETE") == 0) {
				processing = 0;
				dequeue(readyQ);
				//printf("%s\n", msg.mtext);
		
				msgrcv(cMsgQID, &msg, sizeof(Message), shared->table[position].userPID, 0);
			
				int temp = atoi(msg.mtext);
				int adj = (int) ((double) 10000000 * ((double) temp / (double) 100));
			
				incrementSimClock(&(shared->simTime), adj);
		
				shared->table[position].leaveTime.sec = shared->simTime.sec;
				shared->table[position].leaveTime.ns = shared->simTime.ns;
		
				fprintf(fp, "OSS: [LOCAL PID: %d] [PID: %d] USED ENTIRE TIME QUANTUM [10 ms]\n\t[TIME FINISHED: [%d:%d]] REMOVED FROM READY QUEUE\n",
				shared->table[position].localPID, shared->table[position].userPID, shared->simTime.sec, shared->simTime.ns);
			
				shared->table[position].state = -1;	
				
			}
			else if(strcmp(msg.mtext, "BLOCKED") == 0) {
		
				processing = 0;
			
				shared->table[position].blockedLocalPID = locPID;
						
				enqueue(blockedQ, pid);		
				dequeue(readyQ);

				//printf("%s\n", msg.mtext);
		
				msgrcv(cMsgQID, &msg, sizeof(Message), shared->table[position].userPID, 0);
				//printf("%s\n", msg.mtext);
			
				int temp = atoi(msg.mtext);
				int adj = (int) ((double) 10000000 * ((double) temp / (double) 100));
				
				incrementSimClock(&(shared->simTime), adj);
		
				fprintf(fp, "OSS: [LOCAL PID: %d] [PID: %d] PROCESS BLOCKED [%d% USED]\n \t [TIME ENTERED BLOCKED QUEUE: [%d:%d]]\n \t [UNBLOCK AT TIME: (ADDVAR:ADDVAR)]\n",
				shared->table[position].localPID, shared->table[position].userPID, temp, shared->simTime.sec, shared->simTime.ns);

			}
			else if(strcmp(msg.mtext, "TERMINATE") == 0) {	
				processing = 0;
				dequeue(readyQ);
			
				//printf("%s\n", msg.mtext);
		
				msgrcv(cMsgQID, &msg, sizeof(Message), shared->table[position].userPID, 0);
			
				int temp = atoi(msg.mtext);
				int adj =  (int) ((double) 10000000 * ((double) temp / (double) 100));
			
				//printf("adj: %d\n", adj);
			
				incrementSimClock(&(shared->simTime), adj);
				
				shared->table[position].leaveTime.sec = shared->simTime.sec;
				shared->table[position].leaveTime.ns = shared->simTime.ns;
		
				fprintf(fp, "OSS: [LOCAL PID: %d] [PID: %d] TERMINATED STATE\n\t[USED %d% OF TIMESLICE] [TIME FINISHED: [%d:%d]] REMOVED FROM READY QUEUE\n", 
				shared->table[position].localPID, shared->table[position].userPID, temp, shared->simTime.sec, shared->simTime.ns);
				
				shared->table[position].state = -1;
			}
		
			if(isEmpty(blockedQ) == 0) {

				if(processing == 0) {
					incrementSimClock(&(shared->simTime), 500000);
					incrementSimClock(&idle, 500000);
				}
				
				int j;
				for(j = 0; j < sizeQ(blockedQ); j++) {
					int blockedLocalPID = dequeue(blockedQ);
					int PIDinPCB = findIndex(blockedLocalPID);
					if(msgrcv(cMsgQID, &msg, sizeof(Message), shared->table[PIDinPCB].userPID, IPC_NOWAIT) > -1) {
						if(strcmp(msg.mtext, "UNBLOCKED") == 0) {
							fprintf(fp, "OSS: [LOCAL PID: %d] [PID: %d] UNBLOCKED \n\t[TIME FINISHED: [%d:%d]] REMOVED FROM READY QUEUE\n", 
							shared->table[PIDinPCB].blockedLocalPID, shared->table[PIDinPCB].userPID, shared->simTime.sec, shared->simTime.ns);
							left++;
							int mvQ = timeToMoveQueues(&(shared->simTime));
							enqueue(readyQ, shared->table[PIDinPCB].userPID);
						
							strcpy(msg.mtext, "DISPATCH");
							fprintf(fp, "OSS: DISPATCHING UNBLOCKED PROCESS [LOCAL PID: %d] [PID: %d] [TIME TO SWITCH QUEUES: [%d NS]]\n",
								shared->table[PIDinPCB].localPID, front(readyQ), mvQ);
							
							msgsnd(pMsgQID, &msg, sizeof(Message), 0);
						}
					}
					else {
						enqueue(blockedQ, PIDinPCB);
					}
				}
			}
		
		}
		
		
		if(totalProcess < 0 || locPID == 100) {
			break;
		} 
		locPID++;
		totalProcess--;
		
	}
	
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

int timeToMoveQueues(SimulatedClock* blockToReady) {
	int lower = 500;
	int upper = 50000;
	
	int rng = (rand() % (upper - lower + 1)) + lower;
	
	incrementSimClock(blockToReady, rng);
	
	return rng;
}

void averageTime(SimulatedClock * timeStruct) {
	long avg = (((long)(timeStruct->sec) * (long)1000000000) + (long)(timeStruct->ns))/100;
	SimulatedClock convert = {0,0};
	biggerAverageTime(&convert, avg);
	timeStruct->sec = convert.sec;
	timeStruct->ns = convert.ns;
}

void biggerAverageTime(SimulatedClock * timeStruct, long increment) {
	long nanoSec = timeStruct->ns + increment;
	
	while(nanoSec >= 1000000000) {
		nanoSec -= 1000000000;
		(timeStruct->sec)++;
	}
	timeStruct->ns = (int)nanoSec;

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
		shared->table[i].state = -1;
	}
}

void spawnUser(int index, int PID) {	

		int length = snprintf(NULL, 0, "%d", index);
		char* xx = (char*)malloc(length + 1);
		snprintf(xx, length + 1, "%d", index);
	
		execl("./user", xx, (char*)NULL);
		
		free(xx);
		xx = NULL;
}

int findEmptyPCB() {
	int i;
	for(i = 0; i < MAX_USER_PROCESS; i++) {
		if(shared->table[i].state == -1) {
			return i;
		}
	}
	
	return -1;
}

int findIndex(int index) {
	int i = 0;
	for(i = 0; i < MAX_USER_PROCESS; i++) {
		if(index == shared->table[i].userPID) {
			return i;
		}
	}
	return -1;
}

void signalHandler(int signal) {

	if(signal == SIGINT) {
		printf("oss.c: terminating: ctrl + c signal handler\n");
		fflush(stdout);
	}
	else if(signal == SIGALRM) {
		printf("oss.c: terminating: timer signal handler\n");
		fflush(stdout);
	}
	
	int i;
	for(i = 0; i < MAX_USER_PROCESS; i++) {
		kill(shared->table[i].userPID, SIGINT);
	}
	
	fclose(fp);
	releaseSharedMemory();
	deleteMessageQueues();
	
	exit(EXIT_SUCCESS);
}

void setTimer(int seconds) {

	struct sigaction act;
	act.sa_handler = &signalHandler;
	act.sa_flags = SA_RESTART;
	
	if(sigaction(SIGALRM, &act, NULL) == -1) {
		perror("oss.c: error: failed to set up sigaction handler for setTimer()");
		exit(EXIT_FAILURE);
	}
	
	struct itimerval value;
	value.it_interval.tv_sec = 0;
	value.it_interval.tv_usec = 0;
	
	value.it_value.tv_sec = seconds;
	value.it_value.tv_usec = 0;
	
	if(setitimer(ITIMER_REAL, &value, NULL)) {
		perror("oss.c: error: failed to set the timer");
		exit(EXIT_FAILURE);
	}

}

void cleanup() {
	int i;
	for(i = 0; i < MAX_USER_PROCESS; i++) {
		if(shared->table[i].userPID != -1) {
			kill(shared->table[i].userPID, SIGINT);
		}
	}

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

