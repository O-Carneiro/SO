#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <math.h>
#include <unistd.h>

typedef struct Ciclista {
    int id;
    int voltas;
    int pos;
    int velocidade;
    int faixa;
    int vencedor;
    int quebrou;
}Ciclista;

typedef struct pthreadArgs {
    void *p;
}pthreadArgs;
