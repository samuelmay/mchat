#ifndef _CHATCLIENT_H_
#define _CHATCLIENT_H_

/* #define PASSWORD "3118miniproject" */
#define DEFAULT_PASSWORD "3118miniproject"
/* #define DEFAULT_SERVER_IP "149.171.92.193" */
#define DEFAULT_SERVER_IP "127.0.0.1"
#define DEFAULT_SERVER_PORT 31180
#define DEFAULT_USERNAME "guest"

#define PASSWORD_LEN 16
#define USERNAME_LEN 14

#define SERVER_TIMEOUT 60

#define INPUT_LEN 256

/** structures **
*****************/
struct reg_msg {
	char password[PASSWORD_LEN];
	char username[USERNAME_LEN];
	unsigned short tcp_port;
};

struct reg_resp {
	unsigned long nusers; /* number of users (no more than 50) */
	struct user_info {
		char username[USERNAME_LEN];
		unsigned short tcp_port;
		unsigned long ip_addr;
	} user[50]; /* info about each user */
};

struct options {
	char username[USERNAME_LEN];
	char password[PASSWORD_LEN];
	unsigned short server_port;
	unsigned short local_port;
	struct in_addr ip;
	/* for human readableness */ 
	unsigned short server_port_h;
	unsigned short local_port_h;
	char ip_string[INET_ADDRSTRLEN];
};

struct connection {
	int socket;
	char remote_user[USERNAME_LEN];
};

struct user {
	char name[USERNAME_LEN];
	unsigned short port;
	unsigned long ip;
	int socket;
};

/** functions **
****************/

/* registration.c */
void *registration_thread (void *arg);

/* server.c */
int start_listening(struct options *opts);
void *server_thread(void *arg);
void accept_new_connection(int fd);
void receive_message(int fd);

/* user.c */
void print_user_list(void);
/* looks up user with the given connection details. Returns index into the user
 * list if found, -1 otherwise */
int lookup_user(char name[USERNAME_LEN]);

/* options.c */
void parse_cmdline(int argc, char **argv, struct options *opts);
void print_help(void);

/* client.c */
void client_connect(char remote_user[USERNAME_LEN]);
void broadcast_message(char message[INPUT_LEN]);


/** global variables **
 **********************/
extern struct user user_list[50];
extern int num_users;

/* THIS IS READ ONLY!!! It is populated before any threads are started. */
extern struct options opts;

/* CONDITION VARIABLES */
/* used to wake the registration server thread to update now */
extern pthread_cond_t server_update;

/* LOCKS */
/*********/
/* To prevent deadlock, each resource is assigned a lock and a number. Resource
 * locks MUST be aquired in ascending numerical order, and released in
 * descending numerical order, on pain of the filthiest debugging you will ever
 * have to do. */

/* NUMBER 1 */
extern pthread_mutex_t user_list_lock;

/* NUMBER 2 */
/* Needs to be held if printing to stdout. We won't worry about stdin, because
 * only the main thread uses that, and if we're printing to stderr we have more
 * pressing issues. */
extern pthread_mutex_t console_lock;

/* this is unfortunately GCC specific. DON'T EVER USE IF YOU HOLD ANOTHER
 * LOCK. */
#define printf_threadsafe(format,...) do {		\
		pthread_mutex_lock(&console_lock);	\
		printf(format, ## __VA_ARGS__);		\
		pthread_mutex_unlock(&console_lock);	\
	} while (0)

#endif /* _CHATCLIENT_H_ */
