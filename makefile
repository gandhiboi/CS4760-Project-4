#source stackoverflow; was confused how it would work
#with object files that not both executables use

CC = gcc
CFLAG = -g -Wall

OSS = oss.c
USER = user.c

OSS_EXEC = oss
USER_EXEC = user
EXECUTABLES = $(OSS_EXEC) $(USER_EXEC)

OSS_OBJ = $(OSS:.c=.o) $(SHARED_OBJ) $(QUEUE_OBJ)
USER_OBJ = $(USER:.c=.o) $(SHARED_OBJ)

SHARED_OBJ = shared.o
QUEUE_OBJ = queue.o 

all: $(EXECUTABLES)

$(OSS_EXEC): $(OSS_OBJ)
	$(CC) $(CFLAG) $(OSS_OBJ) -o $(OSS_EXEC)

$(USER_EXEC): $(USER_OBJ)
	$(CC) $(CFLAG) $(USER_OBJ) -o $(USER_EXEC)

%.o: %.c
	$(CC) $(CFLAG) -c $*.c -o $*.o

clean:
	rm -f $(OUTPUT) *.o *.log $(OSS_EXEC) $(USER_EXEC)
