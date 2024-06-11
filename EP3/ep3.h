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
#define ROOT_ADDR 53248
#define ROOT_BLOCK 13
#define DIR_INDICATOR 32768
#define LAST_BLOCK 32768
#define LAST(k,n) ((k) & ((1<<(n))-1))
#define DIR 1
#define FIL 0
#define ENTRY_SIZE 256
#define ALLOC true
#define NO_ALLOC false
#define STOP_BEFORE true
#define STOP_AT false
#define INITIAL_FREE_SPACE 104800256 // 25600 - 14 blocos livres
#define INITIAL_WASTED_SPACE 6144
//espaço desperdiçado no fim da FAT(que tem os metadados no fim) e pelo ROOT vazio

typedef struct Entry {
    uint16_t blockPtr; 
    uint32_t size;
    uint32_t createTime;
    uint32_t modTime;
    uint32_t accessTime;
    char name[238];
} Entry;

typedef struct dbEntry {
    char name[238];
    char path[1024];
} dbEntry;

