#SHELL = /bin/sh
# decide on compiler and flags depending on host

CC = gcc
CFLAGS =
ALL_CFLAGS = -g $(CFLAGS)

OBJS = skiplist.o chatRegServ.o
LIBS = 
EXE = chatRegServ

# pattern rule to compile object files from C files
%.o : %.c Makefile
	$(CC) $(ALL_CFLAGS) -c $< -o $@

all: $(EXE)

$(EXE): $(OBJS) Makefile
	$(CC) $(ALL_CFLAGS) $(OBJS) -o $(EXE) $(LIBS)

.PHONY : clean
clean:
	rm -f $(OBJS) $(EXE)
