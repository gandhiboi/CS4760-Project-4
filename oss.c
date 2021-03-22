#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <unistd.h>
#include <ctype.h>

#include "shared.h"
#include "queue.h"

#define MAX_USER_PROCESS 18

SharedMemory *shared = NULL;
Message msg;

Queue * blockedQ;
Queue * readyQ;

const int maxTimeBetweenNewProcsNS = 100000;
const int maxTimeBetweenNewProcsSecs = 1;

static int pMsgQID;
static int cMsgQID;

void allocation();
void usage();

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

	printf("s: %d\n", s);
	printf("lVar: %s\n", lVar);

	allocation();

	shared->simTime.sec = 4;

	msg.mtype = 1;
	
	//printf("parentMsgQID: %d\n", pMsgQID);
	
	strcpy(msg.mtext, "hello 1 from oss.c");

	if(msgsnd(pMsgQID, &msg, sizeof(Message), IPC_NOWAIT) == -1) {
		perror("msgsnd failed");
	}
		
	if(fork() == 0) {
		execl("./user", "user", (char*)NULL);
	}

	//printf("childMsgQID: %d\n", cMsgQID);
	fflush(stdout);	
	msgrcv(cMsgQID, &msg, sizeof(Message), 1, 0);
	printf("from user: %s\n", msg.mtext);

	sleep(2);

	releaseSharedMemory();
	deleteMessageQueues();

        return EXIT_SUCCESS;

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
