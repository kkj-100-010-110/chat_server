#ifndef _USER_H_
#define _USER_H_

#include <stdbool.h>
#include "common.h"

typedef struct {
	int fd;
	char id[20];
	char *room_name;
	bool manager;
	bool in_room;
} User;

User *create_user(int fd);

#endif//_USER_H_
