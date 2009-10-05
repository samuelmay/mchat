#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#define INPUT_LEN 256
#define INPUT_SCAN_FMT "%256s"
#define INPUT_PRINT_FMT "%.256s"

/* LOCKS */
/*********/
/* To prevent deadlock, each resource is assigned a lock and a number. Resource
 * locks MUST be aquired in ascending numerical order, and released in
 * descending numerical order, on pain of the filthiest debugging you will ever
 * have to do. */

/* Needs to be held if printing to stdout. We won't worry about stdin, because
 * only the main thread uses that, and if we're printing to stderr we have more
 * pressing issues. */
extern pthread_mutex_t console_lock; /* NUMBER 2 */

/* this is unfortunately GCC specific. This should work if you're holding
 * another lock, AS LONG AS THE CONSOLE IS THE HIGHEST NUMBERED RESOURCE. */
#define printf_threadsafe(format,...) do {		\
		pthread_mutex_lock(&console_lock);	\
		printf(format, ## __VA_ARGS__);		\
		fflush(stdout);				\
		pthread_mutex_unlock(&console_lock);	\
	} while (0)


#endif /* _CONSOLE_H_ */
