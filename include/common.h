#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <stdint.h>

#include "rb.h"

#define OK_MSG "\033[96m"
#define ERR_MSG "\033[91m"
#define INFO_MSG "\033[93m"
#define RESET_MSG "\033[0m"

#define print_msg(io, msgtype, arg...) \
	flockfile(io); \
	fprintf(io, "["#msgtype"][%s/%s:%03d]", __FILE__, __FUNCTION__, __LINE__); \
	fprintf(io, arg); \
	fputc('\n', io); \
	funlockfile(io);

#define pr_err(arg...) \
	fprintf(stderr, ERR_MSG); \
	print_msg(stderr, ERR, arg) \
	fprintf(stderr, RESET_MSG);
#define pr_out(arg...) \
	fprintf(stdout, INFO_MSG); \
	print_msg(stdout, REP, arg) \
	fprintf(stdout, RESET_MSG);


#endif//_COMMON_H_
