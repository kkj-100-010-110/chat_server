#ifndef _SERVER_H_
#define _SERVER_H_

#include "common.h"
#include "user.h"
#include "command.h"
#include "thread_pool.h"
#include "err_log.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <fcntl.h>

#define LISTEN_BACKLOG 256
#define MAX_EP_EVENTS 256

#define ADD_EV(a, b) if (add_ev(a, b) == -1) { pr_err("Fail: add_ev"); exit(1); }
#define DEL_EV(a, b) if (del_ev(a, b) == -1) { pr_err("Fail: del_ev"); exit(1); }

int add_ev(int efd, int fd);	// add a fd to epoll
int del_ev(int efd, int fd);	// delete a fd from epoll
int fcntl_setnb(int fd);		// nonblocking mode
void set_sockopt(int fd);		// set socket options
void print_sock_info(int fd);	// print socket info
int set_address_and_listen_socket(char *port); // set up address info and listen socket
void handle_client_disconnection(int fd, RB_tree *users, RB_tree *rooms);
int server(char *port);

#endif//_SERVER_H_
