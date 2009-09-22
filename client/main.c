/*
 * chatClient.c
 *
 * Sam May 3206842
 * 22/09/09
 *
 * Client/server for a simple UDP internet chat protocol. A TELE3118
 * mini-project.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <error.h>
#include <unistd.h>
#include <pthread.h>
#include "chatClient.h"

int main (int argc, char **argv) {

	/* get server connection details from command-line */
	struct server_options server_opts;
	bzero(&server_opts,sizeof(struct server_options));
	parse_cmdline(argc,argv,&server_opts);


	/* create a background thread to stay in touch with the server */
	pthread_t server_polling_thread;
	pthread_create(&server_polling_thread,
		       NULL,
		       poll_server,
		       &server_opts);

	/* loop on the input prompt, waiting for commands */
	char input[INPUT_BUF_SIZE];
	while (1) {
		printf("> ");
		fgets(input,INPUT_BUF_SIZE,stdin);
		if (strncmp(input,"list\n",INPUT_BUF_SIZE) == 0) {
			print_user_list();
		} else if (strncmp(input,"quit\n",INPUT_BUF_SIZE) == 0) {
			break;
		} else {
			printf("you entered: '%s'\n",input);
		}
	} 
	printf("Bye.\n");
	return 0;
}	

