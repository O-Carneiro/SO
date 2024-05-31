#include "ep2.h"
#include <pthread.h>

int *pista[10];
int *podio;
int *idQuebrados;
int *voltaQuebrados;
int podioPos = 0;
int winVoltas = 2;
int maxVoltas = -1;
int quebrados = 0;
int d;
int nCiclistas; // quantos ciclistas tem na pista
double raceClock = 0;
double *lastCrossTime;
Ciclista *ciclistas;
Ciclista *ciclistasShuffle;
pthread_t *tids;
pthread_barrier_t barreiraAndaram, barreiraPrint;
pthread_mutex_t nCiclistasMut, pistaMut, maxVoltasMut;

int sorteio(int probabilidade){
    int r =(rand()%100)+1;
    return (r <= probabilidade);
}

void swap(Ciclista *a, Ciclista *b)
{
    Ciclista temp = *a;
    *a = *b;
    *b = temp;
}

void shuffleCiclistas(int k)
{
    for(int i = 0; i < k; i ++){
        ciclistasShuffle[i] = ciclistas[i];
    }
    srand ( time(NULL) );
    for (int i = k-1; i > 0; i--)
    {
        int j = rand() % (i+1);
        swap(&ciclistasShuffle[i], &ciclistasShuffle[j]);
    }
}

void printaPista(){
    for(int i = 9; i >= 0; i--){
        for(int j= d-1; j >= 0; j--){
            if(pista[i][j] > 0)
                printf("%4d",pista[i][j]);
            else
                printf("%4s",".");
        }
        printf("\n");
    }
}

void inicializaPista(){
    for(int i = 0; i < 10; i++){
        for(int j= 0; j < d; j++){
            pista[i][j] = -1;
        }
    }
}

void posicionaLargada(int k){
    int index = 0;
    for(int i = 0; i < ceil((double)k/5);i++){
        int pos = (d - i)% d;
        for(int j = 0; j < 5 && index < k; j++){
            ciclistas[ciclistasShuffle[index].id - 1].faixa = j;
            ciclistas[ciclistasShuffle[index].id - 1].pos = pos; 
            pista[j][pos] = ciclistasShuffle[index].id;
            index++;
        }
    }

}

void ciclistaFora(Ciclista *ciclistaP){
    while(nCiclistas > 0){
        pthread_mutex_unlock(&nCiclistasMut);
        pthread_barrier_wait(&barreiraAndaram);
        pthread_barrier_wait(&barreiraPrint);
        pthread_mutex_lock(&nCiclistasMut);
    } 
    pthread_mutex_unlock(&nCiclistasMut);
    return;
}

void ultimoCiclistaHandler(Ciclista *ciclistaP, int pos){
    Ciclista ciclista = *ciclistaP;
    pthread_barrier_wait(&barreiraAndaram);

    pthread_mutex_lock(&nCiclistasMut);
    pthread_mutex_lock(&pistaMut);
    pista[ciclista.faixa][pos] = -1;
    nCiclistas--;
    pthread_mutex_unlock(&pistaMut);
    pthread_mutex_unlock(&nCiclistasMut);

    pthread_barrier_wait(&barreiraPrint);
}

int achaFaixaLivre(int pos,int faixa){
    for(int i = 0; i < 10; i++){
        if(pista[i][pos] < 0)
            return i;
    }
    return -1;
}

void internalizaCiclistas(int pos){
    int aux;
    for(int i = 1; i < 10; i++){
        if(pista[i-1][pos] < 0 && pista[i][pos] > 0){
            ciclistas[pista[i][pos]-1].faixa = i-1;
            aux = pista[i][pos];
            pista[i][pos] = pista[i-1][pos];
            pista[i-1][pos] = aux;
        }
    }
}

void procuraVencedor(){
    int potenciaisVencedores[10] = {0};
    int index = 0;
    for(int i = 0; i < 10; i++){
    pthread_mutex_lock(&pistaMut);
        if(ciclistas[pista[i][0]-1].voltas == winVoltas){
            potenciaisVencedores[index++] = pista[i][0]-1;
        }
    pthread_mutex_unlock(&pistaMut);
    }
    if(index > 0){
        int cicliVencedor = potenciaisVencedores[(rand()%index)];
        ciclistas[cicliVencedor].vencedor = 1;
        podio[podioPos++]=cicliVencedor + 1;
        winVoltas+=2;
    }
}

int maxVoltasNaChegada(){
    for(int i = 0; i < 10; i++){
        if(ciclistas[pista[i][0]-1].voltas == maxVoltas)
            return 1;
    }
    return 0;
}

void pedalaArmstrong(Ciclista *ciclista){
    int pos,posPrev, faixaLivre;
    int andou = 0;
    ciclista->pos *=2;
    pos = ciclista->pos/2;

    if(ciclista->pos == 0)
        ciclista->voltas++;

    while(1){
        andou = 0;
        pthread_mutex_lock(&nCiclistasMut);
        if(nCiclistas == 1){
            podio[podioPos++] = ciclista->id;
            pthread_mutex_unlock(&nCiclistasMut);
            ultimoCiclistaHandler(ciclista,pos);
            return;
        }
        if(ciclista->vencedor){
            pthread_mutex_lock(&pistaMut);
            pista[ciclista->faixa][pos] = -1;
            nCiclistas--;
            internalizaCiclistas(pos);
            pthread_mutex_unlock(&pistaMut);
            break;
            
        }
        if(ciclista->quebrou){
            printf("O ciclista %d quebrou na volta %d.\n", ciclista->id, ciclista->voltas);
            fflush(stdout);
            pthread_mutex_lock(&pistaMut);
            idQuebrados[quebrados++] = ciclista->id;
            voltaQuebrados[ciclista->id-1] = ciclista->voltas;
            pista[ciclista->faixa][pos] = -1;
            nCiclistas--;
            internalizaCiclistas(pos);
            pthread_mutex_unlock(&pistaMut);
            break;

        }
        pthread_mutex_unlock(&nCiclistasMut);

        posPrev = pos;
        ciclista->pos = (ciclista->pos+ciclista->velocidade) % (2*d);
        pos = ciclista->pos/2;

        if(pos != posPrev){
            pthread_mutex_lock(&pistaMut);
            faixaLivre = achaFaixaLivre(pos, ciclista->faixa);
           
            if(faixaLivre >= 0){
                //garante que ele só tira o ID dele da pista.
                if(pista[ciclista->faixa][posPrev] == ciclista->id)
                    pista[ciclista->faixa][posPrev] = -1;
                ciclista->faixa = faixaLivre;
                pista[ciclista->faixa][pos] = ciclista->id;
                internalizaCiclistas(posPrev);
                andou = 1;
            }
            else{
                ciclista->pos = ((posPrev*2)+1) % (2*d);
                pos = ciclista->pos/2;
            }

            pthread_mutex_unlock(&pistaMut);
        }

        if(andou){
            if(pos == 0){
                ciclista->voltas++;
                lastCrossTime[ciclista->id-1] = raceClock;
                pthread_mutex_lock(&maxVoltasMut);
                if(ciclista->voltas > maxVoltas)
                    maxVoltas = ciclista->voltas;
                pthread_mutex_unlock(&maxVoltasMut);
                if(ciclista->voltas >= 1){
                    if(ciclista->velocidade == 1){
                        if(sorteio(70))
                            ciclista->velocidade = 2;
                    }
                    else{
                        if(sorteio(50))
                            ciclista->velocidade = 1;
                    }
                }
            }
        }

        pthread_mutex_lock(&nCiclistasMut);
        if(andou && nCiclistas > 1){
            if(pos == 0 && ciclista->voltas % 6 == 0 && ciclista->voltas > 0){
                if(sorteio(15)){
                    ciclista->quebrou = 1;
                }
            }
        }
        pthread_mutex_unlock(&nCiclistasMut);

        pthread_barrier_wait(&barreiraAndaram);
        pthread_barrier_wait(&barreiraPrint);
        
    }
    ciclistaFora(ciclista);
    return;
}

void printVoltaCompletada(int k){
    printf(" ________________________________________________\n");
    printf("|         Posição dos ciclistas em %*.2f s  |\n", 10,raceClock);
    printf("|________________________________________________|\n");
    printf("|         id           |   Posicao do Ciclista   |\n");
    printf("|______________________|_________________________|\n");
    for(int i = 0; i < k; i++){
        if(!ciclistas[i].quebrou && !ciclistas[i].vencedor){
            if(ciclistas[i].voltas < maxVoltas)
                printf("|%11d           |%9d m [RET %3d]    |\n", ciclistas[i].id, ciclistas[i].pos/2, maxVoltas - ciclistas[i].voltas);
            else
                printf("|%11d           |%13d m          |\n", ciclistas[i].id, ciclistas[i].pos/2);
        }
    }
    printf("|________________________________________________|\n");

    
}

void printaVencedores(){
    printf(" ________________________________________________\n");
    printf("|      Ciclistas que completaram a corrida       |\n");
    printf("|________________________________________________|\n");
    printf("|   Colocação   |    id    |    ultima chegada   |\n");
    printf("|_______________|__________|_____________________|\n");
    for(int i = 0; i < podioPos; i++){
        printf("|%7d        |%5d     |%*.2f           |\n", i+1, podio[i], 10, lastCrossTime[podio[i]-1]);
    }
    printf("|________________________________________________|\n");
}
void printaArmstrongs(){
    printf(" ________________________________________________\n");
    printf("|          Ciclistas que se quebraram            |\n");
    printf("|________________________________________________|\n");
    printf("|         id           |     Volta da quebrada   |\n");
    printf("|______________________|_________________________|\n");
    for(int i = 0; i < quebrados; i++){
        printf("|%11d           |%13d            |\n", idQuebrados[i], voltaQuebrados[idQuebrados[i]-1]);
    }
    printf("|________________________________________________|\n");
}

int main(int argc, char *argv[]){
    int debug = 0;
    if(argc < 3){
        printf("Deve dar pelo menos dois argumentos: ./ep2 <d> <k> [-debug(opcional)]");
        return 1;
    }
    if(argc > 3){
        if(strcmp(argv[3], "-debug") == 0){
            debug = 1;
        }
        else{
            printf("Opcão %s inválida. Flag deve ser \'-debug\'", argv[3]);
            return 1;
        }
    }
    int k = atoi(argv[2]);
    d = atoi(argv[1]);
    nCiclistas = k; 
    //Inicializa Coisas
    pthread_barrier_init(&barreiraAndaram, NULL, k+1);
    pthread_barrier_init(&barreiraPrint, NULL, k+1);
    pthread_mutex_init(&nCiclistasMut, NULL);
    pthread_mutex_init(&pistaMut, NULL);
    pthread_mutex_init(&maxVoltasMut, NULL);
    for(int i = 0; i < 10; i++){
        pista[i] = malloc(sizeof(int)*d);
    }
    lastCrossTime = malloc(sizeof(double)*k);
    podio = malloc(sizeof(int)*k);
    idQuebrados = malloc(sizeof(int)*k);
    voltaQuebrados = malloc(sizeof(int)*k);
    ciclistas = malloc(sizeof(Ciclista)*k);
    ciclistasShuffle = malloc(sizeof(Ciclista)*k);
    tids = malloc(sizeof(pthread_t)*k);
    for(int i = 0; i < k; i++){
        ciclistas[i].id = i+1;
        ciclistas[i].voltas = -1;
        ciclistas[i].velocidade = 1;
        ciclistas[i].vencedor = 0;
        ciclistas[i].quebrou = 0;
    }

    shuffleCiclistas(k);
    inicializaPista();
    posicionaLargada(k);
    free(ciclistasShuffle);
    if(debug){
        printf("=====Iniciou-se a corrida=====\n");
        printaPista();
    }

    //Criação das threads.
    for(int i = 0; i < k; i++){
        if((pthread_create(&tids[i],NULL,(void *)pedalaArmstrong, &ciclistas[i])) != 0){
            perror("Falha ao criar thread.");
        }
    }
    raceClock += 0.06;
    pthread_mutex_lock(&nCiclistasMut);
    while(nCiclistas > 0){
        pthread_mutex_unlock(&nCiclistasMut);
        pthread_barrier_wait(&barreiraAndaram);
        if(debug){
            printf("=====Proximo passo===== [%.2f s]\n", raceClock);
            printaPista();
        }
        else {
            if(maxVoltasNaChegada()){
                printVoltaCompletada(k);
            }
        }
        maxVoltas = -1;
        procuraVencedor();
        raceClock += 0.06;
        pthread_barrier_wait(&barreiraPrint);
        pthread_mutex_lock(&nCiclistasMut);
    }
    pthread_mutex_unlock(&nCiclistasMut);
    
    //Join nas threads.
    for(int i = 0; i < k; i++){
        if((pthread_join(tids[i],NULL)) != 0){
            perror("Falha ao dar join em thread.");
        }
    }
    printaVencedores();
    printaArmstrongs();
    printf("%d terminaram a corrida.\n", podioPos);
    printf("%d se quebraram.\n", quebrados);

    //free stuff
    pthread_barrier_destroy(&barreiraAndaram);
    pthread_barrier_destroy(&barreiraPrint);
    pthread_mutex_destroy(&nCiclistasMut);
    pthread_mutex_destroy(&pistaMut);
    pthread_mutex_destroy(&maxVoltasMut);
    for(int i = 0; i < 10; i++){
        free(pista[i]);
    }
    free(idQuebrados);
    free(voltaQuebrados);
    free(lastCrossTime);
    free(podio);
    free(ciclistas);
    free(tids);
    return 0;
}
