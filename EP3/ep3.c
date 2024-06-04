#include "ep3.h"
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
//TODOS:
//GLOBAIS
int mountedFS = FALSE;
FILE *FSFile;
uint8_t *BMP;
uint16_t *FAT;
//FUNCOES MISC
void loadBMP(){
    for(int i = 0; i < BITMAP_BYTES; i++){
        uint8_t byte;
        (void)!fread(&byte, sizeof(uint8_t), 1, FSFile);
        BMP[i] = byte;
    }
}

void setBMP(uint8_t index, bool b){
    uint8_t i = index/8;
    if(b)
        BMP[i] = BMP[i] | 128 >> index%8;
    else
        BMP[i] = BMP[i] & (0xFF - (128 >> index%8));
    printf("BMP[%d] is now %02x\n", i, BMP[i]);
}

void loadFAT(){
    FAT[0] = LAST_BLOCK;
    for(int i = 1; i < 13; i++){
        FAT[i] = i+1;
    }
    FAT[14] = LAST_BLOCK;
    FAT[15] = LAST_BLOCK;
}

void createMountableFile(){
    //Escreve o bitmap no primeiro bloco,
    // contando que a fat vai ocupar 13 blocos,
    // e depois, o bloco que contém o root.
    uint8_t *data = calloc(DISK_BYTES, sizeof(uint8_t));
    data[0] = 0xFF; //11111111
    data[1] = 0xFE; //11111110
    //todo: write fat to disk;  
    fwrite(data, sizeof(uint8_t), DISK_BYTES, FSFile);
    free(data);
    rewind(FSFile);
    loadBMP();
    loadFAT();
    //BIG TO DO: ONLY WRITE BMP AND FAT TO 
    // DISK ONCE UMOUNTING / exiting;
}
uint16_t findFreeBlock(){
    for(int i = 0; i < BLOCK_NUMBER; i++){
        for(int j = 0; j < 8; j++){
            if(!(BMP[i] & 128 >> j)){
                uint16_t ret = (i*8) + j;
                FAT[ret] = LAST_BLOCK;
                setBMP(ret, true);
                return ret;
            }
        }
    }
    printf("DISCO CHEIO, impossível continuar.");
    return -1;
}

void findEntrySpace(uint16_t size){
    uint16_t curBlock = ftell(FSFile)/4096;
    uint16_t blockPtr;
    while(true){
        (void)!fread(&blockPtr, 2, 1, FSFile);
        fseek(FSFile, -2, SEEK_CUR);
        blockPtr = htons(blockPtr);
        //se nao ha mais entradas no bloco atual
        if(blockPtr == 0){
            //se cabe a entrada nesse bloco
            if((curBlock +1)*4096 - ftell(FSFile) >= size){
                return;
            }
            else{
                //se for o ultimo bloco.
                if(FAT[curBlock] == LAST_BLOCK){
                        FAT[curBlock] = findFreeBlock();
                        curBlock = FAT[curBlock];
                        printf("Now going to block: %d\n", curBlock);
                        fseek(FSFile, curBlock * BLOCK_SIZE, SEEK_SET);
                        return;
                }
                //se nao for, continua buscando no proximo bloco.
                else {
                    curBlock = FAT[curBlock];
                    fseek(FSFile, curBlock * BLOCK_SIZE, SEEK_SET);
                    continue;
                }
            }
        }
        if(blockPtr & DIR_INDICATOR){
            blockPtr -= DIR_INDICATOR;
            fseek(FSFile, DIR_ATTR_SIZE+2, SEEK_CUR);
        }
        else
            fseek(FSFile, FIL_ATTR_SIZE+2, SEEK_CUR);
        uint8_t c;
        while((c = fgetc(FSFile)) != '\0'){}
    }
    
}

void writeEntry(char *entryName, uint8_t ehDir){
    int stringLen = strlen(entryName);
    //limite de tamanho para o tamanho da entrada caber em 8 bits.
    if(stringLen > 240){
        printf("criadir: o nome do diretorio deve ser menor.");
        return;
    }
    uint8_t size = 15 + stringLen;
    findEntrySpace(size);
    uint16_t freeBlock = findFreeBlock();
    time_t now = time(NULL);
    uint8_t *data = malloc(size);
    data[0] = LAST((freeBlock + DIR_INDICATOR) >> 8,8);
    data[1] = LAST(freeBlock,8);
    for(int j = 0; j < 3; j++){
        for(int i = 3; i >=0 ; i--){
            data[5-i+(j*4)] = LAST(now >> i*8, 8);
        }
    }
    fwrite(data, sizeof(uint8_t), size - (stringLen + 1), FSFile);
    for(int i = 0; i < stringLen; i++){
        fputc(entryName[i], FSFile);
    }
    fputc('\0', FSFile);
    free(data);

}


//COMANDOS DO FS
void criadir(char *dirName){
    char *nextDir = strtok(dirName, "/");
    //começa a busca no ROOT
    uint16_t curBlock = 14;
    fseek(FSFile, ROOT_ADDR, SEEK_SET);
    char *parent = dirName;
    nextDir = strtok(NULL, "/");
    //eh filho do ROOT
    if(nextDir == NULL){
        writeEntry(parent, DIR);
        return;
    }
    //se nao eh filho do root
    uint16_t blockPtr;
    while(true){
        (void)!fread(&blockPtr, 2, 1, FSFile);
        blockPtr = htons(blockPtr);
        //se nao ha mais entradas no bloco atual
        if(blockPtr == 0){
            //se for o ultimo bloco
            if(FAT[curBlock] & LAST_BLOCK){
                return;
            }
            else {
                curBlock = FAT[curBlock];
                fseek(FSFile, curBlock * BLOCK_SIZE, SEEK_SET);
                continue;
            }
        }
        if(blockPtr & DIR_INDICATOR){
            blockPtr -= DIR_INDICATOR;
            fseek(FSFile, DIR_ATTR_SIZE, SEEK_CUR);
        }
        else
            fseek(FSFile, FIL_ATTR_SIZE, SEEK_CUR);
        uint8_t i = 0, c;
        char name[240];
        while((c = fgetc(FSFile)) != '\0'){
            name[i++] = c; 
        }
        name[i] = c;
        if(strcmp(name, parent) == 0){
            fseek(FSFile, blockPtr*BLOCK_SIZE, SEEK_SET);
            parent = nextDir;
            nextDir = strtok(NULL, "/");
            if(nextDir == NULL){
                writeEntry(parent, DIR);
                return;
            }
        }
    }

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
