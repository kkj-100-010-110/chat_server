#ifndef _QUEUE_H_
#define _QUEUE_H_

#include <stdlib.h>
#include <stdio.h>

typedef struct
{
	struct qnode *head;
	struct qnode *tail;
	int size;
} Queue;

struct qnode
{
	void *data;
	struct qnode* next;
};

Queue *qcreate(void);
int qsize(Queue *q);
void *qfront(Queue *q);
void qpop(Queue *q);
void qpush(Queue *q, void *d);
void qclear(Queue **q);
void qprint_i(Queue *q);
void qprint_s(Queue *q);

#endif//_QUEUE_H_
