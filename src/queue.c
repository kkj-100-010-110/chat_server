#include "queue.h"

Queue *qcreate(void)
{
	Queue *new_q = (Queue*)malloc(sizeof(Queue));
	if (!new_q) {
		fprintf(stderr, "malloc failed in qcreate()\n");
		return NULL;
	}

	new_q->head = NULL;
	new_q->tail = NULL;
	new_q->size = 0;
	
	return new_q;
}

int qsize(Queue *q)
{
	if (!q) {
		fprintf(stderr, "Error: Queue is null in qsize()\n");
		return -1;
	}
	return q->size;
}

void *qfront(Queue *q)
{
	if (!q) {
		fprintf(stderr, "Error: Queue is null in qfront()\n");
		return NULL;
	}
	return q->head->data;
}

void qpop(Queue *q)
{
	if (!q) {
		fprintf(stderr, "Error: Queue is null in qpop()\n");
		return;
	}
	if (!q->head) {
		fprintf(stderr, "Error: Queue is empty in qpop()\n");
		return;
	}
	struct qnode *tmp = q->head;
	q->head = q->head->next;
	q->size--;
	free(tmp);
	tmp = NULL;
}

void qpush(Queue *q, void *d)
{
	struct qnode *new_q = (struct qnode*)malloc(sizeof(struct qnode));
	if (!new_q) {
		fprintf(stderr, "malloc failed in qpush()\n");
		return;
	}
	new_q->data = d;
	new_q->next = NULL;
	if (!q->head) {
		q->head = new_q;
		q->tail = new_q;
	} else {
		q->tail->next = new_q;
		q->tail = q->tail->next;
	}
	q->size++;
}

void qclear(Queue **q)
{
	if (!*q) {
		printf("empty\n");
		return;
	}
	while ((*q)->size)
		qpop(*q);
	free(*q);
	*q = NULL;
}

void qprint_i(Queue *q)
{
	if (!q) {
		fprintf(stderr, "error: queue is null\n");
		return ;
	}
	struct qnode *tmp = q->head;
	while (tmp)
	{
		printf("%d ", *(int*)tmp->data);
		tmp = tmp->next;
	}
}

void qprint_s(Queue *q)
{
	if (!q) {
		fprintf(stderr, "error: queue is null\n");
		return ;
	}
	struct qnode *tmp = q->head;
	while (tmp)
	{
		printf("%s\n", (char*)tmp->data);
		tmp = tmp->next;
	}
}
