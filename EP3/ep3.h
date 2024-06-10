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
#define ENTRY_SIZE 256
#define ALLOC true
#define NO_ALLOC false

    
typedef struct Entry {
    uint16_t blockPtr; 
    uint32_t size;
    uint32_t createTime;
    uint32_t modTime;
    uint32_t accessTime;
    char name[238];
} Entry;

char *promptUser();
int handleCommand(char *command);

