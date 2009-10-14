#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#define INPUT_LEN 256
#define INPUT_SCAN_FMT "%256s"
#define INPUT_PRINT_FMT "%.256s"

/* 'chomp' string. If the last character is a newline, remove
 * it. Defined in main.c */
void chomp(char *string,size_t maxlen);

#endif /* _CONSOLE_H_ */
