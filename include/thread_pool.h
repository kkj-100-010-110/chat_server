#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_

#include <pthread.h>

#include "common.h"

#define MAX_CPU sysconf(_SC_NPROCESSORS_ONLN)
#define MAX_QUEUE_SIZE 16

typedef struct {
    void (*function)(void *);
    void *arg;
} Task;

typedef struct {
    Task queue[MAX_QUEUE_SIZE];
    int front;
	int rear;
	int count;
    pthread_mutex_t lock;
    pthread_cond_t cond_non_empty;
    pthread_cond_t cond_non_full;
    bool stop;
} ThreadPool;

extern ThreadPool *pool;
extern pthread_t *threads;

ThreadPool *threadpool_create();
bool threadpool_add_task(ThreadPool *pool, void (*function)(void *), void *arg);
void *worker_thread(void *arg);
pthread_t *threadpool_start(ThreadPool *pool, int num_threads);
void threadpool_destroy(ThreadPool *pool, pthread_t *threads, int num_threads);

#endif//_THREAD_POOL_H_
