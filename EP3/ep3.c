#include "ep3.h"
#include <stdio.h>
#include <stdlib.h>
//TODOS:
//-> o arquivo deve ser escrito só quando desmontar o FS.
//
//GLOBAIS
int mountedFS = FALSE;
FILE *FSFile;
Dir *ROOT = NULL;
//Construtores
Dir *Dir_new(char *name){
    Dir *ret = malloc(sizeof(Dir));
    strcpy(ret->meta.name, name);
    ret->size = 16;
    ret->children = malloc(sizeof(Entry *) * 16);
    time_t now = time(NULL);
    struct tm* ts = localtime(&now);
    ret->meta.created = ts;
    ret->meta.lastAccessed = ts;
    ret->meta.lastModified = ts;
    
    return ret;
}
//FUNCOES MISC
int findChild(Dir *parent, char *dirName, char *caller,int mode){
    //acha por busca binária o diretório na lista
    printf("%s: %s: não existe tal arquivo ou diretório.\n",caller, dirName);
    printf("%s: %s: não é um diretório.\n",caller, dirName);
    printf("%s: %s: não é um arquivo.\n",caller, dirName);
    return 0;
}
//COMANDOS DO FS
void criadir(char *dirName){
    //Considera criação do ROOT
    if(strcmp(dirName, "/") == 0 && ROOT == NULL){
        ROOT = Dir_new(dirName);
    }
    else {
        printf("Nome de diretório não pode ser root (\"\\\")");
    }
    //Caso geral
    Dir *parent = ROOT;
    char *next = strtok(dirName, "/");
    while(next != NULL){
        int childIndex = findChild(parent, next, "criadir", MODE_DIR);
        if(childIndex < 0){
            return;
        }
        else {
            parent = parent->children[childIndex]->dir; 
            next = strtok(NULL,"/");
        }

    }


}
void monta(char *path){
    //todo: buscar arquivo no fs real
    //se não achou no fs real:
        mountedFS = TRUE;
        criadir("/");
}

//PROMPT E LEITURA DO PROMPT
char *promptUser(){
    char *command = (char *)malloc(sizeof(char) * 1024);
    command = readline("{ep3}: ");
    return command;
}
int handleCommand(char *command){
    char *token = strtok(command, " ");
    if(strcmp(token, "monta") == 0){
        monta(strtok(NULL, " "));
    }
    else if(strcmp(token, "criadir") == 0){
        criadir(strtok(NULL, " "));
    }
    free(command);
    return 0;
}

int main(){
    while(TRUE){
        char *command = promptUser();
        if(strcmp(command, "sai") == 0)
            break;
        handleCommand(command);
    }

    return 0;
}
