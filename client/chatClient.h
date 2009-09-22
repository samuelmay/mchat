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

#define INPUT_BUF_SIZE 256

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

/** global variables **
 **********************/
extern struct server_options server_opts;

extern struct reg_resp user_list;
extern pthread_mutex_t user_list_lock;

#endif /* _CHATCLIENT_H_ */
