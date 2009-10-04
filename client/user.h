#ifndef _USER_H_
#define _USER_H_

#define USERNAME_LEN 14
#define MAX_USERS 50

struct user {
	char name[USERNAME_LEN];
	u_int16_t port;
	u_int32_t ip;
	int socket;
	unsigned int flags; 	/* connected, blocked */
};

/* bitmasks for the flags */
#define USER_CONNECTED 0x00000001
#define USER_BLOCKED   0x00000002

/* FUNCTIONS */
void print_user_list(void);
/* looks up user with the given connection details. Returns index into the user
 * list if found, -1 otherwise */
int lookup_user(char name[USERNAME_LEN]);
int lookup_socket(int fd);

/* GLOBAL VARIABLES */
extern struct user user_list[MAX_USERS];
extern int num_users;

/* LOCKS FOR THE GLOBAL VARIABLES */
/* To prevent deadlock, each resource is assigned a lock and a number. Resource
 * locks MUST be aquired in ascending numerical order, and released in
 * descending numerical order, on pain of the filthiest debugging you will ever
 * have to do. */
extern pthread_mutex_t user_list_lock; /* NUMBER 1 */

#endif /* _USER_H_ */
