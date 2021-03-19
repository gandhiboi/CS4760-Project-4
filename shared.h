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
	char mtext[MSG_QUEUE_BUFF];
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
	int indexedPID;
	SimulatedClock cpuTime;			//time spent on cpu
	SimulatedClock waitTime;			//time spent waiting for something to happen
	SimulatedClock queueTime;			//time spent in queue
	SimulatedClock blockTime;			//time it spent in blocked queue
	SimulatedClock enterTime;			//time process began calculations
	SimulatedClock leaveTime;			//time process left after completing calculations
	SimulatedClock arrivalTime;			//initial arrival time
	SimulatedClock completeTime;			//time process completed
} PCB;

//Structure for shared memory for PCB and simulated clock
typedef struct {
	PCB table[MAX_USER_PROCESS];
	SimulatedClock simTime;
} SharedMemory;

void allocateSharedMemory();
void releaseSharedMemory();
void deleteSharedMemory();

void allocateMessageQueues();
void deleteMessageQueues();






#endif
