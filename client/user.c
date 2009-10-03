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

#include "user.h"
#include "console.h"

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
		printf("%14s %15s %6d",username, ip, port);
		if (user_list[i].flags & USER_CONNECTED) {
			printf(" (connected)");
		}
		if (user_list[i].flags & USER_BLOCKED) {
			printf(" (blocked)");
		}
		printf("\n");
	}
	pthread_mutex_unlock(&console_lock);   /* NUMBER 2 */
	pthread_mutex_unlock(&user_list_lock); /* NUMBER 1 */
	/* UNSAFE CONCURRENT STUFF ENDS */
	
	return;
}

/* looks up user list entry for the given username details. Returns the index in
 * the user list, or -1 if not found. MUST HOLD USER LIST LOCK. */
int lookup_user(char name[USERNAME_LEN]) {
	int i;
	for (i = 0; i < num_users && i < 50; i++) {
		if (strncmp(user_list[i].name,name,USERNAME_LEN) == 0) {
			return i;
		}
	}
	return -1;
}

int lookup_socket(int fd) {
	int i;
	for (i = 0; i < num_users && i < 50; i++) {
		if (user_list[i].socket == fd) {
			return i;
		}
	}
	return -1;
}
