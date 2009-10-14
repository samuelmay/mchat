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
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "user.h"
#include "console.h"
#include "commands.h"
#include "connection.h"
#include "registration.h"

int execute_command(char input[INPUT_LEN]) {
	char arg1[INPUT_LEN];
	char arg2[INPUT_LEN];
	int tmp;
	if (strncmp(input,"\n",INPUT_LEN) == 0) {
		;		/* empty */
	} else if (strncmp(input,"msg ",4) == 0) {
		strncpy(arg1,&(input[4]),INPUT_LEN-4);
		broadcast_message(arg1);
	} else if (strncmp(input,"msg/",4) == 0 &&
		   sscanf(input, "msg/" USERNAME_SCAN_FMT " %n",
			  arg1,&tmp) >= 1) {
		/* the '%n' directive in scanf returns the number of
		 * characters that have been processed so far. It's
		 * undefined how it affects the return value, so check
		 * for >= 1. */
		strncpy(arg2,&(input[tmp]),INPUT_LEN-tmp);
		send_message(arg1,arg2);
	} else if (strncmp(input,"list\n",5) == 0) {
		print_user_list();
	} else if (strncmp(input,"update\n",7) == 0) {
		pthread_cond_signal(&registration_update);
	} else if (strncmp(input,"connect ",8) == 0 &&
		   sscanf(input,"connect " USERNAME_SCAN_FMT "\n",
			  arg1) == 1) {
		/* Remember to set the specifier length for the above
		 * scanf to USERNAME_LEN.
		 *
		 * Initialize a TCP connection with a given user. */
		connect_user(arg1);
	} else if (strncmp(input,"disconnect ",11) == 0 &&
		   sscanf(input,"disconnect " USERNAME_SCAN_FMT "\n",
			  arg1) == 1) {
		disconnect_user(arg1);
	} else if (strncmp(input,"block ",6) == 0 &&
		   sscanf(input,"block " USERNAME_SCAN_FMT "\n",
			  arg1) == 1) {
		block(arg1);
	} else if (strncmp(input,"unblock ",8) == 0 &&
		   sscanf(input,"unblock " USERNAME_SCAN_FMT "\n",
			  arg1) == 1) {
		unblock(arg1);
	} else if (strncmp(input,"help",4) == 0) {
		help();
	} else if (strncmp(input,"quit\n",5) == 0 ||
		   strncmp(input,"bye\n",4) == 0 ||
		   strlen(input) == 0) {
		/* if the length of the input is 0, someone hit
		 * Ctrl-D */
		return 1;
	} else {
		/* the 'ed' school of error reporting. */
		printf("Unknown or invalid command.\n");
	}
	printf("> ");
	fflush(stdout);
	return 0;
}

void block(char user[USERNAME_LEN]) {
	int i;
	/* BEGIN UNSAFE CONCURRENT STUFF */
	pthread_mutex_lock(&user_list_lock);
	i = lookup_user(user);
	if (i < 0) {
		printf("that user's not logged in!\n");
	} else if (user_list[i].flags & USER_BLOCKED) {
		printf("that user's already blocked!\n");
	} else {
		user_list[i].flags |= USER_BLOCKED;
                /* disconnect them as well */
		if (user_list[i].flags & USER_CONNECTED) {
			if (close(user_list[i].socket) < 0) {
				perror("disconnect user");
			}
			user_list[i].flags &= ~USER_CONNECTED;
			user_list[i].socket = 0;
		}
		printf(USERNAME_PRINT_FMT " was blocked.\n", user);
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
		printf("that user's not logged in!\n");
	} else if (!(user_list[i].flags & USER_BLOCKED)) {
		printf("that user's not blocked!\n");
	} else {
		user_list[i].flags &= ~USER_BLOCKED;
		printf(USERNAME_PRINT_FMT " was unblocked.\n",user);
	}
	pthread_mutex_unlock(&user_list_lock);
	/* END UNSAFE CONCURRENT STUFF */
	return;
}

void help(void) {
	printf(
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
