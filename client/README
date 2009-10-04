Threads
-------

* Main thread: Console loop. Dispatches commands, as detailed in 'COMMANDS'
  section.
  FILES: main.c message.c, client.c
  CONTENDS: user list, server options, connection list

* Registration thread: simply throws a UDP packet at the registration server
  every 30 seconds, updates the user list with the response, and goes back to
  sleep.
  FILES: registration.c
  CONTENDS: user list, server options.

* Incoming connection server: uses select() to wait for a readable socket, and
  either accepts the connection and notifies user, or receives message and
  writes to the console.
  FILES: server.c
  CONTENDS: console, connection list

Resources
---------

(Globally visible, require mutex/locks to access)

User list: Main list of users. Flat array in the exact binary form of a
registration response. % TODO use actual data structure

Connection list: linked list of open socket file descriptors, with the remote
names. % TODO see if I can combine with user list

Output console: stdout. We want all printing to the screen to be in coherent
blocks. (% TODO possibly lock when reading stdin as well?)

Startup
-------

* parse command line as per 'options.c', putting results in options struct.

* start server thread:
  - create and bind listening socket.
  % TODO automatic port selection: bind to port 0, get actual port number, and
    update options struct.
  - create poll group, add listening socket
  - start thread

* start registration thread

* start command loop.

Commands
--------

* list: print out the user list
  RESOURCES: user list, console.
  PROCEDURE: Loop through user list and print to console.

* connect <username>: initiate connection with someone on the user list
  RESOURCES: user list, connection list
  PROCEDURE: Check if that's a logged-on user; Check if we don't already have a
  connection; if not, create connection and add to connection list; add user to
  poll set.

* disconnect <username>: disconnect with someone
  RESOURCES: connection list
  PROCEDURE: Check if we have a connection, if so, close connection, remove from
  connection list, and remove from poll set.

* msg: send message to all connected users
  RESOURCES: connection list
  PROCEDURE: send buffer to each open socket in turn.

* quit/bye: exit
  RESOURCES: everything and nothing.
  PROCEDURE: kill all threads, close all open sockets.

%% TODOs
* msg <username>: send a message to just one user
* block <username>: prevent user from connecting with you
