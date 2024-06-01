#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <readline/readline.h>

#define TRUE 1
#define FALSE 0
#define BYTE 8 //8 * 1 bit
#define KB 8192 //1024 * 1 Byte
#define MB 8388608 //1024 * 1 KByte
#define MODE_DIR 0
#define MODE_FILE 1

typedef struct {
    char name[255];
    struct tm *created;
    struct tm *lastModified;
    struct tm *lastAccessed;
} metadata;

typedef struct {
    int startBlock;
    int bytes;
    metadata meta;
} File;

union Entry;
typedef struct {
    union Entry **children;
    int size;
    int count;
    metadata meta;
} Dir;

typedef union Entry{
    File *file;
    Dir *dir;
}Entry;

char *promptUser();
int handleCommand(char *command);

