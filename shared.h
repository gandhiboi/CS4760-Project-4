/*
 * Header file for shared memory
 * and message queues
 *
*/

#ifndef SHARED_H
#define SHARED_H

#define MSG_QUEUE_BUFF 100
#define MAX_USER_PROCESS 18

//Structure for Message Queue
typedef struct {
	long mtype;
	char mtext[100];
} Message;

//Structure for Simulated Clock
typedef struct {
	unsigned int sec;
	unsigned int ns;
} SimulatedClock;


//check to see if i actually need all these for the log file
//Structure for Process Control Block
typedef struct {
	pid_t userPID;
	int localPID;
	int state;
	int blockedLocalPID;
	char* processTypeFlag;				//cpu or io bound
	//SimulatedClock cpuTime;			//time spent on cpu
	//SimulatedClock waitTime;			//time spent waiting for something to happen
	//SimulatedClock blockTime;			//time it spent in blocked queue
	//SimulatedClock totalTime;			//total time spent in system
	SimulatedClock leaveTime;			//time process left the system
	SimulatedClock arrivalTime;			//initial arrival time
} PCB;

//Structure for shared memory for PCB and simulated clock
typedef struct {
	PCB table[MAX_USER_PROCESS];
	int cpuCount;
	int ioCount;
	SimulatedClock simTime;				//running simulated clock
	SimulatedClock totalCPU;				//total time spent on CPU
	SimulatedClock totalIO;
	//SimulatedClock totalSystem;				//total time in the system
	SimulatedClock totalBlockedCPU;			//time spent being blocked
	SimulatedClock totalBlockedIO;
} SharedMemory;

void allocateSharedMemory();
void releaseSharedMemory();
void deleteSharedMemory();

void allocateMessageQueues();
void deleteMessageQueues();

SharedMemory* shmemPtr();
int childMsgQptr();
int parentMsgQptr();

#endif
