/*
 * registration.c
 *
 * Sam May
 * 22/09/09
 *
 * Thread that polls the server with registration messages
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#include "user.h"
#include "options.h"
#include "registration.h"

/** global variables **
 **********************/
pthread_mutex_t registration_update_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t registration_update = PTHREAD_COND_INITIALIZER;
pthread_mutex_t registration_update_lock; /* for some reason a mutex must always
					   * be used with a condition
					   * variable. DO NOT USE */

void *registration_thread(void *arg) {
	struct options *server_opts = arg;
	int socket_fd;
	u_int32_t i;
	int j;

	/* we build a temporary new user list from the server response here,
	 * before copying it over to the real one. */
	struct user tmp_user_list[MAX_USERS];

	/* misc structs for dealing with time and sleeping */
	int retval;
	struct timespec timeout;
	struct timeval time;

	/* Acquire the condition variable lock. Magic concurrent stuff. */
	pthread_mutex_lock(&registration_update_lock);

	/* create socket */
	if ((socket_fd = socket(AF_INET,SOCK_DGRAM,0)) < 0) {
		perror("registration");
		exit(EXIT_FAILURE);
	}

	/* zero and assign server address struct  */
	struct sockaddr_in reg_addr;
	struct sockaddr *reg_addr_p = (struct sockaddr *)&reg_addr;
	socklen_t addr_len;
	memset(&reg_addr,0,sizeof(struct sockaddr_in));
	reg_addr.sin_family = AF_INET;
	/* Port and IP address of login server  */
	reg_addr.sin_port = server_opts->server_port;
	reg_addr.sin_addr = server_opts->ip;

	/* compose and send our registration message */
	struct reg_msg message;
	size_t message_len = sizeof(struct reg_msg);
	struct reg_resp response;
	size_t response_len = sizeof(struct reg_resp);

	memset(&message,0,message_len);
	strncpy((char *)&message.password,server_opts->password,15);
	strncpy((char *)&message.username,server_opts->username,13);
	message.tcp_port = server_opts->local_port;

	/* poll server continously, every 30s */
	while (1) {
  		/* what do you know huh, porting programs does reveal bugs */
		addr_len=sizeof(struct sockaddr_in);
		memset(&tmp_user_list,0,MAX_USERS*sizeof(struct user));

		/* printf("sending registration message to server..."); */
		if (sendto(socket_fd, &message, message_len, 0,
			   reg_addr_p, addr_len) < message_len) {
			perror("failed to send message to registration server");
		}

		/* We don't know the recieved message length, so we
		 * unfortunately can't check for an incomplete read error. */
		if (recvfrom(socket_fd, &response, response_len, 0,
			     reg_addr_p, &addr_len) < 0) {
			perror("failed to recieve response from registration server");
		}
		/* printf("recieved response!\n"); */

		/* UNSAFE CONCURRENT STUFF BEGINS */
		pthread_mutex_lock(&user_list_lock);
		/* copy response info into temporary user list. Use the flags
		 * from the existing user list if they are there. */
		for (i = 0; i < ntohl(response.nusers); i++) {
			strncpy(tmp_user_list[i].name,
				response.user[i].username,
				USERNAME_LEN);
			tmp_user_list[i].port = response.user[i].tcp_port;
			tmp_user_list[i].ip = response.user[i].ip_addr;
			j = lookup_user(response.user[i].username);
			if (j >= 0) {
				/* we know this user, preserve existing
				 * connection details */
				tmp_user_list[i].flags = user_list[j].flags;
				tmp_user_list[i].socket = user_list[j].socket;
			} else {
				/* new user, initialize connection fields to
				 * zero */
				tmp_user_list[i].flags = 0;
				tmp_user_list[i].socket = 0;
			}
		}
		/* update user list */
		num_users = ntohl(response.nusers);
		memcpy(user_list,tmp_user_list,
		       num_users*sizeof(struct user));
		pthread_mutex_unlock(&user_list_lock);
		/* UNSAFE CONCURRENT STUFF ENDS */

		/* set wait time, and sleep */
		retval = gettimeofday(&time,NULL);
		timeout.tv_sec = time.tv_sec + SERVER_TIMEOUT/2;
//		timeout.tv_sec = time.tv_sec + 10;
		timeout.tv_nsec = time.tv_usec*100;
		/* Your standard 'sleep()' from unistd.h also works here. */
		retval = pthread_cond_timedwait(&registration_update,
						&registration_update_lock,
						&timeout);
	}
	return NULL;
}
