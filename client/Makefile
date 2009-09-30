DEBUG = -g
LIBS = -pthread
CFLAGS = -Wall -Werror $(DEBUG)
SRC = main.c options.c registration.c client.c server.c user.c
HEADERS = options.h registration.h connection.h user.h console.h
OBJ = $(SRC:.c=.o)
BIN = chat

.PHONY: clean clobber 

%o:%c
	gcc $(CFLAGS) -c $<

main: $(OBJ) $(HEADERS)
	gcc $(CFLAGS) $(LIBS) -o $(BIN) $(OBJ)

user.c: user.h console.h
options.c: options.h
registraction.c: user.h options.h registration.h
server.c: user.h options.h console.h connection.h
client.c: user.h options.h console.h connection.h

clean:
	rm -f $(OBJ)

clobber: clean
	rm -f $(BIN)
