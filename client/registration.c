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
#include <errno.h>
#include <unistd.h>
#include <pthread.h>

#include "user.h"
#include "options.h"
#include "registration.h"

/** global variables **
 **********************/
pthread_mutex_t registration_update_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t registration_update = PTHREAD_COND_INITIALIZER;
pthread_mutex_t registration_update_lock; /* for some reason a mutex must always be
				     * used with a condition variable. DO NOT
				     * USE */

void *registration_thread(void *arg) {
	struct options *server_opts = arg; 
	int socket_fd;
	unsigned int i;
	int j;

	/* we build a new user list from the server response here, before
	 * copying it over to the real one. */
	struct user tmp_user_list[MAX_USERS];

	/* structs for dealing with time and sleeping */
	int retval;
	struct timespec timeout;
	struct timeval time;

	/* acquire condition variable lock */
	pthread_mutex_lock(&registration_update_lock);

	/* create socket */
	if ((socket_fd = socket(AF_INET,SOCK_DGRAM,0)) < 0) {
		perror("registration");
		exit(EXIT_FAILURE);
	}

	/* zero and assign server address struct  */
	struct sockaddr_in server_addr;
	struct sockaddr *server_addr_p = (struct sockaddr *)&server_addr;
	bzero(&server_addr,sizeof(struct sockaddr_in));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = server_opts->server_port;
	server_addr.sin_addr = server_opts->ip;


	/* compose and send our registration message */
	struct reg_msg message;
	struct reg_resp response;
	socklen_t response_len;  

	bzero(&message,sizeof(struct reg_msg));
	strncpy((char *)&message.password,server_opts->password,15);
	strncpy((char *)&message.username,server_opts->username,13);
	message.tcp_port = server_opts->local_port;

	/* poll server continously, every 30s */
	while (1) {
		/* printf("sending registration message to server..."); */
		if (sendto(socket_fd, &message, sizeof(struct reg_msg), 0,
			   server_addr_p,sizeof(struct sockaddr_in)) < 0) {
			perror("failed to send message to registration server");
		}

		if (recvfrom(socket_fd, &response, sizeof(struct reg_resp), 0,
			     server_addr_p, &response_len) < 0) {
			perror("failed to recieve response from registration server");
		}
		/* printf("recieved response!\n"); */

		/* UNSAFE CONCURRENT STUFF BEGINS */
		pthread_mutex_lock(&user_list_lock);
		/* copy response info into temporary user list. Use the flags
		 * from the existing user list if they are there.
		 *
		 * IMPORTANT: Assume the server returns user information in
		 * alphabetical/sorted order. I know the current server
		 * implementation does this. CHECK THIS ASSUMPTION otherwise the
		 * binary search used in lookup_user() will not work.
		 */
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
		retval = pthread_cond_timedwait(&registration_update,
						&registration_update_lock,
						&timeout);
	}
	return NULL;
}
