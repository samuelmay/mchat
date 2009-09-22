DEBUG = -g
LIBS = -pthread
CFLAGS = -Wall -Werror $(DEBUG)
SRC = main.c registration.c options.c
OBJ = $(SRC:.c=.o)
BIN = chatClient

.PHONY: clean clobber 

%o:%c
	gcc $(CFLAGS) -c $<

main: $(OBJ)
	gcc $(CFLAGS) $(LIBS) -o $(BIN) $(OBJ)

clean:
	rm -f $(OBJ)

clobber: clean
	rm -f $(BIN)
