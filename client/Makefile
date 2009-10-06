DEBUG = -g
LIBS = -pthread
CFLAGS = -Wall -Werror $(DEBUG)
SRC = main.c options.c registration.c connection.c user.c commands.c
HEADERS = options.h registration.h connection.h user.h console.h commands.h
OBJ = $(SRC:.c=.o)
BIN = mchat

.PHONY: clean clobber 

%o:%c
	gcc $(CFLAGS) -c $<

main: $(OBJ) $(HEADERS)
	gcc $(CFLAGS) $(LIBS) -o $(BIN) $(OBJ)

user.c: user.h console.h
options.c: options.h
registraction.c: user.h options.h registration.h
connection.c: user.h options.h console.h connection.h
commands.c: user.h console.h commands.h connection.h registration.h

clean:
	rm -f $(OBJ)

clobber: clean
	rm -f $(BIN)
