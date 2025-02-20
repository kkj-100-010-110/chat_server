#ifndef _ERR_LOG_H_
#define _ERR_LOG_H_

#include <signal.h>
#include <time.h>
#include <pthread.h>
#include "common.h"
#include "thread_pool.h"

extern FILE *log_file;
extern volatile sig_atomic_t keep_running;
extern pthread_mutex_t log_mutex;

void get_timestamp(char *buffer, size_t size);
void log_to_file(void *arg);
void write_log(const char *message);
int setup_signals(int signo, void (*handler)(int));
void signal_handler(int signo);
void init_logging(const char *log_path);

#endif//_ERR_LOG_H_
