#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <readline/readline.h>

#define TRUE 1
#define FALSE 0
#define BLOCK_NUMBER 25600
#define DISK_BYTES 104857600
#define DISK_HEX_VALUES 52428800
#define BITMAP_BYTES 3200
#define BLOCK_SIZE 4096
#define ROOT_ADDR 57344 
#define DIR_INDICATOR 32768
#define LAST(k,n) ((k) & ((1<<(n))-1))
#define MID(k,m,n) LAST((k)>>(m),((n)-(m)))

char *promptUser();
int handleCommand(char *command);

