#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/utsname.h>
#include <pwd.h>
#include <time.h>
#include <readline/history.h>
#include <readline/readline.h>

#define TRUE 1
#define FALSE 0

char *promptUser(char *username);
int handleCommand(char *command);

