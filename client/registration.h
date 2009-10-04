#ifndef _REGISTRATION_H_
#define _REGISTRATION_H_

#include "options.h" 		/* for PASSWORD_LEN */
#include "user.h"		/* for USERNAME_LEN and MAX_USERS */

#define SERVER_TIMEOUT 60

struct reg_msg {
	char password[PASSWORD_LEN];
	char username[USERNAME_LEN];
	u_int16_t tcp_port;
};

struct reg_resp {
	u_int32_t nusers; /* number of users (no more than 50) */
	struct user_info {
		char username[USERNAME_LEN];
		u_int16_t tcp_port;
		u_int32_t ip_addr;
	} user[MAX_USERS]; /* info about each user */
};

void *registration_thread (void *arg);

/* CONDITION VARIABLES */
/* used to wake the registration server thread to update now */
extern pthread_cond_t registration_update;

#endif /* _REGISTRATION_H_ */
