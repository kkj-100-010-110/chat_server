#include "server.h"

int server(char *port)
{
	char buf[1024];
	
    pool = threadpool_create();
    threads = threadpool_start(pool, MAX_CPU);

	init_logging("./log/err.log");

	if (setup_signals(SIGINT, signal_handler) == -1) {
		pr_err("Failed to set up SIGINT");
		exit(EXIT_FAILURE);
	}
    if (setup_signals(SIGTERM, signal_handler) == -1) {
		pr_err("Failed to set up SIGTERM");
		exit(EXIT_FAILURE);
	}
    if (setup_signals(SIGHUP, signal_handler) == -1) {
		pr_err("Failed to set up SIGHUP");
		exit(EXIT_FAILURE);
	}

	RB_tree *users;
	users = rb_create(comparison_i, free_key, free_data);
	RB_tree *rooms;
	rooms = rb_create(comparison_s, free_key, free_data);

	int fd_listener = set_address_and_listen_socket(port);

	/* epoll create */
	int epollfd = epoll_create(1); // size doesn't matter
	if (epollfd == -1) {
		pr_err("Fail: epoll_create()");
		exit(EXIT_FAILURE);
	}
	struct epoll_event *ep_events; // a structure for epoll_wait
	ep_events = calloc(MAX_EP_EVENTS, sizeof(struct epoll_event));
	if (ep_events == NULL) {
		pr_err("Fail: calloc()");
		exit(EXIT_FAILURE);
	}

	ADD_EV(epollfd, fd_listener);

	pr_out("[Server]: starts");

	while (keep_running)
	{
		pr_out("Waiting... ");
		int ret_poll = epoll_wait(epollfd, ep_events, MAX_EP_EVENTS, -1);
		if (ret_poll == -1) {
			if (errno == EINTR) { break; }
			pr_err("Fail: epoll_wait()");
			break;
		}
		pr_out("epoll_wait() returned: %d", ret_poll);
		for (int i = 0; i < ret_poll; i++) {
			if (ep_events[i].events & EPOLLIN) { // read event
				if (ep_events[i].data.fd == fd_listener) { // check listen socket
					struct sockaddr_storage saddr_c;
					while (1) {
						socklen_t len_saddr = sizeof(saddr_c);
						int fd = accept(fd_listener, (struct sockaddr *)&saddr_c, &len_saddr);
						if (fd == -1) {
							if (errno == EAGAIN) {
								break;
							}
							pr_err("Fail: accept()");
							break;
						}
						fcntl_setnb(fd);
						ADD_EV(epollfd, fd); // register a new connection
						int *fd_cp = (int *)malloc(sizeof(int));
						if (fd_cp == NULL) {
							pr_err("malloc() failed");
							exit(EXIT_FAILURE);
						}
						*fd_cp = fd;
						rb_insert(users, (void *)fd_cp, (void *)create_user(fd));
						pr_out("New client connect, fd(%d)", fd);
					}
					continue; // if taken all connections, back to epoll_wait
				} else {
					int ret_recv = recv(ep_events[i].data.fd, buf, sizeof(buf), 0);
					if (ret_recv == -1) { // EAGAIN, EWOULDBLOCK or EINTR
						if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
							continue;
						} else {
							handle_client_disconnection(ep_events[i].data.fd, users, rooms);
							close(ep_events[i].data.fd);
							continue;
						}
					}
					if (ret_recv == 0) { // passive close
						pr_out("Client, fd(%d): disconnected", ep_events[i].data.fd);
						handle_client_disconnection(ep_events[i].data.fd, users, rooms);
						DEL_EV(epollfd, ep_events[i].data.fd);
						close(ep_events[i].data.fd);
					}
					buf[ret_recv] = '\0';
					trim_newline(buf);
					if (buf[0] == '\0') {
						pr_out("Empty message received, ignoring");
						continue;
					}
					char *cmd, *arg;
					parse_command(buf, &cmd, &arg);
					pr_out("recv(fd=%d,n=%d) = %.*s", ep_events[i].data.fd, ret_recv, ret_recv, buf);
					execute_command(ep_events[i].data.fd, cmd, arg, users, rooms);
				}
			} else if (ep_events[i].events & EPOLLPRI) { // OOB data
				pr_out("EPOLLPRI: urgent data detected");
				int ret_recv = recv(ep_events[i].data.fd, buf, 1, MSG_OOB);
				if (ret_recv == -1) {
					continue;
				}
				pr_out("recv(fd=%d,n=1) = %.*s(OOB)", ep_events[i].data.fd, (int)sizeof(buf), buf);
			} else if (ep_events[i].events & EPOLLERR) {
				int error = 0;
				socklen_t len = sizeof(error);
				if (getsockopt(ep_events[i].data.fd, SOL_SOCKET, SO_ERROR, &error, &len) == 0) {
					pr_out("Socket error on fd(%d): %s", ep_events[i].data.fd, strerror(error));
				} else {
					pr_err("getsockopt() failed");
				}
				DEL_EV(epollfd, ep_events[i].data.fd);
				close(ep_events[i].data.fd);
				rb_remove(users, (void *)(intptr_t)ep_events[i].data.fd);
				pr_out("Closed socket fd(%d) due to EPOLLERR\n", ep_events[i].data.fd);
			} else {
				pr_err("Unhandled epoll event(%d) on fd(%d) err(%s)"
						, ep_events[i].events
						, ep_events[i].data.fd
						, strerror(errno));
			}
		} // for loop
	} // while loop
    threadpool_destroy(pool, threads, MAX_CPU);
	rb_remove_tree(users);
	rb_remove_tree(rooms);
	users = NULL;
	rooms = NULL;
	close(epollfd);
	return 0;
}

int add_ev(int efd, int fd)
{
	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLPRI;
	ev.data.fd = fd;
	if (epoll_ctl(efd, EPOLL_CTL_ADD, fd, &ev) == -1) {
		pr_out("fd(%d) EPOLL_CTL_ADD Error(%d:%s)", fd, errno, strerror(errno));
		return -1;
	}
	return 0;
}

int del_ev(int efd, int fd)
{
	if (epoll_ctl(efd, EPOLL_CTL_DEL, fd, NULL) == -1) {
		pr_out("fd(%d) EPOLL_CTL_DEL Error(%d:%s)", fd, errno, strerror(errno));
		return -1;
	}
	close(fd);
	return 0;
}

int fcntl_setnb(int fd)
{
	if (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK) == -1) {
		return errno;
	}
	return 0;
}

void set_sockopt(int fd)
{
	int sockopt = 1;

	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt)) == -1) {
		pr_err("Fail: setsockopt(SO_REUSEADDR)");
		exit(EXIT_FAILURE);
	}
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &sockopt, sizeof(sockopt)) == -1) {
		pr_err("Fail: setsockopt(SO_REUSEPORT)");
		exit(EXIT_FAILURE);
	}
	if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &sockopt, sizeof(sockopt)) == -1) {
		pr_err("Fail: setsockopt(TCP_NODELAY)");
		exit(EXIT_FAILURE);
	}

	struct linger ling;
    ling.l_onoff = 1;
    ling.l_linger = 10;

    if (setsockopt(fd, SOL_SOCKET, SO_LINGER, &ling, sizeof(ling)) == -1) {
        pr_err("Fail: setsockopt(SO_LINGER)");
        exit(EXIT_FAILURE);
    }
}

void print_sock_info(int fd)
{
	struct sockaddr_storage saddr_s;
	socklen_t len_saddr = sizeof(saddr_s);
	getsockname(fd, (struct sockaddr *)&saddr_s, &len_saddr);
	if (saddr_s.ss_family == AF_INET) {
		pr_out("[Server]: IPv4 Port: #%d"
				, ntohs(((struct sockaddr_in *)&saddr_s)->sin_port));
	} else if (saddr_s.ss_family == AF_INET6) {
		pr_out("[Server]: IPv6 Port: #%d"
				, ntohs(((struct sockaddr_in6 *)&saddr_s)->sin6_port));
	} else {
		pr_out("[Server]: (ss_family=%d)", saddr_s.ss_family);
	}
}

int set_address_and_listen_socket(char *port)
{
	int fd_listener;
	struct addrinfo ai;
	struct addrinfo *ai_ret;
	memset(&ai, 0, sizeof(ai));
	ai.ai_family = AF_UNSPEC;
	ai.ai_socktype = SOCK_STREAM;
	ai.ai_flags = AI_ADDRCONFIG | AI_PASSIVE;
	int rc_gai;
	rc_gai = getaddrinfo(NULL, port, &ai, &ai_ret);
	if (rc_gai != 0) {
		pr_err("Fail: getaddrinfo():%s", gai_strerror(rc_gai));
		exit(EXIT_FAILURE);
	}

	fd_listener = socket(ai_ret->ai_family, ai_ret->ai_socktype, ai_ret->ai_protocol);
	if (fd_listener == -1) {
		pr_err("Fail: socket()");
		exit(EXIT_FAILURE);
	}

	/* socket options */
	set_sockopt(fd_listener);

	if (bind(fd_listener, ai_ret->ai_addr, ai_ret->ai_addrlen) == -1) {
		perror("Fail: bind()");
		exit(EXIT_FAILURE);
	}

	fcntl_setnb(fd_listener);
	print_sock_info(fd_listener);

	if (listen(fd_listener, LISTEN_BACKLOG) == -1) {
		perror("Fail: listen()");
		exit(EXIT_FAILURE);
	}

	freeaddrinfo(ai_ret);

	return fd_listener;
}

void handle_client_disconnection(int fd, RB_tree *users, RB_tree *rooms)
{
    User *user = (User *)rb_find(users->root, users->compare, &fd)->data;

    if (user != NULL && user->in_room) {
        int *room_member = (int *)rb_find(rooms->root, rooms->compare, user->room_name)->data;
        for (int i = 0; i < MAX_MEMBER; i++) {
            if (room_member[i] == fd) {
                room_member[i] = 0;
                break;
            }
        }

        bool room_is_empty = true;
        for (int i = 0; i < MAX_MEMBER; i++) {
            if (room_member[i] != 0) {
                room_is_empty = false;
                break;
            }
        }

		size_t len1 = strlen(user->id);
		size_t len2 = strlen(user->room_name);
		size_t len = len1 + len2 + 2;
		char * leave_msg = (char *)malloc(sizeof(char) * len);
		if (!leave_msg) {
			pr_err("malloc() failed");
			exit(EXIT_FAILURE);
		}

		snprintf(leave_msg, len, "%s|%s", user->room_name, user->id);
		send_message(fd, "LEAVE_SUCCESS %s", leave_msg);
		char *msg = "left the room.";
		exit_msg(fd, rooms, users, msg);

		if (room_is_empty) {
			rb_remove(rooms, user->room_name);
			send_message(fd, "ROOM_DELETE %s", user->room_name);
			rb_remove(users, &fd);
			return;
		}

		free(leave_msg);
		free(user->room_name);
        user->in_room = false;
        user->room_name = NULL;
    }
    rb_remove(users, &fd);
}
