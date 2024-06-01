#include "ep3.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
//TODOS:
//-> o arquivo deve ser escrito só quando desmontar o FS.
//
//GLOBAIS
int mountedFS = FALSE;
FILE *FSFile;
bool BMP[BLOCK_NUMBER] = {false};
//FUNCOES MISC
void loadBMP(char *path){
    uint8_t BMPBytes[3200];
    fread(BMPBytes, sizeof(uint8_t), 3200, FSFile);
    for(int i = 0; i < BITMAP_BYTES; i++){
        uint8_t n = 128; //10000000
        for(int j = 0; j < 8; j++){
           if(BMPBytes[i] & n >> j)
               BMP[i*8 + j] = true;
        }
    }
    for(int i = 0; i < 50; i++){
        printf("%d ", BMP[i]);
        if(i % 24 == 0)
            printf("\n");
    }
    printf("\n");
    fclose(FSFile);
}

void loadFAT(){

}

void montaFromScratch(char *fileName){
    //Escreve o bitmap no primeiro bloco,
    // contando que a fat vai ocupar 13 blocos,
    // e depois, o bloco que contém o root.
    uint8_t *data = malloc(sizeof(uint8_t) * DISK_BYTES);
    data[0] = 0xFF; //11111111
    data[1] = 0xFE; //11111110
    fwrite(data, sizeof(uint8_t), DISK_BYTES, FSFile);
    free(data);
    uint8_t byte[2];
    fread(byte, sizeof(uint8_t), 1, FSFile);
    printf("byte read: %d", byte[0]);
    // fclose(FSFile);
    // // FSFile = fopen(fileName, "wb+");
    // loadBMP(fileName);
    loadFAT();
}


//COMANDOS DO FS
void criadir(char *dirName){

}
void monta(char *path){
    FSFile = fopen(path, "rb+");
    if(FSFile == NULL){
        FSFile = fopen(path, "wb+");
        montaFromScratch(path);
    }
    else {
        return;
    }
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
        // criadir(strtok(NULL, " "));
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
    // fclose(FSFile);

    return 0;
}
