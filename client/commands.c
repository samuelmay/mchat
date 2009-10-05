/*
 * commands.c
 *
 * Sam May
 * 02/10/09
 *
 * Functions implementing miscellaneous chat commands.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#include "user.h"
#include "console.h"
#include "commands.h"

void block(char user[USERNAME_LEN]) {
	int i;
	/* BEGIN UNSAFE CONCURRENT STUFF */
	pthread_mutex_lock(&user_list_lock);
	i = lookup_user(user);
	if (i < 0) {
		printf_threadsafe("that user's not logged in!\n\n");
	} else if (user_list[i].flags & USER_BLOCKED) {
		printf_threadsafe("that user's already blocked!\n\n");
	} else {
                /* this also clears the 'connected' flag and thus disconnects
		 * them as well */
		user_list[i].flags &= ~USER_CONNECTED;
		user_list[i].flags |= USER_BLOCKED;
		printf_threadsafe("%s was blocked.\n\n",user);
	}
	pthread_mutex_unlock(&user_list_lock);
	/* END UNSAFE CONCURRENT STUFF */
	return;
}

void unblock(char user[USERNAME_LEN]) {
	int i;
	/* BEGIN UNSAFE CONCURRENT STUFF */
	pthread_mutex_lock(&user_list_lock);
	i = lookup_user(user);
	if (i < 0) {
		printf_threadsafe("that user's not logged in!\n\n");
	} else if (!(user_list[i].flags & USER_BLOCKED)) {
		printf_threadsafe("that user's not blocked!\n\n");
	} else {
		user_list[i].flags &= ~USER_BLOCKED;
		printf_threadsafe("%s was unblocked.\n\n",user);
	} 
	pthread_mutex_unlock(&user_list_lock);
	/* END UNSAFE CONCURRENT STUFF */
	return;
}

void disconnect(char user[USERNAME_LEN]) {
	int i;
	/* BEGIN UNSAFE CONCURRENT STUFF */
	pthread_mutex_lock(&user_list_lock);
	i = lookup_user(user);
	if (i < 0) {
		printf_threadsafe("that user's not logged in!\n\n");
	} else if (!(user_list[i].flags & USER_CONNECTED)) {
		printf_threadsafe("you're not connected to that user!\n\n");
	} else {
		user_list[i].flags &= ~USER_CONNECTED;
		printf_threadsafe("%s was disconnected.\n\n",user);
	} 	
	pthread_mutex_unlock(&user_list_lock);
	/* END UNSAFE CONCURRENT STUFF */
	return;
}

void help(void) {
	printf_threadsafe(
"AVAILABLE COMMANDS\n"
"help                 print this help\n"
"\n"
"list                 list logged-in users\n"
"\n"
"update               force update of logged-in users list from server\n"
"\n"
"connect <user>       connect to a logged-in user, so you can send and recieve\n"
"                     messages with them. Unless they are blocked, incoming\n"
"                     connection attempts from other users on the user list are\n"
"                     always accepted.\n"
"\n"
"disconnect <user>    disconnect from a user\n"
"\n"
"block <user>         prevent a user from connecting to you\n"
"\n"
"unblock <user>       re-allow a blocked user to connect to you\n"
"\n"
"msg <text>           broadcast a message to all users you are connected to\n"
"\n"
"msg/<user> <text>    send a message to only one user\n"
"\n"
"quit                 exit mchat\n"
		);
	return;
}
