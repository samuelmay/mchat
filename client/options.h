#ifndef _OPTIONS_H_
#define _OPTIONS_H_

#include "user.h"

/* #define PASSWORD "3118miniproject" */
#define DEFAULT_PASSWORD "3118miniproject"
/* #define DEFAULT_SERVER_IP "149.171.92.193" */
#define DEFAULT_SERVER_IP "127.0.0.1"
#define DEFAULT_SERVER_PORT 31180
#define DEFAULT_USERNAME "guest"

#define PASSWORD_LEN 16

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

/* FUNCTIONS */
void parse_cmdline(int argc, char **argv, struct options *opts);
void print_help(void);

/* GLOBAL VARIABLES */
/* THIS IS READ ONLY!!! It is populated before any threads are started. */
extern struct options opts;

#endif /* _OPTIONS_H_ */
