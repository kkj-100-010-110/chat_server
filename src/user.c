#include "user.h"

User *create_user(int fd)
{
	User *user = (User *)malloc(sizeof(User));
	if (!user) {
		pr_err("malloc() failed in create_user()");
		return NULL;
	}
	user->fd = fd;
	strcpy(user->id, "NO ID");
	user->room_name = NULL;
	user->manager = false;
	user->in_room = false;

	return user;
}
