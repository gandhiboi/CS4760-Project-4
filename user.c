#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/types.h>

#include "shared.h"

//macros for setting up processes
#define PERCENT_BLOCKED 20
#define PERCENT_USE_ALL 90
#define BUFFER_SIZE 10
#define CHANCE_CPU 70

//used for attaching to shared memory
SharedMemory * shared = NULL;
static int pMsgQID;
static int cMsgQID;
Message msg;

//object used for unblocking processes
SimulatedClock unblocked;

int pid;

static int help;

void allocation();
void simProcessComplete();
void simProcessBlock();
void simProcessTerminate();

void timeUntilUnblocked(SimulatedClock*, int, int);
SimulatedClock findBlockedTime(SimulatedClock*);
void incrementSimClock(SimulatedClock*, int);

int main(int argc, char* argv[]) {

	allocation();				//allocates/attaches message qs and shared memory
	srand(getpid());			//seeds based of pid
	
	pid = getpid();
	
	int indexer = atoi(argv[0]);		//accepts position in PCB from oss
	help = indexer;
	shared->table[indexer].arrivalTime.sec = shared->simTime.sec;
	shared->table[indexer].arrivalTime.sec = shared->simTime.ns;
	
	//printf("user: index: %d\n", indexer);			//debugging purposes	
	
	//determines if process is IO bound or CPU bound and sets flag in PCB
	if(rand() % 100 <= CHANCE_CPU) {
		shared->table[indexer].processTypeFlag = "CPU";			//sets flag for process to be CPU bound
		printf("PID: %d\nProcess Type Flag: %s\n", pid, shared->table[indexer].processTypeFlag);
		shared->cpuCount += 1;
	}
	else {
		shared->table[indexer].processTypeFlag = "IO";			//sets flag for process to be IO bound
		printf("Process Type Flag: %s\n", shared->table[indexer].processTypeFlag);
		shared->ioCount += 1;
	}
	
	//sets pid in PCB
	shared->table[indexer].userPID = getpid();
	
	//var used for determining whether process will terminate, block, or use all
	int rp = (rand() % 100);
	
	while(1) {
	
		//used to wake processes
		msgrcv(pMsgQID, &msg, sizeof(Message), pid, 0);
	
		//a wonky set up to do it but it works, but uses standard values for determining the state of process
		if(strcmp(shared->table[indexer].processTypeFlag, "CPU") == 0) {
			if(rp <= PERCENT_BLOCKED) {
				simProcessBlock();
			}
			else if(rp <= PERCENT_USE_ALL) {
				simProcessComplete();
			}
			else {
				simProcessTerminate();
			}
		}
		//increased chance of getting blocked for IO processes
		else if(strcmp(shared->table[indexer].processTypeFlag, "IO") == 0) {
			if(rp <= PERCENT_BLOCKED + 5) {
				simProcessBlock();
			}
			else if(rp <= PERCENT_USE_ALL) {
				simProcessComplete();
			}
			else {
				simProcessTerminate();
			}
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

//sends message to oss if the process is determined to use all and increments clock for totalCPU time
void simProcessComplete() {
	//sends message to oss that process is complete
	msg.mtype = pid;
	strcpy(msg.mtext, "COMPLETE");
	if(msgsnd(cMsgQID, &msg, sizeof(Message), 0)) {
		perror("user.c: error: simProcessComplete msgsnd failed");
		exit(EXIT_FAILURE);
	}
	
	int usedAll = 100;
	char temp[BUFFER_SIZE];
	snprintf(temp, BUFFER_SIZE, "%d", usedAll);
	
	//kind of unnecessary but used it for making it easier to understand for myself; sends message that used 100%
	msg.mtype = pid;	
	strcpy(msg.mtext, temp);
	if(msgsnd(cMsgQID, &msg, sizeof(Message), 0)) {
		perror("user.c: error: simProcessComplete msgsnd failed");
		exit(EXIT_FAILURE);
	}
	
	int adj =  (int) ((double) 10000000 * ((double) usedAll / (double) 100));
	
	if(strcmp(shared->table[help].processTypeFlag, "CPU") == 0) {
		incrementSimClock(&(shared->totalCPU), adj);
	}
	else {
		incrementSimClock(&(shared->totalIO), adj);
	}
	
	exit(EXIT_SUCCESS);
}

//func for blocked processes
void simProcessBlock() {
	srand(getpid());
	
	//sets unblocked object to current time so it can be added to later
	unblocked.sec = shared->simTime.sec;
	unblocked.ns = shared->simTime.ns;
	
	//printf("user before adding time: unblocked.sec: %d\tunblocked.ns: %d\n", unblocked.sec, unblocked.ns);
	
	//sends message to oss that it will be blocked
	msg.mtype = pid;
	strcpy(msg.mtext, "BLOCKED");
	if(msgsnd(cMsgQID, &msg, sizeof(Message), IPC_NOWAIT)) {
		perror("user.c: error: 1simProcessBlock msgsnd failed");
		exit(EXIT_FAILURE);
	}

	//generates random percentage of how much process completed before being blocked
	int rng = (rand() % 99) + 1;
	char temp[BUFFER_SIZE];
	snprintf(temp, BUFFER_SIZE, "%d", rng);
	
	msg.mtype = pid;	
	strcpy(msg.mtext, temp);
	if(msgsnd(cMsgQID, &msg, sizeof(Message), 0)) {
		perror("user.c: error: 2simProcessBlock msgsnd failed");
		exit(EXIT_FAILURE);
	}

	//creates and sets random time that the process will be unblocked in the future
	int rSec = (rand() % 5) + 1;
	int sNs = (rand() % 1000) + 1;
	timeUntilUnblocked(&unblocked, sNs, rSec);
	
	int adj =  (int) ((double) 10000000 * ((double) rng / (double) 100));
	
	if(strcmp(shared->table[help].processTypeFlag, "CPU") == 0) {
		incrementSimClock(&(shared->totalCPU), adj);
		shared->totalBlockedCPU.sec += rSec;
		incrementSimClock(&(shared->totalBlockedCPU), sNs);
	}
	else {
		incrementSimClock(&(shared->totalIO), adj);
		shared->totalBlockedIO.sec += rSec;
		incrementSimClock(&(shared->totalBlockedIO), sNs);
	}
	
	//printf("user: unblocked.sec: %d\tunblocked.ns: %d\n", unblocked.sec, unblocked.ns);
	
	//source: git/jaredible; used to keep process going and until it's ready to be unblocked
	while(1) {
		if(shared->simTime.sec >= unblocked.sec && shared->simTime.ns >= unblocked.ns) {
			break;
		}
	}
	
	//sends message to oss that process is unblocked
	msg.mtype = pid;
	strcpy(msg.mtext, "UNBLOCKED");
	if(msgsnd(cMsgQID, &msg, sizeof(Message), 0)) {
		perror("user.c: error: 3simProcessBlock msgsnd failed");
		exit(EXIT_FAILURE);
	}

}

//func for terminating a process instantly after computing random percentage of wrok done
void simProcessTerminate() {
	srand(getpid());
	
	//sends message to oss that process will terminate
	msg.mtype = pid;
	strcpy(msg.mtext, "TERMINATE");
	if(msgsnd(cMsgQID, &msg, sizeof(Message), 0)) {
		perror("user.c: error: simProcessTerminate msgsnd failed");
		exit(EXIT_FAILURE);
	}

	//computes random percentage of work process has completed before terminating
	int rng = (rand() % 99) + 1;
	char temp[BUFFER_SIZE];
	snprintf(temp, BUFFER_SIZE, "%d", rng);
	
	//sends that percentage to oss
	msg.mtype = pid;	
	strcpy(msg.mtext, temp);
	if(msgsnd(cMsgQID, &msg, sizeof(Message), 0)) {
		perror("user.c: error: simProcessTerminate msgsnd failed");
		exit(EXIT_FAILURE);
	}
	
	//increments clock for totalcpu time or io time
	int adj =  (int) ((double) 10000000 * ((double) rng / (double) 100));
	
	if(strcmp(shared->table[help].processTypeFlag, "CPU") == 0) {
		incrementSimClock(&(shared->totalCPU), adj);
	}
	else {
		incrementSimClock(&(shared->totalIO), adj);
	}
	
	exit(EXIT_SUCCESS);
}

//used to determine the time that the process will be unblocked
void timeUntilUnblocked(SimulatedClock* unblockMe, int nanoSec, int segundos) {
	unblockMe->sec += segundos;
	
	int nSec = unblockMe->ns + nanoSec;
	while(nSec >= 1000000000) {
		nSec -= 1000000000;
		(unblockMe->sec)++;
	}
	unblockMe->ns = nSec;

}

//same as oss; used to increment local simTime structs and shared memory structs
void incrementSimClock(SimulatedClock* timeStruct, int increment) {
	int nanoSec = timeStruct->ns + increment;
	
	while(nanoSec >= 1000000000) {
		nanoSec -= 1000000000;
		(timeStruct->sec)++;
	}
	timeStruct->ns = nanoSec;
}

