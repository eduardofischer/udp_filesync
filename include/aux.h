#ifndef __aux_functions__
#define __aux_functions__

#include <time.h>

void delay(int secs);

char **cmd_completion(const char *, int, int);
char *cmd_generator(const char *, int);

#endif
