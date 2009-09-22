/*
 * client.c
 *
 * Sam May
 * 22/09/09
 *
 * This file contains the code for connecting to another chat user via TCP and
 * exchanging messages.
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
#include <pthread.h>
#include "chat.h"

