#include "thread_pool.h"

ThreadPool *pool = NULL;
pthread_t *threads = NULL;

ThreadPool *threadpool_create()
{
    ThreadPool *pool = (ThreadPool *)malloc(sizeof(ThreadPool));
    if (!pool) {
		pr_err("malloc() failed");
		return NULL;
	}
    pool->front = 0;
    pool->rear = 0;
    pool->count = 0;
    pool->stop = false;

    pthread_mutex_init(&pool->lock, NULL);
    pthread_cond_init(&pool->cond_non_empty, NULL);
    pthread_cond_init(&pool->cond_non_full, NULL);

    return pool;
}

bool threadpool_add_task(ThreadPool *pool, void (*function)(void *), void *arg)
{
    pthread_mutex_lock(&pool->lock);

    while (pool->count == MAX_QUEUE_SIZE && !pool->stop) {
        pthread_cond_wait(&pool->cond_non_full, &pool->lock);
    }

    if (pool->stop) {
        pthread_mutex_unlock(&pool->lock);
        return false;
    }

    pool->queue[pool->rear].function = function;
    pool->queue[pool->rear].arg = arg;
    pool->rear = (pool->rear + 1) % MAX_QUEUE_SIZE;
    pool->count++;

    pthread_cond_signal(&pool->cond_non_empty);
    pthread_mutex_unlock(&pool->lock);

    return true;
}

void *worker_thread(void *arg)
{
    ThreadPool *pool = (ThreadPool *)arg;

    while (true) {
        pthread_mutex_lock(&pool->lock);

        while (pool->count == 0 && !pool->stop) {
            pthread_cond_wait(&pool->cond_non_empty, &pool->lock);
        }

        if (pool->stop && pool->count == 0) {
            pthread_mutex_unlock(&pool->lock);
            break;
        }

        Task task = pool->queue[pool->front];
        pool->front = (pool->front + 1) % MAX_QUEUE_SIZE;
        pool->count--;

        pthread_cond_signal(&pool->cond_non_full);
        pthread_mutex_unlock(&pool->lock);

        task.function(task.arg);
    }

    return NULL;
}

pthread_t *threadpool_start(ThreadPool *pool, int num_threads)
{
    pthread_t *threads = (pthread_t *)malloc(num_threads * sizeof(pthread_t));
    if (!threads) return NULL;

    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, worker_thread, pool);
    }

    return threads;
}

void threadpool_destroy(ThreadPool *pool, pthread_t *threads, int num_threads)
{
    pthread_mutex_lock(&pool->lock);
    pool->stop = true;
    pthread_cond_broadcast(&pool->cond_non_empty);
    pthread_cond_broadcast(&pool->cond_non_full);
    pthread_mutex_unlock(&pool->lock);

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    free(threads);
    pthread_mutex_destroy(&pool->lock);
    pthread_cond_destroy(&pool->cond_non_empty);
    pthread_cond_destroy(&pool->cond_non_full);
    free(pool);
}
