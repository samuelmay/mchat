/*
 * options.c
 *
 * Sam May
 * 22/09/09
 *
 * Command-line options parsing for chat client
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
#include <getopt.h>

#include "options.h"

static struct option long_options[] = {
	{"server",1,0,'s'},
	{"port",1,0,'o'},
	{"user",1,0,'u'},
	{"password",1,0,'p'},
	{"help",0,0,'h'}
};

void set_ip(char *str,struct options *opts) {
	strncpy(opts->ip_string,str,INET_ADDRSTRLEN);
	if (inet_aton(opts->ip_string,&(opts->ip)) == 0) {
		fprintf(stderr,"invalid server IP address.\n"); 
		exit(EXIT_FAILURE);
	}
	return;
}	

void set_server_port(unsigned short port, struct options *opts) {
	opts->server_port_h = port;
	opts->server_port = htons(opts->server_port_h);
	return;
}

void set_local_port(unsigned short port, struct options *opts) {
	opts->local_port_h = port;
	opts->local_port = htons(opts->local_port_h); /* htons IS IMPORTANT!!! */
	return;
}

void set_username(char *str,struct options *opts) {
	strncpy(opts->username,str,USERNAME_LEN);
	return;
}

void set_password(char *str, struct options *opts) {
	strncpy(opts->password,str,PASSWORD_LEN);
	return;
}

void parse_cmdline(int argc, char **argv, struct options *opts) {
	char option;
	unsigned short port; 	/* the only numeric option */
	/* flags to let us know when we need to set defaults */
	int got_ip       = 0;
	int got_port     = 0;
	int got_username = 0;
	int got_password = 0;
	
	while ((option = getopt_long(argc,argv,"s:o:u:p:h",long_options,NULL)) > 0) {
		switch(option) {
		case 's':
			/* server ip */
			got_ip = 1;
			set_ip(optarg,opts);
			break;
		case 'o':
			/* server port */
			got_port = 1;
			if (sscanf(optarg,"%hu",&port) < 0) {
				fprintf(stderr,"invalid server port.\n");
				exit(EXIT_FAILURE);
			}
			set_local_port(port,opts);
			break;
		case 'u':
			/* username */
			got_username = 1;
			set_username(optarg,opts);
			break;
		case 'p':
			/* password */
			got_password = 1;
			set_password(optarg,opts);
			break;
		case 'h':
			print_options_help();
			exit(EXIT_SUCCESS);
			break;
		default:
			exit(EXIT_FAILURE);
			break;
		}
	}
	
	/* set default if an option wasn't specified */
	if (!got_ip)
		set_ip(DEFAULT_SERVER_IP,opts); 

	/* no option for this at the moment */
	set_server_port(DEFAULT_SERVER_PORT,opts);

	/* setting the local port to 0 will give us a random free port. */
	if (!got_port)
		set_local_port(0,opts); 

	if (!got_username)
		set_username(DEFAULT_USERNAME,opts);

	if (!got_password)
		set_password(DEFAULT_PASSWORD,opts);

	return;
}

void print_options_help(void) {
	printf("mchat v0.1, copyright sam may 2009.\n"
	       "A simple internat chat program.\n\n"
	       "Options:\n"
	       "  -s --server        IP of the chat server to connect to\n"
	       "  -o --port          Specify TCP port to listen for incoming connections on.\n"
	       "  -p --password      Server password\n"
	       "  -u --user          Username to register with the server (default 'guest')\n"
	       "  -h --help          Print this help\n\n"
	       "Example: mchat -s 149.171.92.193 -o 31181 -u alice -p mypassword\n"
	       "Once in the program, enter 'help' to get a list of commands.\n");
	return;
}
