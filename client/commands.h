#ifndef _COMMANDS_H_
#define _COMMANDS_H_

#include "user.h"

void block(char user[USERNAME_LEN]);
void unblock(char user[USERNAME_LEN]);

void disconnect(char user[USERNAME_LEN]);

void help(void);

#endif /* _COMMANDS_H_ */
