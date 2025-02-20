#include "command.h"

void execute_command(int fd, char *cmd, char *arg, RB_tree *users, RB_tree *rooms)
{
	if (strcmp(cmd, "ID") == 0) { process_id(fd, arg, users); }
	else if (strcmp(cmd, "LIST") == 0) { process_list(fd, rooms); }
	else if (strcmp(cmd, "OPEN") == 0) { process_open(fd, arg, users, rooms); }
	else if (strcmp(cmd, "JOIN") == 0) { process_join(fd, arg, users, rooms); }
	else if (strcmp(cmd, "MSG") == 0) { process_msg(fd, arg, users, rooms); }
	else if (strcmp(cmd, "LEAVE") == 0) { process_leave(fd, users, rooms); }
	else if (strcmp(cmd, "MEMBER") == 0) { process_member(fd, users, rooms); }
	free(cmd);
	if (arg)
		free(arg);
}

void parse_command(const char *msg, char **cmd, char **arg)
{
	size_t i = 0;
    while (msg[i] != '\0' && msg[i] != ' ' && msg[i] != '\r' && msg[i] != '\n') {
        i++;
    }

    if (i == 0) {
        *cmd = NULL;
        *arg = NULL;
        return;
    }

    *cmd = (char *)malloc(sizeof(char) * (i + 1));
    if (*cmd == NULL) {
		pr_err("malloc() failed");
        exit(EXIT_FAILURE);
    }

    strncpy(*cmd, msg, i);
    (*cmd)[i] = '\0';

	pr_out("Command: %s", *cmd);

    if (msg[i] == ' ') {
        *arg = strdup(&msg[i + 1]);
        if (*arg == NULL) {
			pr_err("malloc() failed");
            free(*cmd);
            exit(EXIT_FAILURE);
        }
		trim_newline(*arg);
    } else {
        *arg = NULL;
    }
}

void trim_newline(char *str)
{
	char *newline = strpbrk(str, "\r\n");
	if (newline) {
		*newline = '\0';
	}
}

void send_message(int fd, const char *format, const char *arg)
{
	size_t len = snprintf(NULL, 0, format, arg) + 3; // \r\n\0
	char *message = (char*)malloc(len);
	if (message == NULL) {
		perror("malloc() failed");
		exit(EXIT_FAILURE);
	}

	snprintf(message, len, format, arg);
	strncat(message, "\r\n", len - strlen(message) - 1);

	if (send(fd, message, strlen(message), 0) == -1) {
		pr_err("send() failed");
	} else {
		trim_newline(message);
		pr_out("Sent message: %s", message);
	}

	free(message);
}

void broadcast_msg(void *arg)
{
	BroadcastMsgArg *args = (BroadcastMsgArg *)arg;
	int fd = args->fd;
	RB_node *room_node = args->room_node;
	User *user = args->user;
	const char *msg = args->msg;

	size_t len = snprintf(NULL, 0, "MSG [%s][%s]:%s", (char *)room_node->key, user->id, msg) + 3;
	char *message = malloc(len);
	if (message == NULL) {
		perror("malloc() failed");
		exit(EXIT_FAILURE);
	}

	snprintf(message, len, "MSG [%s][%s]:%s", (char *)room_node->key, user->id, msg);
	strncat(message, "\r\n", len - strlen(message) - 1);

	int *room_member = (int *)room_node->data;
	int member;
	for (int i = 0; i < MAX_MEMBER; i++) {
		member = room_member[i];
		if (member != 0 && member != fd) {
			if (send(member, message, strlen(message), 0) == -1) {
				pr_err("send() error");
			}
		}
	}
	trim_newline(message);
	pr_out("Sent message: %s", message);

	free(message);
	free(args);
}

void broadcast_info(void *arg)
{
	BroadcastInfoArg *args = (BroadcastInfoArg *)arg;
	char *msg = args->msg;
	RB_node *room = args->room_node;

	size_t len = snprintf(NULL, 0, "INFO %s", msg) + 3;
	char *message = malloc(len);
	if (message == NULL) {
		perror("malloc() failed");
		exit(EXIT_FAILURE);
	}

	snprintf(message, len, "INFO %s", msg);
	strncat(message, "\r\n", len - strlen(message) - 1);

	int *room_member = (int *)room->data;
	int member;
	for (int i = 0; i < MAX_MEMBER; i++) {
		member = room_member[i];
		if (member != 0) {
			if (send(member, message, strlen(message), 0) == -1) {
				pr_err("send() error");
			}
		}
	}
	trim_newline(message);
	pr_out("Sent message: %s", message);

	free(message);
	free(msg);
	free(args);
}

void get_room_list(RB_node *n, char **room_list, size_t *len)
{
	if (!n)
		return;

	get_room_list(n->link[0], room_list, len);

	size_t room_name_len = strlen(n->key);
	*room_list = realloc(*room_list, *len + room_name_len + 2);
	if (!room_list) {
		pr_err("realloc() failed");
		exit(EXIT_FAILURE);
	}
	snprintf(*room_list + *len, room_name_len + 2, "%s,", (char *)n->key);
	*len += room_name_len + 1;

	get_room_list(n->link[1], room_list, len);
}

bool check_duplicate_id(RB_node *n, char *id)
{
	if (!n)
		return false;

	User *user = (User *)n->data;
	if (strcmp(user->id, id) == 0) return true;

	return (check_duplicate_id(n->link[0], id) || check_duplicate_id(n->link[1], id));
}

void process_id(int fd, char *arg, RB_tree *users)
{
	size_t size = strlen(arg);
	if (size > 20) {
		send_message(fd, "ID_FAIL failed to create your id because it's too long", NULL);
		return;
	}
	if (check_duplicate_id(users->root, arg)) {
		send_message(fd, "ID_FAIL failed to create your id because it already exists", NULL);
		return;
	}
	User *user = (User *)rb_find(users->root, users->compare, &fd)->data;
	memset(user->id, '\0', sizeof(user->id));
	strncpy(user->id, arg, size);
	send_message(fd, "ID %s", arg);
	return;
}

void process_list(int fd, RB_tree *rooms)
{
	char *room_list = strdup("");
	size_t len = 0;
	get_room_list(rooms->root, &room_list, &len);
	if (len > 0)
		room_list[len - 1] = '\0';
	send_message(fd, "LIST %s", room_list);
	free(room_list);
}

void process_open(int fd, char *arg, RB_tree *users, RB_tree *rooms)
{
	if (rb_find(rooms->root, rooms->compare, arg) != NULL) {
		send_message(fd, "OPEN_FAIL %s room already exists", arg);
		return;
	}
	User *user = (User *)rb_find(users->root, users->compare, &fd)->data;
	if (user->in_room == true) {
		send_message(fd, "OPEN_FAIL You already joined a room", NULL);
		return;
	}

	rb_insert(rooms, strdup(arg), (int *)calloc(MAX_MEMBER, sizeof(int)));

	int *room_member = (int *)rb_find(rooms->root, rooms->compare, arg)->data;
	room_member[0] = fd;
	user->in_room = true;
	user->room_name = strdup(arg);
	size_t len1 = strlen(user->id);
	size_t len2 = strlen(arg);
	size_t len = len1 + len2 + 2;
	char * join_msg = (char *)malloc(sizeof(char) * len);
	if (!join_msg) {
		pr_err("malloc() failed");
		exit(EXIT_FAILURE);
	}
	snprintf(join_msg, len, "%s|%s", arg, user->id);
	send_message(fd, "JOIN_SUCCESS %s", join_msg);
	char *msg = "joined the room.";
	process_msg(fd, msg, users, rooms);
	free(join_msg);

	send_message(fd, "OPEN_SUCCESS %s", arg);
	return;
}

void process_join(int fd, char *arg, RB_tree *users, RB_tree *rooms)
{
	if (arg == NULL || strlen(arg) == 0) {
		send_message(fd, "JOIN_FAIL Invalid argument", NULL);
		return;
	}
	User *user = (User *)rb_find(users->root, users->compare, &fd)->data;
	if (user->in_room == true) {
		send_message(fd, "JOIN_FAIL You already joined a room named %s", user->room_name);
		return;
	}
	RB_node *room = rb_find(rooms->root, rooms->compare, arg);
	if (room == NULL) {
		send_message(fd, "JOIN_FAIL No room named %s", arg);
		return;
	}
	int *room_member = (int *)room->data;
	for (int i = 0; i < MAX_MEMBER; i++)
	{
		if (room_member[i] == 0) {
			room_member[i] = fd;
			user->in_room = true;
			user->room_name = strdup(arg);
			size_t len1 = strlen(user->id);
			size_t len2 = strlen(arg);
			size_t len = len1 + len2 + 2;
			char * join_msg = (char *)malloc(sizeof(char) * len);
			if (!join_msg) {
				pr_err("malloc() failed");
				exit(EXIT_FAILURE);
			}
			snprintf(join_msg, len, "%s|%s", arg, user->id);
			send_message(fd, "JOIN_SUCCESS %s", join_msg);
			char *msg = "joined the room.";
			process_msg(fd, msg, users, rooms);
			process_info(room, users);
			free(join_msg);
			return;
		}
	}
	send_message(fd, "JOIN_FAIL %s room is full", arg);
	return;
}

void process_msg(int fd, char *arg, RB_tree *users, RB_tree *rooms)
{
	User *user = (User *)rb_find(users->root, users->compare, &fd)->data;
	if (user->in_room == false) {
		send_message(fd, "MSG_FAIL You haven't joined any rooms", NULL);
		return;
	}
	BroadcastMsgArg *args = malloc(sizeof(BroadcastMsgArg));
	if (!args) {
		pr_err("malloc() failed");
		exit(EXIT_FAILURE);
	}
	args->fd = fd;
	args->room_node = rb_find(rooms->root, rooms->compare, user->room_name);
	args->user = user;
	args->msg = strdup(arg);

	Task task;
	task.function = (void (*)(void *))broadcast_msg;
	task.arg = args;

	threadpool_add_task(pool, task.function, task.arg);
}


void process_leave(int fd, RB_tree *users, RB_tree *rooms)
{
	User *user = (User *)rb_find(users->root, users->compare, &fd)->data;
	if (user->room_name != NULL) {
		RB_node *room = rb_find(rooms->root, rooms->compare, user->room_name);
		int *room_member = (int *)room->data;
		int i;
		for (i = 0; i < MAX_MEMBER; i++) {
			if (room_member[i] == fd) {
				room_member[i] = 0;
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
		
		for (i = 0; i < MAX_MEMBER; i++) {
			if (room_member[i] != 0) {
				break;
			}
		}
		if (i == MAX_MEMBER) {
			rb_remove(rooms, user->room_name);
			send_message(fd, "ROOM_DELETE %s", user->room_name);
		} else {
			char *msg = "left the room.";
			exit_msg(fd, rooms, users, msg);
			process_info(room, users);
		}

		free(user->room_name);
		free(leave_msg);
		user->room_name = NULL;
	}
	if (user->in_room == true) user->in_room = false;
}

void process_member(int fd, RB_tree *users, RB_tree *rooms)
{
	User *user = (User *)rb_find(users->root, users->compare, &fd)->data;
	if (user->in_room == false) {
		send_message(fd, "MEMBER_FAIL You haven't joined any room.", NULL);
		return;
	}

	size_t len = strlen(user->room_name) + 2;
	char *member_list = (char *)malloc(sizeof(char) * len);
	if (!member_list) {
		pr_err("malloc() failed");
		exit(EXIT_FAILURE);
	}
	snprintf(member_list, len, "%s|", user->room_name);

	len = strlen(member_list);

	int *room_member = (int *)rb_find(rooms->root, rooms->compare, user->room_name)->data;
	User *member;
	size_t member_id_len;
	for (int i = 0; i < MAX_MEMBER; i++)
	{
		if (room_member[i] != 0) {
			member = (User *)rb_find(users->root, users->compare, &room_member[i])->data;
			member_id_len = strlen(member->id);
			member_list = realloc(member_list, len + member_id_len + 2);
			if (!member_list) {
				pr_err("realloc() failed");
				exit(EXIT_FAILURE);
			}
			snprintf(member_list + len, member_id_len + 2, "%s,", member->id);
			len += member_id_len + 1;
		}
	}
	if (len > 0)
		member_list[len - 1] = '\0';
	send_message(fd, "MEMBER_SUCCESS %s", member_list);
	return;
}

void process_info(RB_node *room, RB_tree *users)
{
	size_t len = strlen((char *)room->key) + 2;
	char *info = (char *)malloc(sizeof(char) * len);
	if (!info) {
		pr_err("malloc() failed");
		exit(EXIT_FAILURE);
	}
	snprintf(info, len, "%s|", (char *)room->key);

	len = strlen(info);

	int *room_member = (int *)room->data;
	User *member;
	size_t member_id_len;
	for (int i = 0; i < MAX_MEMBER; i++)
	{
		if (room_member[i] != 0) {
			member = (User *)rb_find(users->root, users->compare, &room_member[i])->data;
			member_id_len = strlen(member->id);
			info = realloc(info, len + member_id_len + 2);
			if (!info) {
				pr_err("realloc() failed");
				exit(EXIT_FAILURE);
			}
			snprintf(info + len, member_id_len + 2, "%s,", member->id);
			len += member_id_len + 1;
		}
	}
	if (len > 0)
		info[len - 1] = '\0';

	BroadcastInfoArg *args = malloc(sizeof(BroadcastInfoArg));
	if (!args) {
		pr_err("malloc() failed");
		exit(EXIT_FAILURE);
	}
	args->room_node = room;
	args->msg = info;

	Task task;
	task.function = (void (*)(void *))broadcast_info;
	task.arg = args;

	threadpool_add_task(pool, task.function, task.arg);
	return;
}

void exit_msg(int fd, RB_tree *rooms, RB_tree *users, char *msg)
{
	User *user = (User *)rb_find(users->root, users->compare, &fd)->data;
	if (user->in_room == false) {
		send_message(fd, "MSG_FAIL You haven't joined any rooms", NULL);
		return;
	}
	RB_node *room = rb_find(rooms->root, rooms->compare, user->room_name);

	size_t len = snprintf(NULL, 0, "MSG [%s][%s]:%s", (char *)room->key, user->id, msg) + 3;
	char *message = malloc(len);
	if (message == NULL) {
		perror("malloc() failed");
		exit(EXIT_FAILURE);
	}

	snprintf(message, len, "MSG [%s][%s]:%s", (char *)room->key, user->id, msg);
	strncat(message, "\r\n", len - strlen(message) - 1);

	int *room_member = (int *)room->data;
	int member;
	for (int i = 0; i < MAX_MEMBER; i++) {
		member = room_member[i];
		if (member != 0 && member != fd) {
			if (send(member, message, strlen(message), 0) == -1) {
				pr_err("send() error");
			}
		}
	}
	trim_newline(message);
	pr_out("Sent message: %s", message);
	free(message);
}
