#include "server.h"

void load_env();

int main(int argc, char **argv)
{
	/*
	if (argc > 2) {
		printf("%s [port number]\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	char *port;
	if (argc == 2) {
		port = strdup(argv[1]);
	} else {
		port = strdup("0"); // random port
	}
	server(port);
	*/

	load_env();
	char *port = getenv("PORT");
	if (!port) {
		pr_err("Failed to read env in getenv()");
		exit(EXIT_FAILURE);
	}

	(void)argc; (void)argv;

	server(port);

	return 0;
}

void load_env()
{
    FILE *file = fopen(".env", "r");
    if (file == NULL) {
        pr_err("fopen() failed");
        return;
    }

	pr_out("SET UP ENV");

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0;

        if (line[0] == '#' || strlen(line) == 0) {
            continue;
        }

        char *key = strtok(line, "=");
        char *value = strtok(NULL, "=");

        if (key != NULL && value != NULL) {
			setenv(key, value, 1);
            pr_out("KEY: %s, VALUE: %s\n", key, value);
        }
    }
    fclose(file);
}
