#include "ep1.h"

processArray *pQueue = NULL;
int runHeader = 0;
pthread_mutex_t cpuMutex, globalClockLock;
pthread_cond_t cond;
clock_t t0Global = 0;
clock_t t_elapsedGlobal = 0;
int terminatedProcess = 0;
int contextChange = 0;
int runningProcesses = 0;
FILE *outputFile;

void resize(processArray *pArr){
    pArr->size *= 2;
    pArr->Arr = (process **)realloc(pArr->Arr, pArr->size * sizeof(process *));
}

void push(processArray *pArr, process *process){
    if(pArr->count == pArr->size) resize(pArr);
    pArr->Arr[pArr->count++] = process;
}

int nextValidProcess(){
    int i;
    for(i = runHeader; i < pQueue->count + runHeader;i++){
        if(pQueue->Arr[i%pQueue->count]->status != RAN){
            return i%pQueue->count;
        }
    }
    return -1;
}

void assembleProcessArray(processArray *pArr,char * traceFilename){
    char lineBuf[128];

    FILE *traceFile = fopen(traceFilename, "r");
    while(fgets(lineBuf, sizeof(lineBuf), traceFile)){
        process *process = (void *)malloc(sizeofProcess);
        mutexNode *auxMutexNode = (void *)malloc(sizeof(mutexNode));

        strcpy(process->name,strtok(lineBuf, " "));
        process->deadline = atof(strtok(NULL, " "));
        process->t0 = atof(strtok(NULL, " "));
        process->dt = atof(strtok(NULL, " \n"));
        process->status = READY;        
        process->pMutexNode = auxMutexNode;
        push(pArr, process);
    }

    fclose(traceFile);
}

void pushToProcessQueue(processArray *pArr, processArray *pQueue, double tNow){
    int i;
    for(i = pArr->readHeader; i < pArr->count; i++){
        if(pArr->Arr[i]->t0 <= tNow){
            push(pQueue,pArr->Arr[i]);
            pArr->readHeader = i+1;
        }
    }
}

int arrivalOrderComp(const void *first, const void *second){
    double f = (*(process **)first)->t0;
    double s = (*(process **)second)->t0;
    if((f - s) > 0 + epsilon)return 1;
    if((f - s) < 0 - epsilon)return -1;
    return 0;

}

int SJFComp(const void *first, const void *second){
    double f = (*(process **)first)->dt;
    double s = (*(process **)second)->dt;
    if((f - s) > 0 + epsilon)return 1;
    if((f - s) < 0 - epsilon)return -1;
    return 0;

}

int definePrioridade(double deadline, double tNow){
    double timeRemaining = deadline - tNow;
    if(timeRemaining < QUANTUM)
        return 5;
    if(timeRemaining < 2 * QUANTUM)
        return 4;
    if(timeRemaining < 4 * QUANTUM)
        return 3;
    if(timeRemaining < 6 * QUANTUM)
        return 2;

    return 1;
}

void simulateProcess(void *arg){
    unsigned long long int taskCont = 0;
    process *process = ((pthreadArgs *)arg)->p; 
    process->status = RUNNING;
    
    pthread_mutex_lock(&cpuMutex);
    while(strcmp(pQueue->Arr[runHeader]->name, process->name) != 0 && runningProcesses > 1){
        pthread_cond_wait(&cond, &cpuMutex);
    }
        double timeRunning = 0;
        double dt = process->dt;
        double lim;
        int id = ((pthreadArgs*)arg)->id;
        clock_t t0, t0Running;

        t0 = t_elapsedGlobal;
        switch(id){
            case -1:
                lim = dt;
                break;
            case 0:
                lim = QUANTUM;
                break;
            default:
                lim = id * QUANTUM;
        }

        t0Running = t0;
        while (timeRunning - dt < 0 + epsilon) {
            pthread_mutex_lock(&globalClockLock);
            t_elapsedGlobal = clock();
            pthread_mutex_unlock(&globalClockLock);
            timeRunning = (double)(t_elapsedGlobal-t0Running)/CLOCKS_PER_SEC;
            taskCont++;
            if(timeRunning - lim >= 0 + epsilon && lim < dt
                && runningProcesses > 1){
                contextChange++;
                dt -= timeRunning;
                timeRunning = 0;
                runHeader++;
                runHeader = nextValidProcess();
                pthread_cond_broadcast(&cond);
                while(strcmp(pQueue->Arr[runHeader]->name, process->name) != 0 && runningProcesses > 1){
                    pthread_cond_wait(&cond, &cpuMutex);
                }
                if(id > 0){
                    lim = QUANTUM * definePrioridade(process->deadline, t_elapsedGlobal);  
                }
                t0Running = clock();
            }
        }
        runningProcesses--;
        terminatedProcess++;
        
        fprintf(outputFile,"%s %.2f %.2f", process->name, 
                ((double)(t_elapsedGlobal)/CLOCKS_PER_SEC)-process->t0,
                (double)(t_elapsedGlobal - t0Global)/CLOCKS_PER_SEC);
        fprintf(outputFile,"\n");
        
        process->status = RAN;
        
        runHeader = nextValidProcess();
        pthread_mutex_unlock(&cpuMutex);
        pthread_cond_broadcast(&cond);

    pthread_exit(NULL);
}

void SJF(processArray *pArr){
    pQueue = (void *)malloc(sizeof(process**) + sizeof(int) * 3);
    pQueue->Arr = (process **)malloc(sizeof(process *) * 16);
    pQueue->readHeader = 0;
    pQueue->count = 0;
    pQueue->size = 16;
    
    t0Global = clock();
    double elapsed_time = 0;

    while(terminatedProcess < pArr->count){
        t_elapsedGlobal = clock();
        elapsed_time = (double)(t_elapsedGlobal - t0Global)/CLOCKS_PER_SEC;
        pushToProcessQueue(pArr, pQueue, elapsed_time);
        if(pQueue->readHeader < pQueue->count){
            qsort(&(pQueue->Arr[pQueue->readHeader]), (pQueue->count - pQueue->readHeader), sizeof(process **), SJFComp);
        }
        
        if(pQueue->count > 0 && terminatedProcess < pQueue->count){
            pthread_t tid;
            pthreadArgs *args = (void *)malloc(sizeof(void *) + sizeof(int));
            args->p = pQueue->Arr[pQueue->readHeader];
            args->id = -1;
            runningProcesses++;         
            runHeader = 0;
            runHeader = nextValidProcess();
            pthread_create(&tid, NULL, (void *)simulateProcess, (void *)args);
            pthread_join(tid, NULL);
            
            pQueue->readHeader++;
            free(args);
        }
    }
     
    fprintf(outputFile,"%d\n", contextChange);
    free(pQueue->Arr);
    free(pQueue);
    fclose(outputFile);
}

void RR(processArray *pArr){
    pQueue = (void *)malloc(sizeof(process**) + sizeof(int) * 3);
    pQueue->Arr = (process **)malloc(sizeof(process *) * 16);
    pQueue->readHeader = 0;
    pQueue->count = 0;
    pQueue->size = 16;
    
    t0Global = clock();
    double elapsed_time = 0;
    int nThreads = 0;
    int i;

    pthread_t *threads = (pthread_t *)malloc(sizeof(pthread_t) * pArr->count);
    pthreadArgs **args = (pthreadArgs **)malloc(sizeof(pthreadArgs *) * pArr->count);
    runHeader = 0;

    while(nThreads < pArr->count){
        t_elapsedGlobal = clock();
        elapsed_time = (double)(t_elapsedGlobal - t0Global)/CLOCKS_PER_SEC;
        pushToProcessQueue(pArr, pQueue, elapsed_time);
        for(i = 0; i < pQueue->count; i++){
            if(pQueue->Arr[i]->status == READY){
                if(runningProcesses == 0){
                    runHeader = 0;
                    runHeader = nextValidProcess();
                } 
                pQueue->Arr[i]->status = RUNNING;
                pthreadArgs *auxArgs = (void *)malloc(sizeof(void *) + sizeof(int));
                auxArgs->p = pQueue->Arr[i];
                auxArgs->id = 0;
                args[i] = auxArgs;

                runningProcesses++;         

                pthread_create(&threads[i], NULL, (void *)simulateProcess, (void *)auxArgs);
                
                nThreads++;
            }
        }
    }

    for(i = 0; i < pArr->count; i++){
        pthread_join(threads[i],NULL);
        free(args[i]);
    }
    
    free(threads);
    free(args);
    fprintf(outputFile,"%d\n", contextChange);
    free(pQueue->Arr);
    free(pQueue);
    fclose(outputFile);

}
void RRwithPriority(processArray *pArr){
    pQueue = (void *)malloc(sizeof(process**) + sizeof(int) * 3);
    pQueue->Arr = (process **)malloc(sizeof(process *) * 16);
    pQueue->readHeader = 0;
    pQueue->count = 0;
    pQueue->size = 16;
    
    t0Global = clock();
    double elapsed_time = 0;
    int nThreads = 0;
    int i;

    pthread_t *threads = (pthread_t *)malloc(sizeof(pthread_t) * pArr->count);
    pthreadArgs **args = (pthreadArgs **)malloc(sizeof(pthreadArgs *) * pArr->count);
    runHeader = 0;

    while(nThreads < pArr->count){
        t_elapsedGlobal = clock();
        elapsed_time = (double)(t_elapsedGlobal - t0Global)/CLOCKS_PER_SEC;
        pushToProcessQueue(pArr, pQueue, elapsed_time);
        for(i = 0; i < pQueue->count; i++){
            if(pQueue->Arr[i]->status == READY){
                if(runningProcesses == 0){
                    runHeader = 0;
                    runHeader = nextValidProcess();
                } 
                pQueue->Arr[i]->status = RUNNING;
                pthreadArgs *auxArgs = (void *)malloc(sizeof(void *) + sizeof(int));
                auxArgs->p = pQueue->Arr[i];
                auxArgs->id = definePrioridade(pQueue->Arr[i]->deadline,elapsed_time);
                args[i] = auxArgs;

                runningProcesses++;         

                pthread_create(&threads[i], NULL, (void *)simulateProcess, (void *)auxArgs);
                
                nThreads++;
            }
        }
    }

    for(i = 0; i < pArr->count; i++){
        pthread_join(threads[i],NULL);
        free(args[i]);
    }
    
    free(threads);
    free(args);
    fprintf(outputFile,"%d\n", contextChange);

    free(pQueue->Arr);
    free(pQueue);
    fclose(outputFile);

}

int main(int argc, char *argv[]){
    if(argc < 4){
        fprintf(stderr, "%s", "Deve ter 3 argumentos\n");
        return 1;
    }

    /*DECLARE STUFF*/
    pthread_mutex_init(&cpuMutex, NULL);
    pthread_mutex_init(&globalClockLock, NULL);
    pthread_cond_init(&cond, NULL);
    
    int i;
    int rflag = 0;

    int scheduler = argv[1][0] - 48;
    char *outputFilename = argv[3];
    outputFile = fopen(outputFilename, "w");

    processArray *pArr = (void *)malloc(sizeof(process**) + sizeof(int) * 3);
    pArr->Arr = (process **)malloc(sizeof(process *) * 16);
    pArr->readHeader = 0;
    pArr->count = 0;
    pArr->size = 16;

    /*DO STUFF*/
    assembleProcessArray(pArr, argv[2]);
    qsort(pArr->Arr, pArr->count, sizeof(process **), arrivalOrderComp);
    /*assembleMutexLL(pArr);*/
    switch(scheduler){
        case 1:
            SJF(pArr);
            break;
        case 2:
            RR(pArr);
            break;
        case 3:
            RRwithPriority(pArr);
            break;
        default:
            fprintf(stderr, "%s", "O escalonador deve ser 1, 2 ou 3");
            rflag = 2;
            break;
    }

    
    for(i = 0; i < pArr->count; i++){
        free(pArr->Arr[i]);
    }
    pthread_mutex_destroy(&cpuMutex);
    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&globalClockLock);
    /*FREE AND CLOSE STUFF*/
    free(pArr->Arr);
    free(pArr);

    return rflag;
}
