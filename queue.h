#ifndef QUEUE_H
#define QUEUE_H

#include <stdio.h>
#include <stdlib.h>

typedef struct{
	unsigned int front, rear, size;
	unsigned int capacity;
	unsigned int* array;
} Queue;

Queue* createQueue(unsigned int);
unsigned int isFull(Queue*);
unsigned int isEmpty(Queue*);
void enqueue(Queue*, unsigned int);
unsigned int dequeue(Queue*);
unsigned int front(Queue*);
unsigned int rear(Queue*);
unsigned int sizeQ(Queue*);

#endif
