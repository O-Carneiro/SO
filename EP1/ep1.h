#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sched.h>
#include <time.h>
#include <unistd.h>

#define sizeofProcess sizeof(char)*32+sizeof(int)+sizeof(double)*3 + sizeof(mutexNode *)
#define BLOCKED 0
#define RUNNING 1
#define READY 2
#define RAN 3
#define CPU_CORE_ID 0
#define QUANTUM 2
#define epsilon 0.0000001

typedef struct mutexNode {
    pthread_mutex_t *mutex;
    struct mutexNode *prev;
    struct mutexNode *next;
} mutexNode;

typedef struct processStruct{
    char name[32];
    double deadline;
    double t0;
    double dt;
    int status;
    mutexNode *pMutexNode;
} process;

typedef struct processArray{
    process **Arr;
    int readHeader;
    int count;
    int size;
} processArray;

typedef struct pthreadArgs {
    void *p;
    int id;
}pthreadArgs;

