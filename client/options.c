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
#include <error.h>
#include <getopt.h>
#include "chat.h"

static struct option long_options[] = {
	{"server",1,0,'s'},
	{"port",1,0,'p'},
	{"user",1,0,'u'},
	{"password",1,0,'w'},
	{"help",0,0,'h'}
};

void set_ip(char *str,struct server_options *opts) {
	strncpy(opts->ip_string,str,INET_ADDRSTRLEN);
	if (inet_aton(opts->ip_string,&(opts->ip)) == 0) {
		error(EXIT_FAILURE,0,"invalid server IP address.");
	}
	return;
}	

void set_port(unsigned short port, struct server_options *opts) {
	opts->port_hostformat = port;
	opts->port = htons(opts->port_hostformat); /* htons IS IMPORTANT!!! */
	return;
}

void set_username(char *str,struct server_options *opts) {
	strncpy(opts->username,str,USERNAME_LEN);
	return;
}

void set_password(char *str, struct server_options *opts) {
	strncpy(opts->password,str,PASSWORD_LEN);
	return;
}

void parse_cmdline(int argc, char **argv, struct server_options *opts) {
	char option;
	unsigned short port; 	/* the only numeric option */
	/* flags to let us know when we need to set defaults */
	int got_ip       = 0;
	int got_port     = 0;
	int got_username = 0;
	int got_password = 0;
	
	while ((option = getopt_long(argc,argv,"s:p:u:w:h",long_options,NULL)) > 0) {
		switch(option) {
		case 's':
			/* server ip */
			got_ip = 1;
			set_ip(optarg,opts);
			break;
		case 'p':
			/* server port */
			got_port = 1;
			if (sscanf(optarg,"%hu",&port) < 0) {
				error(EXIT_FAILURE,0,"invalid server port.");
			}
			set_port(port,opts);
			break;
		case 'u':
			/* username */
			got_username = 1;
			set_username(optarg,opts);
			break;
		case 'w':
			/* password */
			got_password = 1;
			set_password(optarg,opts);
			break;
		case 'h':
			print_help();
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

	if (!got_port)
		set_port(DEFAULT_SERVER_PORT,opts); 

	if (!got_username)
		set_username(DEFAULT_USERNAME,opts);

	return;
}

void print_help(void) {
	printf("TODO\n");
}
