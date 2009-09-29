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
#include <error.h>
#include <unistd.h>
#include <pthread.h>
#include "chat.h"

/** global variables **
 **********************/
struct reg_resp user_list;
pthread_mutex_t user_list_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t server_update_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t server_update = PTHREAD_COND_INITIALIZER;
pthread_mutex_t server_update_lock; /* for some reason a mutex must always be
				     * used with a condition variable. DO NOT
				     * USE */

void print_user_list(void) {
	unsigned int i;
	char ip[INET_ADDRSTRLEN];
	char *username;
	unsigned long nusers;
	unsigned short port;

        /* UNSAFE CONCURRENT STUFF BEGINS */	
	pthread_mutex_lock(&user_list_lock); /* NUMBER 1 */
//	pthread_mutex_lock(&console_lock);   /* NUMBER 2 */
	nusers = ntohl(user_list.nusers);
	printf("%14s %15s %6s\n","USERNAME","IP ADDRESS","PORT");
	for (i = 0; i < nusers && i < 50; i++) {
		username = (char *)&(user_list.user[i].username);
		port = ntohs(user_list.user[i].tcp_port);
		inet_ntop(AF_INET,&(user_list.user[i].ip_addr),ip,INET_ADDRSTRLEN);
		printf("%14s %15s %6d\n",username, ip, port);
	}
//	pthread_mutex_unlock(&console_lock);   /* NUMBER 2 */
	pthread_mutex_unlock(&user_list_lock); /* NUMBER 1 */
	/* UNSAFE CONCURRENT STUFF ENDS */
	
	return;
}

/* looks up user with the given connection details. Returns 1 if found, 0 if not
 * found. Does not work properly for clients on the same host. */
int lookup_user(struct sockaddr_in *connection,
		char user[USERNAME_LEN]) {
	int i;
	int found;
	/* UNSAFE CONCURRENT STUFF BEGINS */	
	pthread_mutex_lock(&user_list_lock);
	for (i = 0; i < ntohl(user_list.nusers) && i < 50; i++) {
		if (user_list.user[i].ip_addr == connection->sin_addr.s_addr) {
			found = 1;
			strncpy(user,
				user_list.user[i].username,USERNAME_LEN);
			break;
		}
	}
	pthread_mutex_unlock(&user_list_lock);
	/* UNSAFE CONCURRENT STUFF ENDS */
	return found;
}

/* looks up connection details for the given user. Returns 1 if found, 0 if not
 * found. */
int lookup_connection(struct sockaddr_in *connection,
		      char user[USERNAME_LEN]) {
	int i;
	int found = 0;
        /* UNSAFE CONCURRENT STUFF BEGINS */	
	pthread_mutex_lock(&user_list_lock);
	for (i = 0; i < ntohl(user_list.nusers) && i < 50; i++) {
		if (strncmp(user_list.user[i].username,
			    user,
			    USERNAME_LEN) == 0) {
			found = 1;
			connection->sin_family = AF_INET;
			connection->sin_port = user_list.user[i].tcp_port;
			connection->sin_addr.s_addr = 
				user_list.user[i].ip_addr;
			break;
		}
	}
	pthread_mutex_unlock(&user_list_lock);
	/* UNSAFE CONCURRENT STUFF ENDS */
	return found;
}

void *poll_server(void *arg) {
	struct server_options *server_opts = arg; 
	int socket_fd;

	/* structs for dealing with time and sleeping */
	int retval;
	struct timespec timeout;
	struct timeval time;

	/* acquire condition variable lock */
	pthread_mutex_lock(&server_update_lock);

	/* create socket */
	if ((socket_fd = socket(AF_INET,SOCK_DGRAM,0)) < 0) {
		error(EXIT_FAILURE,errno,"socket creation failed");
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
			error(EXIT_FAILURE,errno,"failed to send registration message to server");
		}

		if (recvfrom(socket_fd, &response, sizeof(struct reg_resp), 0,
			     server_addr_p, &response_len) < 0) {
			error(EXIT_FAILURE,errno,"failed to recieve response from server");
		}
		/* printf("recieved response!\n"); */

		/* UNSAFE CONCURRENT STUFF BEGINS */
		pthread_mutex_lock(&user_list_lock);
		memcpy(&user_list,&response,sizeof(struct reg_resp));
		pthread_mutex_unlock(&user_list_lock);
		/* UNSAFE CONCURRENT STUFF ENDS */

		/* set wait time, and sleep */
		retval = gettimeofday(&time,NULL);
		timeout.tv_sec = time.tv_sec + SERVER_TIMEOUT/2;
//		timeout.tv_sec = time.tv_sec + 10; 
		timeout.tv_nsec = time.tv_usec*100; 
		retval = pthread_cond_timedwait(&server_update,
						&server_update_lock,
						&timeout);
	}
	return NULL;
}
