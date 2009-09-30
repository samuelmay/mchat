/*
 * user.c
 *
 * Sam May
 * 29/09/09
 *
 * User list and functions for manipulating it.
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
#include <pthread.h>
#include "chat.h"

struct user user_list[50];
int num_users;
pthread_mutex_t user_list_lock = PTHREAD_MUTEX_INITIALIZER;

void print_user_list(void) {
	unsigned int i;
	char ip[INET_ADDRSTRLEN];
	char *username;
	unsigned short port;

        /* UNSAFE CONCURRENT STUFF BEGINS */	
	pthread_mutex_lock(&user_list_lock); /* NUMBER 1 */
	pthread_mutex_lock(&console_lock);   /* NUMBER 2 */
	printf("%14s %15s %6s\n","USERNAME","IP ADDRESS","PORT");
	for (i = 0; i < num_users && i < 50; i++) {
		username = (char *)&(user_list[i].name);
		port = ntohs(user_list[i].port);
		inet_ntop(AF_INET,&(user_list[i].ip),ip,INET_ADDRSTRLEN);
		printf("%14s %15s %6d\n",username, ip, port);
	}
	pthread_mutex_unlock(&console_lock);   /* NUMBER 2 */
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
	for (i = 0; i < num_users && i < 50; i++) {
		if (user_list[i].ip == connection->sin_addr.s_addr) {
			found = 1;
			strncpy(user,
				user_list[i].name,USERNAME_LEN);
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
	for (i = 0; i < num_users && i < 50; i++) {
		if (strncmp(user_list[i].name,
			    user,
			    USERNAME_LEN) == 0) {
			found = 1;
			connection->sin_family = AF_INET;
			connection->sin_port = user_list[i].port;
			connection->sin_addr.s_addr = 
				user_list[i].ip;
			break;
		}
	}
	pthread_mutex_unlock(&user_list_lock);
	/* UNSAFE CONCURRENT STUFF ENDS */
	return found;
}
