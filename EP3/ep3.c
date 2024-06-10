#include "ep3.h"
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
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

void loadFAT(){
}

void setBMP(uint8_t index, bool b){
    uint8_t i = index/8;
    if(b)
        BMP[i] = BMP[i] | 128 >> index%8;
    else
        BMP[i] = BMP[i] & (0xFF - (128 >> index%8));
}

void createMountableFile(){
    //Escreve o bitmap no primeiro bloco,
    // contando que a fat vai ocupar 13 blocos,
    // e depois, o bloco que contém o root.
    uint8_t *data = calloc(DISK_BYTES, sizeof(uint8_t));
    fwrite(data, sizeof(uint8_t), DISK_BYTES, FSFile);
    free(data);
    rewind(FSFile);
    //Carrega bitmap inicial
    BMP[0] = 0xFF;
    BMP[1] = 0xFE;
    //carrega FAT inicial
    FAT[0] = LAST_BLOCK;
    for(int i = 1; i < 13; i++){
        FAT[i] = i+1;
    }
    FAT[13] = LAST_BLOCK;
    FAT[14] = LAST_BLOCK;
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
    //err
    printf("DISCO CHEIO, impossível continuar.");
    exit(0);
}

Entry readEntry(){
    Entry ret;
    fread(&ret, ENTRY_SIZE, 1, FSFile);
    return ret;
}

int nextBlock(uint16_t curBlock, bool mode){
    if(FAT[curBlock] == LAST_BLOCK){
        if(mode){
            FAT[curBlock] = findFreeBlock();
            curBlock = FAT[curBlock];
            fseek(FSFile, curBlock * BLOCK_SIZE, SEEK_SET);
        }
        else
            curBlock = 0;

        return curBlock;
    }
    else {
        curBlock = FAT[curBlock];
        fseek(FSFile, curBlock * BLOCK_SIZE, SEEK_SET);
        return curBlock;
    }
}

void findEntrySpace(){
    uint16_t curBlock = ftell(FSFile)/4096;
    Entry e;
    while(true){
        if(ftell(FSFile)/BLOCK_SIZE != curBlock)
            curBlock = nextBlock(curBlock, ALLOC);
        if(!curBlock)// err no space
            return;
        e = readEntry(); 
        if(e.blockPtr == 0){
            fseek(FSFile, - ENTRY_SIZE, SEEK_CUR);
            return;
        }
    }
}

bool entryExists(char *fileName){
    uint16_t curBlock = ftell(FSFile)/BLOCK_SIZE;
    Entry e;
    while(true){
        if(ftell(FSFile)/BLOCK_SIZE != curBlock)
            curBlock = nextBlock(curBlock, NO_ALLOC);
        if(!curBlock)// err no space
            return false;
        e = readEntry(); 
        if(e.blockPtr == 0){
            fseek(FSFile, - ENTRY_SIZE, SEEK_CUR);
            return false;
        }
        if(strcmp(e.name, fileName) == 0){
            fseek(FSFile, - ENTRY_SIZE, SEEK_CUR);
            return true;
        }
    }
}
void writeEntry(char *entryName, uint32_t size, uint8_t ehDir){
    findEntrySpace();
    int stringLen = strlen(entryName);
    if(stringLen > 234){
        printf("toca: o nome do arquivo deve ser menor.");
        return;
    }

    Entry e;
    time_t now = time(NULL);
    e.blockPtr = findFreeBlock();
    if(ehDir)
        e.blockPtr += DIR_INDICATOR;
    e.size = size;
    for(int i = 0; i < 238; i++){
        e.name[i] = '\0';
    }
    strcpy(e.name, entryName);
    e.createTime = now;
    e.modTime = now;
    e.accessTime = now;
    fwrite(&e, ENTRY_SIZE, 1, FSFile);
}

void pathTo(char *path, char **parent, bool stoppage,bool mode){
    uint16_t curBlock = ROOT_BLOCK;
    fseek(FSFile, ROOT_ADDR, SEEK_SET);
    if(strcmp(path, "/") == 0) return;
    char *nextDir = strtok(path, "/");
    *parent = nextDir;
    nextDir = strtok(NULL, "/");
    if(nextDir == NULL && stoppage){ 
        // is at root, can return
        return;
    }

    Entry e;
    while(true){
        if(ftell(FSFile)/BLOCK_SIZE != curBlock)
            curBlock = nextBlock(curBlock, NO_ALLOC);
        if(!curBlock){
            printf("Next block is Invalid!\n");
            //err dir no exist
            return;
        }
        e = readEntry();
        if(e.blockPtr == 0){
            printf("Read empty entry!\n");
            rewind(FSFile);
            //err dir no exist
            return;
        }
        if(strcmp(e.name, *parent) == 0){
            if(!(e.blockPtr & DIR_INDICATOR) && mode)
                return;//err should be dir but is file
            if(e.blockPtr & DIR_INDICATOR)
                fseek(FSFile, (e.blockPtr - DIR_INDICATOR)*BLOCK_SIZE, SEEK_SET);
            else
                fseek(FSFile, (e.blockPtr)*BLOCK_SIZE, SEEK_SET);
            curBlock = ftell(FSFile)/BLOCK_SIZE;
            *parent = nextDir;
            nextDir = strtok(NULL, "/");
            if(nextDir == NULL && stoppage){
                return;
            }
            if(*parent == NULL && !stoppage){
                return;
            }
        }
    }
}

void printTime(uint32_t now){
    char ret[30];
    time_t c = (time_t) now;
    struct tm *tm_struct = localtime(&c);
    strftime(ret, sizeof(ret), "%d/%m/%Y %H:%M:%S", tm_struct);
    printf("|%s", ret);
}

void compacta(){
    uint32_t eraseAddr = ftell(FSFile) - ENTRY_SIZE;
    uint32_t lastAddr;
    uint16_t curBlock = ftell(FSFile)/4096;
    char zeroes[ENTRY_SIZE] = {'\0'};
    Entry prev, e;
    e = readEntry();
    lastAddr = ftell(FSFile)-ENTRY_SIZE;
    prev = e;
    while(e.blockPtr != 0){
        lastAddr = ftell(FSFile)-ENTRY_SIZE;
        prev = e;
        if(ftell(FSFile)/BLOCK_SIZE != curBlock)
            curBlock = nextBlock(curBlock, NO_ALLOC);
        if(!curBlock){
            break;
        }
        e = readEntry();
        if(e.blockPtr == 0){
            break;
        }
    }
    if((lastAddr % BLOCK_SIZE) == 0 && lastAddr != ROOT_ADDR){
        uint16_t emptyBlock = lastAddr/BLOCK_SIZE;
        FAT[emptyBlock] = 0;
        setBMP(emptyBlock, false);
        uint16_t lastBeforeEmpty = eraseAddr/BLOCK_SIZE;
        //Equanto o lastBeforeEmpty apontar para uma entrada valida
        while(FAT[FAT[lastBeforeEmpty]] != 0){
            lastBeforeEmpty = FAT[lastBeforeEmpty];
        }
        FAT[lastBeforeEmpty] = LAST_BLOCK;
    }
    fseek(FSFile, lastAddr, SEEK_SET);
    fwrite(&zeroes, 1, ENTRY_SIZE, FSFile);
    fseek(FSFile, eraseAddr, SEEK_SET);
    fwrite(&prev, ENTRY_SIZE, 1, FSFile);
}


//COMANDOS DO FS
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

void criadir(char *dirName){
    char *dest = NULL;
    pathTo(dirName, &dest, STOP_BEFORE,DIR);
    if(ftell(FSFile) == 0) return;
    writeEntry(dest, 0, DIR);
}

void toca(char *fileName, uint32_t size){
    char *dest = NULL;
    pathTo(fileName, &dest, STOP_BEFORE, DIR);
    if(ftell(FSFile) == 0) return;
    if(entryExists(dest)){
        //atualiza o tempo de acesso
        time_t now = time(NULL);
        fseek(FSFile, 16, SEEK_CUR);
        fwrite(&now, sizeof(uint32_t), 1, FSFile);
    }
    else {
        writeEntry(dest, size, FIL);
    }
}

void copia(char *origem){
    char *parent,*destino = strtok(NULL, " ");
    char path[2048]; strcpy(path, destino);
    FILE *copy = fopen(origem, "rb");
    uint32_t length;
    if(copy == NULL){
        printf("Origem \"%s\" não existe.\n", origem);
        return;
    }

    fseek(copy, 0, SEEK_END);
    length = ftell(copy);
    rewind(copy);
    char *buffer = malloc(length);
    fread(buffer, 1, length, copy);
    fclose(copy);

    pathTo(path, &parent, STOP_BEFORE, DIR);
    if(ftell(FSFile) == 0) return;
    if(!entryExists(parent)){
        strcpy(path, destino);
        toca(path, length);
    }
    else {
        fseek(FSFile, 4,SEEK_CUR);
        fwrite(&length, sizeof(uint32_t), 1, FSFile);
        time_t now = time(NULL);
        fseek(FSFile, 4, SEEK_CUR);
        fwrite(&now, sizeof(uint32_t), 1, FSFile);
        fwrite(&now, sizeof(uint32_t), 1, FSFile);
    }
    strcpy(path, destino);
    pathTo(path, &parent, STOP_AT, FIL);
    if(ftell(FSFile) == 0) return;

    uint16_t i, curBlock = ftell(FSFile)/BLOCK_SIZE;
    uint64_t written = 0;
    
    while(curBlock != 0){
        i = 0;
        char *writeBuf = calloc(BLOCK_SIZE, sizeof(uint8_t));
        while(written < length && i < 4096){
            writeBuf[i++] = buffer[written++];
        }
        fwrite(writeBuf, 1, BLOCK_SIZE, FSFile);
        free(writeBuf);

        if(written < length)
            curBlock = nextBlock(curBlock, ALLOC);
        else
            break;
    }
    free(buffer);
}

void lista(char *dirName){
    char *parent;
    pathTo(dirName, &parent, STOP_AT, DIR);
    uint16_t curBlock = ftell(FSFile)/BLOCK_SIZE;
    Entry e;
    e = readEntry();
    if(e.blockPtr)
        printf("|  Tamanho  |       Criado      |     Modificado    |     Acessado      | Nome\n");
    while(e.blockPtr != 0){
        if(ftell(FSFile)/BLOCK_SIZE != curBlock)
            curBlock = nextBlock(curBlock, NO_ALLOC);
        if(!curBlock){
            return;
        }
        bool ehDir = e.blockPtr & DIR_INDICATOR;
        if(ehDir){
            printf("|        - B");
        }
        else
            printf("|%9d B", e.size);
        printTime(e.createTime);
        printTime(e.modTime);
        printTime(e.accessTime);
        printf("| %s", e.name);
        if(ehDir) printf("/");
        printf("\n");
        e = readEntry();
    }
}

void mostra(char *fileName){
    char *parent;
    pathTo(fileName, &parent, STOP_AT, FIL);
    if(ftell(FSFile) == 0) return;

    uint16_t curBlock = ftell(FSFile)/BLOCK_SIZE;
    while(curBlock != 0){
        char *writeBuf = malloc(BLOCK_SIZE);
        memset(writeBuf, 0, BLOCK_SIZE);
        fread(writeBuf, 1, BLOCK_SIZE, FSFile);
        printf("%s",writeBuf);
        free(writeBuf);
        curBlock = nextBlock(curBlock, NO_ALLOC);
    }
}
void apaga(char *fileName){
    if(strcmp(fileName, "/") == 0) return;
    char path[2048]; strcpy(path, fileName);
    char *parent;
    uint16_t blocks[BLOCK_NUMBER];
    pathTo(path, &parent, STOP_BEFORE, DIR);
    //err arq no exist
    if(!entryExists(parent)) return;

    uint16_t i = 0, curBlock;
    Entry e = readEntry();
    if(e.blockPtr & DIR_INDICATOR) return;
    curBlock = e.blockPtr;
    compacta();
    char *eraseBuf = calloc(BLOCK_SIZE, sizeof(uint8_t));
    while(curBlock != 0){
        blocks[i++] = curBlock;
        curBlock = nextBlock(curBlock, NO_ALLOC);
    }
    for(int j = 0; j < i; j++){
        fseek(FSFile, blocks[j]*BLOCK_SIZE, SEEK_SET);
        fwrite(eraseBuf, 1, BLOCK_SIZE, FSFile);
        FAT[blocks[j]] = 0;
        setBMP(blocks[j], false);
    }
    free(eraseBuf);
}

void apagadir(char *dirName){
    uint16_t i = 0;
    while(dirName[i] != '\0'){
        i++;
    }
    if(dirName[i] != '/'){
        strcat(dirName, "/\0");
    }
    char *parent;
    char path[4096]; strcpy(path, dirName);
    pathTo(path, &parent, STOP_BEFORE, DIR);
    if(!entryExists(parent)){printf("Entry %s does not exist\n", path);return;}
    uint32_t eraseAddr = ftell(FSFile)+ENTRY_SIZE;
    strcpy(path, dirName);
    pathTo(path, &parent, STOP_AT, DIR);
    if(ftell(FSFile) == 0) return;
    uint16_t curBlock = ftell(FSFile)/4096;
    Entry e;
    i = 0;
    uint32_t lastPos;
    while(true){
        e = readEntry();
        fseek(FSFile, -ENTRY_SIZE, SEEK_CUR);
        if(e.blockPtr == 0) break;
        if(e.blockPtr & DIR_INDICATOR){
            strcpy(path, dirName);
            strcat(path, e.name);
            lastPos = ftell(FSFile); 
            apagadir(path);
            fseek(FSFile, lastPos, SEEK_SET);
            printf("Apagou %s\n", path);
        }
        else {
            strcpy(path, dirName);
            lastPos = ftell(FSFile); 
            apaga(strcat(path, e.name));
            fseek(FSFile, lastPos, SEEK_SET);
            printf("Apagou %s\n", path);
        }
    }
    fseek(FSFile, eraseAddr, SEEK_SET);
    compacta();
    char *eraseBuf = calloc(BLOCK_SIZE, sizeof(uint8_t));
    fseek(FSFile, curBlock*BLOCK_SIZE, SEEK_SET);
    fwrite(eraseBuf, 1, BLOCK_SIZE, FSFile);
    FAT[curBlock] = 0;
    setBMP(curBlock, false);
    free(eraseBuf);
}
//PROMPT E LEITURA DO PROMPT 
char *promptUser(){
    char *command = (char *)malloc(sizeof(char) * 1024);
    command = readline("{ep3}: ");
    return command;
}
int handleCommand(char *command){
    char *token = strtok(command, " ");
    if(strcmp(token, "monta") == 0)monta(strtok(NULL, " "));
    else if(strcmp(token, "criadir") == 0)criadir(strtok(NULL, " "));
    else if(strcmp(token, "toca") == 0)toca(strtok(NULL, " "),0);
    else if(strcmp(token, "copia") == 0)copia(strtok(NULL, " "));
    else if(strcmp(token, "lista") == 0)lista(strtok(NULL, " "));
    else if(strcmp(token, "mostra") == 0)mostra(strtok(NULL, " "));
    else if(strcmp(token, "apaga") == 0)apaga(strtok(NULL, " "));
    else if(strcmp(token, "apagadir") == 0){
        char *path = strtok(NULL, " ");
        apagadir(path);
        printf("Apagou %s\n", path);
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
    for(int i = 0; i < 20; i++){
        printf("%d ", FAT[i]);
    }
    printf("\n");
    for(int i = 0; i < 20; i++){
        printf("%02x ", BMP[i]);
    }
    fclose(FSFile);
    free(BMP);
    free(FAT);
    return 0;
}
