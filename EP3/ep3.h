#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <readline/readline.h>

#define TRUE 1
#define FALSE 0
#define BLOCK_NUMBER 25600
#define BLOCK_SIZE 4096
#define DISK_BYTES 104857600
#define DISK_HEX_VALUES 52428800
#define BITMAP_BYTES 3200
#define ROOT_ADDR 57344 
#define ROOT_BLOCK 14 
#define DIR_INDICATOR 32768
#define LAST_BLOCK 32768
#define LAST(k,n) ((k) & ((1<<(n))-1))
#define DIR 1
#define FIL 0
#define DIR_ATTR_SIZE 12
#define FIL_ATTR_SIZE 14

char *promptUser();
int handleCommand(char *command);

