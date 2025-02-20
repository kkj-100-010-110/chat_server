#ifndef _COMMAND_H_
#define _COMMAND_H_

#include "common.h"
#include "thread_pool.h"
#include "user.h"
#include <sys/socket.h>

#define MAX_MEMBER 20

typedef struct {
	int fd;
	RB_node *room_node;
	User *user;
	const char *msg;
} BroadcastMsgArg;

typedef struct {
	char *msg;
	RB_node *room_node;
} BroadcastInfoArg;

void execute_command(int fd, char *cmd, char *arg, RB_tree *users, RB_tree *rooms);
void parse_command(const char *msg, char **cmd, char **arg);
void trim_newline(char *str);
void send_message(int fd, const char *format, const char *arg);
void broadcast_msg(void *arg);
void broadcast_info(void *arg);
void get_room_list(RB_node *n, char **room_list, size_t *len);
bool check_duplicate_id(RB_node *n, char *id);
void process_id(int fd, char *arg, RB_tree *users);
void process_list(int fd, RB_tree *rooms);
void process_open(int fd, char *arg, RB_tree *users, RB_tree *rooms);
void process_join(int fd, char *arg, RB_tree *users, RB_tree *rooms);
void process_msg(int fd, char *arg, RB_tree *users, RB_tree *rooms);
void process_leave(int fd, RB_tree *users, RB_tree *rooms);
void process_member(int fd, RB_tree *users, RB_tree *rooms);
void process_info(RB_node *room, RB_tree *users);
void exit_msg(int fd, RB_tree *rooms, RB_tree *users, char *msg);

#endif//_COMMAND_H_
