#ifndef _CONNECTION_H_
#define _CONNECTION_H_

#include "user.h"
#include "console.h"

/* The main TCP networking stuff. */

#define MESSAGE_LEN 8		/* length of an application message */

/* server.c */
int start_listening(struct options *opts);
void *server_thread(void *arg);
void accept_new_connection(int fd);
void receive_message(int fd);

/* client.c */
void connect_user(char remote_user[USERNAME_LEN]);
void broadcast_message(char message[INPUT_LEN]);

#endif /* _CONNECTION_H_ */
