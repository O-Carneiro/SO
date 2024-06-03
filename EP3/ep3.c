#include "ep3.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
//TODOS:
//GLOBAIS
int mountedFS = FALSE;
FILE *FSFile;
uint8_t *BMP;
int *FAT;
//FUNCOES MISC
void loadBMP(){
    for(int i = 0; i < BITMAP_BYTES; i++){
        uint8_t byte;
        fread(&byte, sizeof(uint8_t), 1, FSFile);
        BMP[i] = byte;
    }
}

void setBMP(uint8_t index, bool b){
    uint8_t i = index/8;
    if(b)
        BMP[i] = BMP[i] | 128 >> index%8;
    else
        BMP[i] = BMP[i] & (0xFF - (128 >> index%8));
}

void loadFAT(){

}

void createMountableFile(){
    //Escreve o bitmap no primeiro bloco,
    // contando que a fat vai ocupar 13 blocos,
    // e depois, o bloco que contém o root.
    uint8_t *data = calloc(DISK_BYTES, sizeof(uint8_t));
    data[0] = 0xFF; //11111111
    data[1] = 0xFE; //11111110
    //todo: escrever o FAT inicial no arquivo.
    fwrite(data, sizeof(uint8_t), DISK_BYTES, FSFile);
    free(data);
    rewind(FSFile);
    loadBMP();
    loadFAT();
}
uint16_t findFreeSpace(){
    for(int i = 0; i < BLOCK_NUMBER; i++){
        for(int j = 0; j < 8; j++){
        if(!(BMP[i] & 128 >> j))
            return (i*8) + j;
        }
    }
    printf("DISCO CHEIO, impossível continuar.");
    return -1;
}


//COMANDOS DO FS
void criadir(char *dirName){
    int stringLen = strlen(dirName);
    //limite de tamanho para o tamanho da entrada caber em 8 bits.
    if(stringLen > 239){
        printf("criadir: o nome do diretorio deve ser menor.");
    }
    //começa a busca no ROOT
    fseek(FSFile, ROOT_ADDR, SEEK_SET);
    //vai até o diretório certo e bota o ponteiro de leitura no lugar 
    //certo.
    //todo;
    //O tamanho da entrada será:
    // 1B = tamanho da entrada
    // 2B = ponteiro pro bloco do diretorio
    // 12B = 3 time_t pros instantes de tempo (serão apenas 4 bytes até 2038)
    // nB = n é o tamanho da string de nome contando o 0 final.
    // TOTAL = 15 + n B.
    uint8_t size = 16 + stringLen;
    uint16_t freeSpace = findFreeSpace();
    FAT[freeSpace] = -1;
    setBMP(freeSpace, true);
    time_t now = time(NULL);
    uint8_t *data = malloc(size);
    data[0] = size;
    data[1] = LAST((freeSpace + DIR_INDICATOR) >> 8,8);
    data[2] = LAST(freeSpace,8);
    for(int j = 0; j < 3; j++){
        for(int i = 3; i >=0 ; i--){
            data[6-i+(j*4)] = LAST(now >> i*8, 8);
        }
    }
    fwrite(data, sizeof(uint8_t), 15, FSFile);
    for(int i = 0; i < stringLen; i++){
        fputc(dirName[i], FSFile);
    }
    fputc('\0', FSFile);
    free(data);
}
void monta(char *path){
    FSFile = fopen(path, "rb+");
    if(FSFile == NULL){
        FSFile = fopen(path, "wb+");
        createMountableFile();
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
        criadir(strtok(NULL, " "));
    }
    free(command);
    return 0;
}


int main(){
    BMP = calloc(BITMAP_BYTES, sizeof(uint8_t));
    FAT = malloc(BLOCK_NUMBER * sizeof(int));
    // memset(FAT, -1, BLOCK_NUMBER * sizeof(int));
    while(TRUE){
        char *command = promptUser();
        if(strcmp(command, "sai") == 0)
            break;
        handleCommand(command);
    }
    fclose(FSFile);
    free(BMP);
    free(FAT);
    return 0;
}
