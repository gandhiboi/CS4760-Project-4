//source: https://www.geeksforgeeks.org/queue-set-1introduction-and-array-implementation/

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "queue.h"

Queue* createQueue(unsigned int capacity) {

	Queue * queue = (Queue*)malloc(sizeof(Queue));
	queue->capacity = capacity;
	queue->front = queue->size = 0;
	
	queue->rear = capacity - 1;
	queue->array = (unsigned int*)malloc(queue->capacity * sizeof(unsigned int));

	return queue;
	
}

unsigned int isFull(Queue* queue) {

	return (queue->size == queue->capacity);

}

unsigned int isEmpty(Queue* queue) {

	return (queue->size == 0);

}

void enqueue(Queue* queue, unsigned int item) {

	if(isFull(queue)) {
		return;
	}
	
	queue->rear = (queue->rear + 1) % queue->capacity;
	queue->array[queue->rear] = item;
	queue->size = queue->size + 1;
	
}

unsigned int dequeue(Queue* queue) {

	if(isEmpty(queue)) {
		return INT_MIN;
	}
	
	int item = queue->array[queue->front];
	queue->front = (queue->front + 1) % queue->capacity;
	queue->size = queue->size - 1;
	
	return item;
	
}

unsigned int front(Queue* queue) {

	if(isEmpty(queue)) {
		return INT_MIN;
	}
	
	return queue->array[queue->front];

}
unsigned int rear(Queue* queue) {

	if(isEmpty(queue)) {
		return INT_MIN;
	}

	return queue->array[queue->rear];

}
