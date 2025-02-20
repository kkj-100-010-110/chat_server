#include "err_log.h"

FILE *log_file = NULL;
volatile sig_atomic_t keep_running = 1;
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

void get_timestamp(char *buffer, size_t size)
{
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", tm_info);
}

void log_to_file(void *arg)
{
    char *message = (char *)arg;

    char timestamp[64];
    get_timestamp(timestamp, sizeof(timestamp));

    pthread_mutex_lock(&log_mutex);
    if (log_file) {
        fprintf(log_file, "[%s] %s\n", timestamp, message);
        fflush(log_file);
    }
    pthread_mutex_unlock(&log_mutex);

    free(message);
}

void write_log(const char *message)
{
    char *log_message = malloc(strlen(message) + 1);
    if (!log_message) {
        pr_err("malloc failed for log message");
        return;
    }
    strcpy(log_message, message);

    Task log_task;
    log_task.function = (void (*)(void *))log_to_file;
    log_task.arg = log_message;

    if (!threadpool_add_task(pool, log_task.function, log_task.arg)) {
        pr_err("Failed to add log task to thread pool");
        free(log_message);
    }
}

int setup_signals(int signo, void (*handler)(int))
{
	struct sigaction sa;
	sa.sa_handler = handler;
	sa.sa_flags = 0;

	if (sigaction(signo, &sa, NULL) == -1) {
		pr_err("sigaction() failed");
		return -1;
	}
	return 0;
}

void signal_handler(int signo)
{
    char log_message[256];

    snprintf(log_message
			, sizeof(log_message)
			, "Received signal %d (%s)"
			, signo
			, strsignal(signo));

    write_log(log_message);

    if (signo == SIGINT || signo == SIGTERM) {
        pr_out("Terminating server...");
		write_log("Termination signal received. Initiating shutdown...");
		keep_running = 0;
    }  else if (signo == SIGHUP) {
        write_log("Reload signal received. Reloading configuration...");
    } else {
        write_log("Unhandled signal received.");
    }
}

void init_logging(const char *log_path)
{
    log_file = fopen(log_path, "a");
    if (!log_file) {
        pr_out("Fail to open log file");
        exit(EXIT_FAILURE);
    }
    write_log("Log started.");
}
