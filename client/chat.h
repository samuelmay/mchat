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

#define CLIENT_PORT 31181

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

struct server_options {
	char username[USERNAME_LEN];
	char password[PASSWORD_LEN];
	unsigned short port;
	struct in_addr ip;
	/* for human readableness */
	unsigned short port_hostformat;
	char ip_string[INET_ADDRSTRLEN];
};

/** functions **
****************/
void *poll_server (void *arg);
void print_user_list(void);

void parse_cmdline(int argc, char **argv, struct server_options *opts);
void print_help(void);

int client_connect(char *user);
int server_accept(char *user);

void send_message(int socket,char message[]);
void recieve_message(int socket, char message[INPUT_LEN]);

/** global variables **
 **********************/
extern struct reg_resp user_list;

/* LOCKS */

/* To prevent concurrency, each resource is assigned a lock and a
 * number. Resource locks MUST be aquired in ascending numerical order, and
 * released in descending numerical order, on pain of the filthiest debugging
 * you will ever have to do. */

/* NUMBER 1 */
extern pthread_mutex_t user_list_lock;

/* NUMBER 2 */
/* Needs to be held if printing to stdout. We won't worry about stdin, because
 * only the main thread uses that, and if we're printing to stderr we have more
 * pressing issues. */
extern pthread_mutex_t console_lock;
#endif /* _CHATCLIENT_H_ */
