#ifndef _COMMANDS_H_
#define _COMMANDS_H_

#include "user.h"

int execute_command(char input[INPUT_LEN]);

void block(char user[USERNAME_LEN]);
void unblock(char user[USERNAME_LEN]);

void help(void);

#endif /* _COMMANDS_H_ */
