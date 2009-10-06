#ifndef _CONNECTION_H_
#define _CONNECTION_H_

#include "user.h"
#include "options.h"
#include "console.h"

/* The main TCP networking stuff. */

#define MESSAGE_LEN 8		/* length of an application message */

int start_listening(struct options *opts);

void accept_new_connection(int fd);

void connect_user(char remote_user[USERNAME_LEN]);
void disconnect_user(char remote_user[USERNAME_LEN]);

void receive_message(int fd);
void broadcast_message(char message[INPUT_LEN]);
void send_message(char remote_user[USERNAME_LEN],char message[INPUT_LEN]);

#endif /* _CONNECTION_H_ */
