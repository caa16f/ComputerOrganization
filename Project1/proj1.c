#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>
#include <unistd.h>
#include <math.h>

//R-Type instructions
struct {
    const char *name;
    char *functionBin;
} rTypes[] = {
        { "add", "100000" },
        { "nor", "100111" },
        { "sll", "000000" },
        { NULL, 0 } };

//I-Type Instructions
struct {
    const char *name;
    char *address;
} iTypes[] = {
        { "lw",   "100011" },
        { "sw",   "101011" },
        { "ori",  "001101" },
        { "lui",  "001111" },
        { "bne",  "000101" },
        { "addi", "001000" },
        { NULL, 0 } };

//J-Type Instructions
struct {
    const char *name;
    char *address;
} jTypes[] = {
        {"j", "000010"},
        { NULL, 0 } };

//Register Numbers
struct{
    char *name;
    char *memAddress;
} registerList[] = {
        { "zero", "00000" },
        { "t0", "01000" },
        { "t1", "01001" },
        { "t2", "01010" },
        { "t3", "01011" },
        { "t4", "01100" },
        { "t5", "01101" },
        { "t6", "01110" },
        { "t7", "01111" },
        { "s0", "10000" },
        { "s1", "10001" },
        { "s2", "10010" },
        { "s3", "10011" },
        { "s4", "10100" },
        { "s5", "10101" },
        { "s6", "10110" },
        { "s7", "10111" },
        { "0" , "00000"},
        { NULL, 0 } };

//Function to return binary address of specified register
char * regAddress(char *reg){
    int i;
    for(i = 0; registerList[i].name != NULL; i++){
        if(strcmp(reg,registerList[i].name) == 0 ){
                return registerList[i].memAddress;
        }
    }
    return NULL;
}

//global used to keep track of how many elements are in the symbol table
int symbolTableCount = 0;

//Symbol table struct
struct symTable {
    char label[25];
    int32_t address;
}symbols[50];


//Function to insert Label and address into symbol table
void insertSym(char labelToInsert[8], int32_t instructionAddy ){
    strcpy(symbols[symbolTableCount].label,labelToInsert);
    symbols[symbolTableCount].address = instructionAddy;
    symbolTableCount++;
}

//Function to get the address of a label
int32_t searchSym(char * test){
    int i = 0;
    int temp = 0;
    for(i = 0; i < symbolTableCount; i++){
        if(strcmp(symbols[i].label,test) == 0){
            return symbols[i].address;
        }
    }

}

//function to determine which instruction type the instruction is. R I OR J
instructionType(char *instruction) {
    if (strcmp(instruction, "add") == 0 || strcmp(instruction, "nor") == 0
        || strcmp(instruction, "sll") == 0) {
        return 'r';
    } else if (strcmp(instruction, "addi") == 0 || strcmp(instruction, "ori") == 0
               || strcmp(instruction, "lui") == 0 || strcmp(instruction, "sw") == 0
               || strcmp(instruction, "lw") == 0 || strcmp(instruction, "bne") == 0
               || strcmp(instruction,"la")==0){                 //I classified la as an I type even though its not really
        return 'i';
    }else if(strcmp(instruction, "j") == 0){
        return 'j';
    }else{
        return 0;
    }
}

//Function to convert string of binary values into integer
int binToDec(char * binaryToChange){
    int result = 0 ;
    int a = 0;
    while( binaryToChange[a] != '\0' )
    {
        if( binaryToChange[a] == '1' )
        {
            result |= 1 ;
        }
        a++ ;
        if( binaryToChange[a] != '\0' )
        {
            result <<= 1 ;
        }
    }
    return result ;
}

//Function to convert integer into string of binary values. IsItJ is used to determine if the target bin is 16 bits
//or 26 bits which are needed for jump commands
char * intToBin(int num, int isItJ) {
    int c, d, count;
    char *pointer;

    count = 0;
    pointer = (char*)malloc(32+1);

    if(isItJ == 1){
        c = 25;
    }else{
        c = 15;
    }
    for (c ; c >= 0 ; c--)
    {
        d = num >> c;

        if (d & 1)
            *(pointer+count) = 1 + '0';
        else
            *(pointer+count) = 0 + '0';

        count++;
    }
    *(pointer+count) = '\0';

    return  pointer;
}

//Function to take R Type fields and return 32 bit binary string
 char * rType(char *instruction, char *rs, char *rt, char *rd, char *shamt){
    //Place holders for the binary values of
    char * rsBinary = "00000";
    char * rtBinary = "00000";
    char * rdBinary = "00000";

    char * funcToDo = NULL;

    if(strcmp(rs, "00000") !=0){
        rsBinary = regAddress(rs);
    }
    if(strcmp(rt, "00000") != 0){
        rtBinary = regAddress(rt);
    }
    if(strcmp(rd, "00000") != 0){
        rdBinary = regAddress(rd);
    }

    int j;
    for(j = 0; rTypes[j].name != NULL; j++ ){
        if(strcmp(instruction, rTypes[j].name) == 0){
            funcToDo = rTypes[j].functionBin;
        }
    }
    char * result = (char *)malloc(500);
    strcpy(result, "");
    strcat(result, "000000");
    strcat(result, rsBinary);
    strcat(result, rtBinary);
    strcat(result, rdBinary);
    strcat(result, shamt);
    strcat(result, funcToDo);
    return result;
}
//Function to take I type fields and return 32 bit binary string
char * iType(char * instruction, char *rs, char *rt, int immediate){
    char * rsBinary = "00000";
    char * rtBinary = "00000";

    if(strcmp(rs, "00000") !=0){
        if(strcmp(rs, "00001") == 0){
            rsBinary = "00001";
        }else{
            rsBinary = regAddress(rs);
        }
    }
    if(strcmp(rt, "00000") != 0){
        if(strcmp(rt, "00001") == 0){
            rtBinary = "00001";
        }else{
            rtBinary = regAddress(rt);
        }
    }
    char *opcode;
    char * immediateHolder;
    int i;
    for( i = 0; iTypes[i].name != NULL; i++){
        if(strcmp(instruction, iTypes[i].name) == 0){
            opcode = iTypes[i].address;
        }
    }
    immediateHolder = intToBin(immediate, 0);

    char * result = (char *)malloc(500);
    strcpy(result, "");
    strcat(result, opcode);
    strcat(result, rsBinary);
    strcat(result, rtBinary);
    strcat(result, immediateHolder);
    return result;
}

//Function to take J type argument fields and return 32 bit binary string
char * jType(char * instruction, int immediate){
    char * opcode = NULL;

    int i;
    for(i = 0; jTypes[i].name != NULL; i++){
        if(strcmp(instruction, jTypes[i]. name) == 0){
            opcode = jTypes[i].address;
        }
    }

    char * result = (char *)malloc(500);
    char * immediateHolder;

    immediateHolder = intToBin(immediate, 1);
    strcat(result, opcode);
    strcat(result, immediateHolder);

}

//Main
int main(int argc, char *argv[]) {
    char lineHolder[100];
    char *tokenPointer = NULL;
    int dataSectionReached = 0;
    int pass = 1;

    int32_t lineNum = 0;
    int32_t instructionAddress = 0x00000000;
    struct symTable SymbolTable;


    int totalTokens = 0;
    char tokArray[1000][256];
    int test1 = 0;

    const char delim[] = " ()\t\n\r\v\f,$";

    while(pass == 1) {
        // Must parse standard input and convert the separate char inputs into readable tokens
        while (fgets(lineHolder, 1000, stdin)) {
          //  printf("%s ", lineHolder);

            //tokenize the string, delimiting at every tab , $ , newline or ','
            tokenPointer = strtok(lineHolder, delim);

            while (tokenPointer != NULL && test1 <= totalTokens) {
                //tokenize the string, delimiting at every tab , $ , newline or ','
                totalTokens++;

                //If first pass then we are looking for labels to add to Symbol Table
                if (pass == 1 && dataSectionReached == 0) {
                    if (strstr(tokenPointer, ":") && dataSectionReached == 0) {

                        //Remove the last character in the token which in this case will always be a :
                        size_t lengthOfToken = strlen(tokenPointer);
                        tokenPointer[lengthOfToken - 1] = '\0';

                        uint32_t instructionTracker = instructionAddress + lineNum * 4;

                        //Insert the found label and address into the symbol table
                        insertSym(tokenPointer, instructionTracker);

                    }else if(instructionType(tokenPointer) == 'i' || instructionType(tokenPointer) == 'r' || instructionType(tokenPointer) == 'j'){
                        instructionAddress += 4;
                    }


                }

                strcpy(tokArray[test1++] , tokenPointer);
                tokenPointer = strtok(NULL, delim);

            }
        }
        break;
    }
        int b = 0;
        int c = 0;

        char *instruction;

    int result1 = 0;
    int result2 = 0;
    int result3 = 0;
    int result4 = 0;
    int result5 = 0;
    int result6 = 0;
    int result7 = 0;

    ///FOR LOOP USED TO LOOP THROUGH LIST OF TOKENS AND PERFORM CALCULATIONS
            for (b ; b < totalTokens+1; b++) {

                instruction  = tokArray[b];


                    if (instructionType(instruction) == 'r') {

                        if (strcmp(tokArray[b], "add") == 0 || strcmp(tokArray[b], "nor") == 0) {

                            char *rd = tokArray[b+1];
                            char *rs = tokArray[b+2];
                            char *rt = tokArray[b+3];
                            //shamt not needed for these operations
                            char *tempShamt = "00000";


                            result1 = binToDec(rType(instruction, rs, rt, rd, tempShamt));

                            printf("0x%08X: ", lineNum);
                            printf("0x%08X\n", result1);
                            lineNum = lineNum + 4;

                        } else if (strcmp(tokArray[b], "sll") == 0) {
                            char *rd = tokArray[b + 1];
                            char *rt = tokArray[b + 2];
                            char * tempShamt = intToBin(atoi(tokArray[b + 3]),0);

                            result2 = binToDec(rType(instruction, "00000", rt, rd, tempShamt));
                            printf("0x%08X: ", lineNum);
                            printf("0x%08X\n", result2);
                            lineNum = lineNum + 4;
                        }
                    } else if (instructionType(instruction) == 'i') {
                        if (strcmp(tokArray[b], "la") == 0) {

                            char *rs = tokArray[b + 1];

                           char *temp = tokArray[b + 2];

                          int32_t address = searchSym(temp);


                            char * addressBin;
                         addressBin = intToBin(address,0);
                        // printf("%s" , addressBin);

                            char upper_bits[16];
                            char lower_bits[16];

                            int q;
                            for(q = 0 ; q < 32; q++){
                              //  printf("%d ", q);
                                if(q < 16 ){
                                    lower_bits[q] = addressBin[q];
                                }else{
                                    upper_bits[q-16]= addressBin[q];
                                }
                            }

                            int immHold = binToDec(upper_bits);
                            result3 = binToDec(iType("lui", "00000", "00001", immHold));
                            printf("0x%08X: ", lineNum);
                            printf("0x%08X\n", result3);

                            lineNum = lineNum + 4;

                            immHold = binToDec(lower_bits);
                            result3 = binToDec(iType("ori",rs,"00001",immHold));
                            printf("0x%08X: ", lineNum);
                            printf("0x%08X\n", result3);

                            lineNum = lineNum + 4;


                        } else if (strcmp(tokArray[b], "lw") == 0 || strcmp(tokArray[b], "sw") == 0) {

                            char *rt = tokArray[b + 1];
                            char *offset = tokArray[b + 2];
                            char *rs = tokArray[b + 3];

                            int immm = atoi(offset);

                            result3 = binToDec(iType(instruction, rs, rt, immm));

                            printf("0x%08X: ", lineNum);
                            printf("0x%08X\n", result3);
                            lineNum = lineNum + 4;

                        } else if (strcmp(tokArray[b], "addi") == 0 || strcmp(tokArray[b], "ori") == 0) {

                            char *rt = tokArray[b + 1];
                            char *rs = tokArray[b + 2];
                            char *offset = tokArray[b + 3];

                            int immm = atoi(offset);


                            result4 = binToDec(iType(instruction, rs, rt, immm));
                            printf("0x%08X: ", lineNum);
                            printf("0x%08X\n", result4);
                            lineNum = lineNum + 4;


                        }
                        else if (strcmp(tokArray[b], "lui") == 0) {
                            char *rt = tokArray[b + 1];
                            int immediate = atoi(tokArray[b + 2]);

                            result5 = binToDec(iType("lui","00000",rt,immediate));

                            printf("0x%08X: ", lineNum);
                            printf("0x%08X\n", result5);
                            lineNum = lineNum + 4;
                        } else if (strcmp(tokArray[b], "bne") == 0) {
                            char* rs = tokArray[b+1];
                            char* rt = tokArray[b+2];
                            char* label = tokArray[b+3];

                            int32_t address = searchSym(label);
                            int immediate = address + lineNum;
                            result6 = binToDec(iType("bne", rs ,rt, immediate));
                            printf("0x%08X: ", lineNum);
                            printf("0x%08X\n", result6);

                            lineNum = lineNum + 4;
                        }
                    } else if(instructionType(instruction) == 'j'){
                                    char * label = tokArray[b+1];
                                    int32_t address = searchSym(label);

                                    result7 = binToDec(jType("j",address));


                                    printf("0x%08X: ", lineNum);
                                    printf("0x%08X\n", result6);
                                    lineNum = lineNum + 4;
                    }
                }

    return 0;
}