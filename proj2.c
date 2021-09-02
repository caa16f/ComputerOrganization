#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>


const int sizeOfBits = 32;
char line[40] = {0};
int memLocation = 0;
int cycleCounter = 1;
int registers[32] = {0};
int dataMem[32] = {0};
int totalStalls = 0;
int totalBranches = 0;

int forwardA = 00;
int forwardB = 00;

bool branchFlush = false;

struct Instruction{
    char opcode[7];

    //One char array to read in
    char rs[6];
    char rt[6];
    char rd[6];

    //One integer array to manipulate
    int Irs;
    int Irt;
    int Ird;

    char immediate[16];
    int intImmediate;
    char target[27];
    char func[7];
    char shiftAmount[5];
    char instructionText[6];
    char wholeInstruction[30];
    char binaryString[33];
    int typeOfInstruct;    // 1 - R . 2 - I . 3 - J

    int memLocation;
    bool NOOP;
    bool shouldStall;
    bool halt;

    int PC;
    int flush;

    bool doBranch;

    struct Instruction* next;
}instruction, *start, *current, *IF, *ID, *EX, *MEM, *WB, *stall;

//Register Numbers
struct{
    char *name;
    char *memAddress;
} registerList[] = {
        { "0", "00000" },
        { "at", "00001"},
        { "v0", "00010"},
        { "v1", "00011"},
        { "a1", "00100"},
        { "a2", "00101"},
        { "a3", "00110"},
        { "a4", "00111"},
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
        { "t8", "11000" },
        { "t9", "11001" },
        { "k0", "11010" },
        { "k1", "11011" },
        { "gp", "11100" },
        { "sp", "11101" },
        { "fp", "11110" },
        { "ra", "11111" }};


struct IF_ID{
    int PCplus4;
    bool stall;
}IF_ID;

struct ID_EX{
    int PCplus4;
    int branchTarget;
    int readData1;
    int readData2;
    int immed;
    char rs[6];
    char rt[6];
    char rd[6];
    int typeOfInstruction;
    bool stall;
    int ALU_Result;
}ID_EX;

struct EX_MEM{
    int ALUinput1;
    int ALUinput2;
    int ALU_Result;
    int writeData;
    char writeRegister[5];
    bool stall;
}EX_MEM;

struct MEM_WB{

    int writeDataFromMem;
    int writeDataFromALU;
    char writeRegister[5];
    bool stall;
}MEM_WB;

struct branch_Prediction{
    int PC;
    int branchTarget;
    int state;
}branch_prediction[100];

//Function to return register of specified binary address
char * regAddress(char * address){

    int i;
    for(i = 0; registerList[i].name != NULL; i++){
        if(strcmp(address,registerList[i].memAddress) == 0 ){
            return registerList[i].name;
        }
    }
    return NULL;
}

int power(int base, int exp){
    int result = 1;
    for (;;)
    {
        if (exp & 1)
            result *= base;
        exp >>= 1;
        if (!exp)
            break;
        base *= base;
    }

    return result;
}

//takes binary char name[] and converts it to int
int bin2Dec(char name[], int length){
    int i = 0;
    int j = 0;
    int temp = 0;
    for (i = length; i > 0; i--){
        if (name[i-1] == '1')
            temp += (i==1?-1:1) * power(2,j);
        j++;
    }
    return temp;
}
//Creates array of binary digits
char *decimal_to_binary(int n)
{
    int c, d, count;
    char *pointer;

    count = 0;
    pointer = (char*)malloc(32+1);

    if (pointer == NULL)
        exit(EXIT_FAILURE);

    for (c = 31 ; c >= 0 ; c--)
    {
        d = n >> c;

        if (d & 1)
            *(pointer+count) = 1 + '0';
        else
            *(pointer+count) = 0 + '0';

        count++;
    }
    *(pointer+count) = '\0';

    return  pointer;
}
int ALUop(char * instruction, int line1, int line2){
    if(!strcmp(instruction, "add")){
        return line1 + line2;
    }else if(!strcmp(instruction, "sub")){
        return line1 - line2;
    }else if(!strcmp(instruction, "sll")){
        return line1 << line2;
    }else {
        return 0;
    }
}
//Print out dataMem and Registers
void printOutUpdatedState(){

    fprintf(stdout, "************************\n");
    printf("State at the beginning of cycle %d\n", cycleCounter);
    printf("\tPC = %d\n", current->memLocation+4);

    fprintf(stdout, "\tData Memory:\n");
    int w;
    for (w = 0; w < 32; w++){
        fprintf(stdout, "\t\tdataMem[%d] = %d",w,dataMem[w]);
        ((w+1)%2==0) ? fprintf(stdout,"\n") : fprintf(stdout,"\t"); //print \n every 2
    }

    fprintf(stdout, "\tRegisters:\n");
    int u;
    for (u = 0; u < 32; u++){
        fprintf(stdout, "\t\tregFile[%d] = %d",u,registers[u]);
        ((u+1)%2==0) ? fprintf(stdout,"\n") : fprintf(stdout,"\t"); //print \n every 2
    }
}

void flush_IF_ID(){
    IF_ID.PCplus4 = 0;
    IF_ID.stall = false;
}
void flush_ID_EX(){
    ID_EX.stall = false;
    ID_EX.PCplus4 = 0;
    ID_EX.ALU_Result = 0;
    strcpy(ID_EX.rs , " ");
    strcpy(ID_EX.rt , " ");
    strcpy(ID_EX.rd , " ");
    ID_EX.immed = 0;
    ID_EX.readData1 = 0;
    ID_EX.readData2 = 0;
    ID_EX.branchTarget = 0;
    ID_EX.typeOfInstruction = 0;
}
void flush_EX_MEM(){
    EX_MEM.ALU_Result = 0;
    EX_MEM.stall = false;
    EX_MEM.ALUinput1 = 0;
    EX_MEM.ALUinput2 = 0;
    strcpy(EX_MEM.writeRegister , " ");
    EX_MEM.writeData = 0;
}
void flush_MEM_WB(){
    strcpy(MEM_WB.writeRegister , " ");
    MEM_WB.writeDataFromMem = 0;
    MEM_WB.writeDataFromALU = 0;
    MEM_WB.stall = false;
};              //Not used but i wrote it anyways

//Populate current instruction. This function is called everytime a new line is read from input.
//Everytime it runs it extracts all needed elements and places them into current. Current is the current
//Instruction we are on. After wards it complies its assembly instruction then increments a new current.
void buildInstruction() {
    current->halt = false;

    strncpy(current->binaryString, decimal_to_binary(atoi(line)), 32);
    current->binaryString[32] = '\0';



    if(!strcmp(current->binaryString, "00000000000000000000000000000000")){
        strcpy(current->instructionText, "NOOP");
        current->NOOP = true;
    }

    int i = 0;
    for (i = 0; i < 6; i++)
        current->opcode[i] = current->binaryString[i];

    if (!strcmp(current->opcode, "000000")) {
        //R-Type Instructions
        current->typeOfInstruct = 1;
        //get rs bits 7-11
        for (i = 6; i < 11; i++)
            current->rs[i - 6] = current->binaryString[i];
        //get rs bits 13-16
        for (i = 11; i < 16; i++)
            current->rt[i - 11] = current->binaryString[i];
        //get rs bits 16-21
        for (i = 16; i < 21; i++)
            current->rd[i - 16] = current->binaryString[i];
        //get rs bits 21-26
        for (i = 21; i < 26; i++)
            current->shiftAmount[i - 21] = current->binaryString[i];
        //get rs bits 26-32
        for (i = 26; i < 32; i++)
            current->func[i - 26] = current->binaryString[i];
        for (i = 16; i < 32; i++)
            current->immediate[i - 16] = current->binaryString[i];

        current->immediate[16] = '\0';

        if (!strcmp(current->func, "100000"))
            strcpy(current->instructionText, "add");
        else if (!strcmp(current->func, "100010"))
            strcpy(current->instructionText, "sub");
        else if (!strcmp(current->func, "000000") && strcmp(current->instructionText, "NOOP"))
            strcpy(current->instructionText, "sll");

        if(bin2Dec(current->immediate,16) == 1){
            strcpy(current->instructionText, "HALT");

        } else if (!strcmp(current->rd, "00000") &&
            !strcmp(current->rt, "00000") &&
            !strcmp(current->shiftAmount, "00000")) {
            strcpy(current->instructionText, "NOOP");
            current->NOOP = true;
        }


        current->Irs = abs(bin2Dec(current->rs, 5));
        current->Irt = abs(bin2Dec(current->rt, 5));
        current->Ird = abs(bin2Dec(current->rd, 5));
        current->intImmediate = bin2Dec(current->immediate, 16);

    } else {
        current->typeOfInstruct = 2;

        //Check for SW
        if (!strcmp(current->opcode, "101011")) {
            for (i = 6; i < 11; i++)
                current->rs[i - 6] = current->binaryString[i];
            for (i = 11; i < 16; i++)
                current->rt[i - 11] = current->binaryString[i];
            for (i = 16; i < 32; i++)
                current->immediate[i - 16] = current->binaryString[i];
            strcpy(current->instructionText, "sw");
        }

        //Check for lw
        if (!strcmp(current->opcode, "100011")) {
            for (i = 6; i < 11; i++)
                current->rs[i - 6] = current->binaryString[i];
            for (i = 11; i < 16; i++)
                current->rt[i - 11] = current->binaryString[i];
            for (i = 16; i < 32; i++)
                current->immediate[i - 16] = current->binaryString[i];
            strcpy(current->instructionText, "lw");
        }

        //Check for BNE
        if (!strcmp(current->opcode, "000101")) {
            for (i = 6; i < 11; i++)
                current->rs[i - 6] = current->binaryString[i];
            for (i = 11; i < 16; i++)
                current->rt[i - 11] = current->binaryString[i];
            for (i = 16; i < 32; i++)
                current->immediate[i - 16] = current->binaryString[i];
            strcpy(current->instructionText, "bne");


        }

        //Check for ori
        if (!strcmp(current->opcode, "001101")) {
            for (i = 6; i < 11; i++)
                current->rs[i - 6] = current->binaryString[i];
            for (i = 11; i < 16; i++)
                current->rt[i - 11] = current->binaryString[i];
            for (i = 16; i < 32; i++)
                current->immediate[i - 16] = current->binaryString[i];
            strcpy(current->instructionText, "ori");
        }


        //Check for andi
        if (!strcmp(current->opcode, "001100")) {
            for (i = 6; i < 11; i++)
                current->rs[i - 6] = current->binaryString[i];
            for (i = 11; i < 16; i++)
                current->rt[i - 11] = current->binaryString[i];
            for (i = 16; i < 32; i++)
                current->immediate[i - 16] = current->binaryString[i];
            strcpy(current->instructionText, "andi");
        }

        current->Irs = bin2Dec(current->rs, 5);
        current->Irt = bin2Dec(current->rt, 5);
        current->intImmediate = bin2Dec(current->immediate, 16);


    }

    //BEGIN MAKING INSTRUCTION STRING FOR OUTPUT
    if (!strcmp(current->opcode, "000000")){
        if (strcmp(current->instructionText, "HALT") && strcmp(current->instructionText, "NOOP")){
            if (!strcmp(current->instructionText, "sub" )|| !strcmp(current->instructionText, "add" )){
                sprintf( current->wholeInstruction ,"%s,$%s,$%s,$%s\n" , current->instructionText, regAddress(current->rd), regAddress(current->rs), regAddress(current->rt));
            }else if(!strcmp(current->instructionText, "sll" )){
                sprintf( current->wholeInstruction ,"%s,$%s,$%s,%d\n" , current->instructionText, regAddress(current->rd), regAddress(current->rt), bin2Dec(current->shiftAmount,5 ));
                }else{
                sprintf(current->wholeInstruction, "%s", current->instructionText);
                }

            }else if(!strcmp(current->instructionText, "HALT" )){
            sprintf(current->wholeInstruction, "%s", current->instructionText);
        }
        }else if(!strcmp(current->instructionText , "lw") || !strcmp(current->instructionText , "sw")){
        sprintf(current->wholeInstruction, "%s, $%s, %d($%s)",current->instructionText,regAddress(current->rt),current->intImmediate, regAddress(current->rs));
    } else if(!strcmp(current->instructionText , "ori") || !strcmp(current->instructionText , "andi")){
        sprintf(current->wholeInstruction, "%s,$%s,$%s,#%d",current->instructionText, regAddress(current->rt), regAddress(current->rs), current->intImmediate);
    } else if(!strcmp(current->instructionText , "bne")){
        sprintf(current->wholeInstruction, "%s,$%s,$%s,#%d",current->instructionText,regAddress(current->rs), regAddress(current->rt), current->intImmediate);
    }else if(!strcmp(current->instructionText, "HALT") ){
        sprintf(current->wholeInstruction, "%s", current->instructionText);
    }


}

//PIPELINE FUNCTIONS STARTED
void _IF_ID_FUNC(){
    current = start;
    while(current->memLocation != IF_ID.PCplus4){
        current = current->next;
    }

    IF = current;
    IF_ID.PCplus4 += 4;

    if(!strcmp(EX->instructionText, "bne")){
        IF->shouldStall = true;
        totalBranches++;
        totalStalls++;
    }
    if(!strcmp(EX->instructionText, "lw") && (EX->Irt == current->Irs ||EX->Irt == current->Irt )){
        IF->shouldStall = true;
        totalStalls++;
    }
    if(!strcmp(IF->immediate, "0000000000000001")){
        IF->halt = true;
    }
}

void _ID_EX_FUNC(){
    int RStemp = 0;
    int RTvalue = 0;
    int RDvalue = 0;

    if(IF->shouldStall){
        EX = stall;
        strcpy(EX->wholeInstruction , "NOOP");
        EX->NOOP = true;
        return;
    }else{
        EX = IF;
        ID_EX.PCplus4 += 4;
    }
    if(!strcmp(EX->instructionText, "HALT")){
        IF->NOOP = ID->NOOP = false;
    }
    //If I Type
    int i;
    if(EX->typeOfInstruct == 2){
        if(!strcmp(EX->instructionText, "ori")){
            ID_EX.branchTarget = (4 * EX->intImmediate) + ID_EX.PCplus4 ;
            ID_EX.immed = EX->intImmediate;
            for(i = 0; i <6; i++){
                ID_EX.rs[i] = EX->rs[i];
                ID_EX.rt[i] = EX->rt[i];
            }

            //Check for EX Hazards
            if(EX_MEM.writeRegister != "000000" && !strcmp(EX_MEM.writeRegister , ID_EX.rt)){
                forwardA = 10;
            }
            //Check for EX Hazards
            if(EX_MEM.writeRegister != "000000" && !strcmp(EX_MEM.writeRegister , ID_EX.rs)){

                forwardB = 10;
            }
            //Check for MEM Hazards
            if(MEM_WB.writeRegister != "000000" &&
                MEM_WB.writeRegister == ID_EX.rs &&
                !(EX_MEM.writeRegister != "000000" &&
                !strcmp(EX_MEM.writeRegister, ID_EX.rs))){
                forwardA = 01;
            }
            if(MEM_WB.writeRegister != "000000" &&
               MEM_WB.writeRegister == ID_EX.rt &&
               !(EX_MEM.writeRegister != "000000" &&
                 !strcmp(EX_MEM.writeRegister, ID_EX.rt))){
                forwardA = 01;
            }
        }else if(!strcmp(EX->instructionText, "lw")){
            ID_EX.branchTarget = (4 * EX->intImmediate) + ID_EX.PCplus4;
            ID_EX.immed = EX->intImmediate;
            ID_EX.ALU_Result = registers[abs((bin2Dec(EX->rs , 5)))] + EX->intImmediate ; //lw
            for(i = 0; i < 6; i++){
                ID_EX.rs[i] = EX->rs[i];
                ID_EX.rt[i] = EX->rt[i];
            }
            //Check for EX Hazards
            if(EX_MEM.writeRegister != "000000" && !strcmp(EX_MEM.writeRegister , ID_EX.rt)){
                forwardA = 10;
            }
            //Check for EX Hazards
            if(EX_MEM.writeRegister != "000000" && !strcmp(EX_MEM.writeRegister , ID_EX.rs)){
                forwardB = 10;
            }

            //Check for MEM Hazards
            if(MEM_WB.writeRegister != "000000" &&
               MEM_WB.writeRegister == ID_EX.rs &&
               !(EX_MEM.writeRegister != "000000" &&
                 !strcmp(EX_MEM.writeRegister, ID_EX.rs))){
                forwardA = 01;
            }
            if(MEM_WB.writeRegister != "000000" &&
               MEM_WB.writeRegister == ID_EX.rt &&
               !(EX_MEM.writeRegister != "000000" &&
                 !strcmp(EX_MEM.writeRegister, ID_EX.rt))){
                forwardA = 01;
            }
        }else if(!strcmp(EX->instructionText, "sw")) {
            ID_EX.branchTarget = (4 * EX->intImmediate) + ID_EX.PCplus4;
            ID_EX.immed = EX->intImmediate;
            ID_EX.ALU_Result = registers[abs((bin2Dec(EX->rs , 5)))] + EX->intImmediate ;//sw
            ID_EX.readData1 =  registers[abs(bin2Dec(EX->rs, 5))];

            for(i = 0; i < 6; i++){
                ID_EX.rs[i] = EX->rs[i];
                ID_EX.rt[i] = EX->rt[i];
            }
            //Check for EX Hazards
            if(EX_MEM.writeRegister != "000000" && !strcmp(EX_MEM.writeRegister , ID_EX.rt)){
                forwardA = 10;
            }
            //Check for EX Hazards
            if(EX_MEM.writeRegister != "000000" && !strcmp(EX_MEM.writeRegister , ID_EX.rs)){
                forwardB = 10;
            }
            //Check for MEM Hazards
            if(MEM_WB.writeRegister != "000000" &&
               MEM_WB.writeRegister == ID_EX.rs &&
               !(EX_MEM.writeRegister != "000000" &&
                 !strcmp(EX_MEM.writeRegister, ID_EX.rs))){
                forwardA = 01;
            }
            if(MEM_WB.writeRegister != "000000" &&
               MEM_WB.writeRegister == ID_EX.rt &&
               !(EX_MEM.writeRegister != "000000" &&
                 !strcmp(EX_MEM.writeRegister, ID_EX.rt))){
                forwardA = 01;
            }
        }else if(!strcmp(EX->instructionText, "andi")) {

            ID_EX.branchTarget = (4 * EX->intImmediate) + ID_EX.PCplus4;
            ID_EX.immed = EX->intImmediate;
            ID_EX.ALU_Result = registers[abs((bin2Dec(EX->rs , 5)))] + EX->intImmediate ;//andi

            for(i = 0; i < 6; i++){
                ID_EX.rs[i] = EX->rs[i];
                ID_EX.rt[i] = EX->rt[i];
            }
            //Check for EX Hazards
            if(EX_MEM.writeRegister != "000000" && !strcmp(EX_MEM.writeRegister , ID_EX.rt)){
                forwardA = 10;
            }
            //Check for EX Hazards
            if(EX_MEM.writeRegister != "000000" && !strcmp(EX_MEM.writeRegister , ID_EX.rs)){
                forwardB = 10;
            }
            //Check for MEM Hazards
            if(MEM_WB.writeRegister != "000000" &&
               MEM_WB.writeRegister == ID_EX.rs &&
               !(EX_MEM.writeRegister != "000000" &&
                 !strcmp(EX_MEM.writeRegister, ID_EX.rs))){
                forwardA = 01;
            }
            if(MEM_WB.writeRegister != "000000" &&
               MEM_WB.writeRegister == ID_EX.rt &&
               !(EX_MEM.writeRegister != "000000" &&
                 !strcmp(EX_MEM.writeRegister, ID_EX.rt))){
                forwardA = 01;
            }
        }else if(!strcmp(EX->instructionText, "bne")) {
            ID_EX.branchTarget = (4 * EX->intImmediate) + ID_EX.PCplus4;
            ID_EX.immed = EX->intImmediate;
            ID_EX.ALU_Result = ID_EX.PCplus4; //bne

            //check for correct branch
            if( registers[EX->Irs] != registers[EX->Irt] ){
                ID_EX.ALU_Result = ID_EX.PCplus4 + EX->intImmediate*4;
                EX->doBranch = false;
            }else{
                EX->doBranch = true;
            }

            if(EX->doBranch == true){
                EX->flush = EX->intImmediate*4 + ID_EX.PCplus4 -12;
            }else{
                branchFlush = true;
            }

            for(i = 0; i < 6; i++){
                ID_EX.rs[i] = EX->rs[i];
                ID_EX.rt[i] = EX->rt[i];
            }
            //Check for EX Hazards
            if(EX_MEM.writeRegister != "000000" && !strcmp(EX_MEM.writeRegister , ID_EX.rt)){
                forwardA = 10;
            }
            //Check for EX Hazards
            if(EX_MEM.writeRegister != "000000" && !strcmp(EX_MEM.writeRegister , ID_EX.rs)){
                forwardB = 10;
            }
            //Check for MEM Hazards
            if(MEM_WB.writeRegister != "000000" &&
               MEM_WB.writeRegister == ID_EX.rs &&
               !(EX_MEM.writeRegister != "000000" &&
                 !strcmp(EX_MEM.writeRegister, ID_EX.rs))){
                forwardA = 01;
            }
            if(MEM_WB.writeRegister != "000000" &&
               MEM_WB.writeRegister == ID_EX.rt &&
               !(EX_MEM.writeRegister != "000000" &&
                 !strcmp(EX_MEM.writeRegister, ID_EX.rt))){
                forwardA = 01;
            }

        }
        //R Type
    }else if(EX->typeOfInstruct == 1){
        if(!strcmp(EX->instructionText, "add")){
            ID_EX.readData1 =  registers[bin2Dec(regAddress(EX->rs) , 5)];
            ID_EX.readData2 =  registers[bin2Dec(regAddress(EX->rt) , 5)];
            ID_EX.branchTarget = (4 * EX->intImmediate) + ID_EX.PCplus4;
            ID_EX.immed = EX->intImmediate;
            ID_EX.ALU_Result = (registers[bin2Dec(EX->rs , 5)] + registers[bin2Dec(EX->rt , 5)]);//add
            for(i = 0; i < 6; i++){
                ID_EX.rs[i] = EX->rs[i];
                ID_EX.rt[i] = EX->rt[i];
                ID_EX.rd[i] = EX->rd[i];
            }
            //Check for EX Hazards
            if(EX_MEM.writeRegister != "000000" && !strcmp(EX_MEM.writeRegister , ID_EX.rt)){
                forwardA = 10;
            }
            //Check for EX Hazards
            if(EX_MEM.writeRegister != "000000" && !strcmp(EX_MEM.writeRegister , ID_EX.rs)){
                forwardB = 10;
            }
            //Check for MEM Hazards
            if(MEM_WB.writeRegister != "000000" &&
               MEM_WB.writeRegister == ID_EX.rs &&
               !(EX_MEM.writeRegister != "000000" &&
                 !strcmp(EX_MEM.writeRegister, ID_EX.rs))){
                forwardA = 01;
            }
            if(MEM_WB.writeRegister != "000000" &&
               MEM_WB.writeRegister == ID_EX.rt &&
               !(EX_MEM.writeRegister != "000000" &&
                 !strcmp(EX_MEM.writeRegister, ID_EX.rt))){
                forwardA = 01;
            }
        }else if(!strcmp(EX->instructionText, "sub")){
            ID_EX.ALU_Result = (registers[bin2Dec(EX->rs , 5)] - registers[bin2Dec(EX->rt , 5)]);//sub
            ID_EX.readData1 =  registers[bin2Dec(regAddress(EX->rs) , 5)];
            ID_EX.readData2 =  registers[bin2Dec(regAddress(EX->rt) , 5)];
            ID_EX.branchTarget = (4 * EX->intImmediate) + ID_EX.PCplus4;
            ID_EX.immed = EX->intImmediate;
            for(i = 0; i < 6; i++){
                ID_EX.rs[i] = EX->rs[i];
                ID_EX.rt[i] = EX->rt[i];
                ID_EX.rd[i] = EX->rd[i];
            }
            //Check for EX Hazards
            if(EX_MEM.writeRegister != "000000" && !strcmp(EX_MEM.writeRegister , ID_EX.rt)){
                forwardA = 10;
            }
            //Check for EX Hazards
            if(EX_MEM.writeRegister != "000000" && !strcmp(EX_MEM.writeRegister , ID_EX.rs)){
                forwardB = 10;
            }
            //Check for MEM Hazards
            if(MEM_WB.writeRegister != "000000" &&
               MEM_WB.writeRegister == ID_EX.rs &&
               !(EX_MEM.writeRegister != "000000" &&
                 !strcmp(EX_MEM.writeRegister, ID_EX.rs))){
                forwardA = 01;
            }
            if(MEM_WB.writeRegister != "000000" &&
               MEM_WB.writeRegister == ID_EX.rt &&
               !(EX_MEM.writeRegister != "000000" &&
                 !strcmp(EX_MEM.writeRegister, ID_EX.rt))){
                forwardA = 01;
            }
        }else if(!strcmp(EX->instructionText, "sll")){
            ID_EX.ALU_Result = (registers[bin2Dec(EX->rs , 5)] << registers[bin2Dec(EX->rt , 5)]);//sll
            ID_EX.readData1 =  registers[bin2Dec(regAddress(EX->rs) , 5)];
            ID_EX.readData2 =  registers[bin2Dec(regAddress(EX->rt) , 5)];
            ID_EX.branchTarget = (4 * EX->intImmediate) + ID_EX.PCplus4;
            ID_EX.immed = EX->intImmediate;
            for(i = 0; i < 6; i++){
                ID_EX.rs[i] = EX->rs[i];
                ID_EX.rt[i] = EX->rt[i];
                ID_EX.rd[i] = EX->rd[i];
            }
            //Check for EX Hazards
            if(EX_MEM.writeRegister != "000000" && !strcmp(EX_MEM.writeRegister , ID_EX.rt)){
                forwardA = 10;
            }
            //Check for EX Hazards
            if(EX_MEM.writeRegister != "000000" && !strcmp(EX_MEM.writeRegister , ID_EX.rs)){
                forwardB = 10;
            }
            //Check for MEM Hazards
            if(MEM_WB.writeRegister != "000000" &&
               MEM_WB.writeRegister == ID_EX.rs &&
               !(EX_MEM.writeRegister != "000000" &&
                 !strcmp(EX_MEM.writeRegister, ID_EX.rs))){
                forwardA = 01;
            }
            if(MEM_WB.writeRegister != "000000" &&
               MEM_WB.writeRegister == ID_EX.rt &&
               !(EX_MEM.writeRegister != "000000" &&
                 !strcmp(EX_MEM.writeRegister, ID_EX.rt))){
                forwardA = 01;
            }
        }
    }
}

void _EX_MEM_FUNC(){
    MEM = EX;

    if(MEM->shouldStall){
        return;
    }
    int i;
    if(MEM->typeOfInstruct == 1){
        if(!strcmp(MEM->instructionText, "add")){

            EX_MEM.ALU_Result = (registers[bin2Dec(MEM->rs , 5)] + registers[bin2Dec(MEM->rt , 5)]);
            EX_MEM.writeData = registers[abs((bin2Dec(MEM->rt , 5)))];
            for(i = 0; i < 6; i++)
                EX_MEM.writeRegister[i] = MEM->rd[i];
            return;
        }else if(!strcmp(MEM->instructionText, "sub")){
            EX_MEM.ALU_Result = registers[bin2Dec(MEM->rs , 6)] - registers[bin2Dec(regAddress(MEM->rt) , 6)];
            EX_MEM.writeData = registers[abs((bin2Dec(MEM->rt , 5)))];
            for(i = 0; i < 6; i++)
                EX_MEM.writeRegister[i] = MEM->rd[i];
            return;
        }else if(!strcmp(EX->instructionText, "sll")){
            EX_MEM.ALU_Result = registers[bin2Dec(MEM->rs , 6)] << registers[bin2Dec(regAddress(MEM->rt) , 6)];
            EX_MEM.writeData = registers[abs((bin2Dec(MEM->rt , 5)))];
            for(i = 0; i < 6; i++)
                EX_MEM.writeRegister[i] = MEM->rd[i];
            return;
        }
    }else if(MEM->typeOfInstruct == 2){
        if(!strcmp(MEM->instructionText, "lw")) {
            EX_MEM.ALU_Result = registers[abs((bin2Dec(MEM->rs , 5)))] + MEM->intImmediate ;
            EX_MEM.writeData = 0;
            for(i = 0; i < 6; i++)
                EX_MEM.writeRegister[i] = MEM->rt[i];
            return;
        }else if (!strcmp(MEM->instructionText, "sw")){
            EX_MEM.ALU_Result = registers[abs((bin2Dec(MEM->rs , 5)))] + MEM->intImmediate ;
            EX_MEM.writeData = registers[abs((bin2Dec(MEM->rt , 5)))];
            for(i = 0; i < 6; i++)
                EX_MEM.writeRegister[i] = MEM->rt[i];
            //Now lets store it in data memory
            dataMem[abs(MEM->intImmediate + MEM->Irs + memLocation/4 + 1)] = registers[MEM->Irt];
            return;
        }else if(!strcmp(MEM->instructionText, "ori") || !strcmp(MEM->instructionText, "andi")){
            EX_MEM.ALU_Result = registers[abs((bin2Dec(MEM->rs , 5)))] + MEM->intImmediate ;
            EX_MEM.writeData = 0;
            for(i = 0; i < 6; i++)
                EX_MEM.writeRegister[i] = MEM->rt[i];
            return;
        }else if(!strcmp(MEM->instructionText, "HALT") ){
            EX_MEM.ALU_Result =0;
            EX_MEM.writeData = 0;
            MEM->NOOP = true;
            int i;
            for(i = 0; i < 6; i++)
                EX_MEM.writeRegister[i] = ' ';

        }else if(!strcmp(MEM->instructionText, "bne")){
            EX_MEM.ALU_Result =0;
            EX_MEM.writeData = 0;

            if(branchFlush){
                flush_IF_ID();
                flush_ID_EX();
                flush_EX_MEM();
                branchFlush = false;
                printf("BRANCH FLUSH EXECUTED");

            }
        }
    }

    //FORWARDING UNIT
    if(forwardA == 00 ){
        EX_MEM.ALUinput1 = registers[abs((bin2Dec(EX->rs , 5)))];
    }else if(forwardA == 10){
        EX_MEM.ALUinput1 = EX_MEM.ALU_Result;
    }else if(forwardA == 01){
        EX_MEM.ALUinput1 = MEM_WB.writeDataFromALU;
    }else{
        printf("PLease so help me god no errors");
    }

    if(forwardB == 00 ){
        EX_MEM.ALUinput2 = registers[abs((bin2Dec(EX->rt , 5)))];
    }else if(forwardB == 10){
        EX_MEM.ALUinput2 = EX_MEM.ALU_Result;
    }else if(forwardB == 01){
        EX_MEM.ALUinput2 = MEM_WB.writeDataFromALU;
    }else{
        printf("PLease so help me god no errors");
    }

    EX_MEM.ALU_Result = ALUop(MEM->instructionText,  EX_MEM.ALUinput1,   EX_MEM.ALUinput2);
}

void _MEM_WB_FUNC(){

    int i;
    WB = MEM;

    if (!strcmp(WB->instructionText, "ori" ) || !strcmp(WB->instructionText, "andi") ){
        MEM_WB.writeDataFromMem =0;
        MEM_WB.writeDataFromALU =  WB->intImmediate;

        registers[abs(WB->Irt)] =  WB->intImmediate ;

        for(i = 0; i < 6; i++)
            MEM_WB.writeRegister[i] = WB->rt[i];
        return;
    }else if(!strcmp(WB->instructionText, "lw")){
        MEM_WB.writeDataFromMem = dataMem[abs(WB->intImmediate)/4];
        MEM_WB.writeDataFromALU = registers[abs((bin2Dec(WB->rs , 5)))] + WB->intImmediate ;
        for(i = 0; i < 6; i++)
            MEM_WB.writeRegister[i] = WB->rt[i];

        registers[abs((bin2Dec(MEM_WB.writeRegister , 5)))] =   MEM_WB.writeDataFromMem;

        return;
    }else if(!strcmp(WB->instructionText, "sw")){
        MEM_WB.writeDataFromMem =0;
        MEM_WB.writeDataFromALU = registers[abs((bin2Dec(WB->rs , 5)))] + WB->intImmediate ;
        for(i = 0; i < 6; i++)
            MEM_WB.writeRegister[i] = WB->rt[i];

    }else if(!strcmp(WB->instructionText, "add") || !strcmp(WB->instructionText, "sub") || !strcmp(WB->instructionText, "sll")){
        MEM_WB.writeDataFromMem =0;
        MEM_WB.writeDataFromALU = (registers[bin2Dec(WB->rs , 5)] + registers[bin2Dec(WB->rt , 5)]);
        for(i = 0; i < 6; i++)
            MEM_WB.writeRegister[i] = WB->rd[i];

        registers[abs((bin2Dec(MEM_WB.writeRegister , 5)))] =   MEM_WB.writeDataFromALU;
    }else if(!strcmp(WB->instructionText, "HALT") ){
        MEM_WB.writeDataFromMem =0;
        MEM_WB.writeDataFromALU = 0;
        int i;
        for(i = 0; i < 6; i++)
            MEM_WB.writeRegister[i] = ' ';
    }
}

void printCycle(){
    fprintf(stdout, "\n\tIF/ID: \n");
    if(!IF->NOOP && !IF->halt && !IF_ID.stall){
        printf("\t\tInstruction: \t%s\n" , IF->wholeInstruction);
        printf("\t\tPCPlus4: %d\n", IF_ID.PCplus4 );

    }else if (IF->NOOP && !IF->halt  ){

        printf("\t\tInstruction: \tNOOP\n");
        printf("\t\tPCPlus4: %d\n", IF_ID.PCplus4 );

    }else if( IF->halt && !IF_ID.stall){
        printf("\t\tInstruction: \tHALT\n");
        printf("\t\tPCPlus4: %d\n", IF_ID.PCplus4 );
    }
    fprintf(stdout, "\n\tID/EX: \n");
    if(!EX->NOOP && !EX->halt){
        printf("\t\tInstruction: \t%s\n" , EX->wholeInstruction);
        printf("\t\tPCPlus4: %d\n", ID_EX.PCplus4 );
        printf("\t\tbranchTarget: %d\n", ID_EX.branchTarget);
        printf("\t\treadData1: %d\n", ID_EX.readData1);
        printf("\t\treadData2: %d\n", ID_EX.readData2);
        printf("\t\timmed: %d\n", ID_EX.immed);
        printf("\t\trs: %s\n", regAddress(ID_EX.rs));
        printf("\t\trt: %s\n", regAddress(ID_EX.rt));
        printf("\t\trd: %s\n", regAddress(ID_EX.rd));

    }else if(EX->NOOP  && !EX->halt){
        printf("\t\tInstruction: \tNOOP\n");
        printf("\t\tPCPlus4: %d\n", ID_EX.PCplus4 );
        printf("\t\tbranchTarget: %d\n", 0);
        printf("\t\treadData1: %d\n", 0);
        printf("\t\treadData2: %d\n", 0);
        printf("\t\timmed: %d\n", 0);
        printf("\t\trs: %s\n", 0);
        printf("\t\trt: %s\n", 0);
        printf("\t\trd: %s\n", 0);
    }else if(EX->halt ){
        printf("\t\tInstruction: \tHALT\n");
        printf("\t\tPCPlus4: %d\n", ID_EX.PCplus4 );
        printf("\t\tbranchTarget: %d\n", 0);
        printf("\t\treadData1: %d\n", 0);
        printf("\t\treadData2: %d\n", 0);
        printf("\t\timmed: %d\n", 0);
        printf("\t\trs: %s\n", 0);
        printf("\t\trt: %s\n", 0);
        printf("\t\trd: %s\n", 0);
    }else{
        printf("\t\tInstruction: \tNOOP\n");
        printf("\t\tPCPlus4: %d\n", ID_EX.PCplus4 );
        printf("\t\tbranchTarget: %d\n", 0);
        printf("\t\treadData1: %d\n", 0);
        printf("\t\treadData2: %d\n", 0);
        printf("\t\timmed: %d\n", 0);
        printf("\t\trs: %s\n", 0);
        printf("\t\trt: %s\n", 0);
        printf("\t\trd: %s\n", 0);
    }
    fprintf(stdout, "\n\tEX/MEM: ");
    if(!MEM->NOOP  && !MEM->halt){
        printf("\n\t\tInstruction: \t%s" , MEM->wholeInstruction);
        printf("\n\t\taluResults: %d", EX_MEM.ALU_Result );
        printf("\n\t\twriteDataReg: %d", EX_MEM.writeData );
        printf("\n\t\twriteReg: %s", regAddress(EX_MEM.writeRegister) );
    }else if(MEM->NOOP   && !MEM->halt ){
        fprintf(stdout, "\n\t\tInstruction: NOOP ");
        fprintf(stdout, "\n\t\taluResult: 0 ");
        fprintf(stdout, "\n\t\twriteDataReg: 0 ");
        fprintf(stdout, "\n\t\twriteReg: 0 ");
    }else if(MEM->halt){
        fprintf(stdout, "\n\t\tInstruction: HALT ");
        fprintf(stdout, "\n\t\taluResult: 0 ");
        fprintf(stdout, "\n\t\twriteDataReg: 0 ");
        fprintf(stdout, "\n\t\twriteReg: 0 ");
    }
    fprintf(stdout, "\n\tMEM/WB: ");
    if(!WB->NOOP && !WB->halt){
        printf("\n\t\tInstruction: \t%s" , WB->wholeInstruction);
        printf("\n\t\twriteDataMem: %d", MEM_WB.writeDataFromMem );
        printf("\n\t\twriteDataALU: %d", MEM_WB.writeDataFromALU );
        printf("\n\t\twriteReg: %s\n", regAddress(MEM_WB.writeRegister) );
    }else if(WB->NOOP  && !WB->halt ){
        fprintf(stdout, "\n\t\tInstruction: NOOP ");
        fprintf(stdout, "\n\t\twriteDataMem: 0 ");
        fprintf(stdout, "\n\t\twriteDataWLU: 0 ");
        fprintf(stdout, "\n\t\twriteReg: 0 \n");
    }else if(WB->halt || !strcmp(WB->instructionText, "HALT" )){
        fprintf(stdout, "\n\t\tInstruction: HALT ");
        fprintf(stdout, "\n\t\twriteDataMem: 0 ");
        fprintf(stdout, "\n\t\twriteDataWLU: 0 ");
        fprintf(stdout, "\n\t\twriteReg: 0 \n");
    }
}

void startCycles( int totalLines){
    current = start;
    current->flush = -1;
    IF_ID.PCplus4 = current->memLocation;

    typedef struct Instruction Instruction;
    IF = ID = EX = MEM = WB =(Instruction *)malloc(sizeof(Instruction));

    //Set all to originally NOOP
    IF->NOOP = ID->NOOP = EX->NOOP = MEM->NOOP = WB->NOOP = true;

    int tempTracker;
    printOutUpdatedState();
    printCycle();
   for(tempTracker = 0; tempTracker < totalLines+10; tempTracker++){

       if(!strcmp(WB->instructionText, "HALT")){
           break;
       }
       if(tempTracker >= 1){
           if(IF->shouldStall){
               cycleCounter++;
               printOutUpdatedState();
               _MEM_WB_FUNC();
               _EX_MEM_FUNC();
               _ID_EX_FUNC();
               printCycle();
               IF->shouldStall = false;
           }
           cycleCounter++;
           printOutUpdatedState();
           _MEM_WB_FUNC();
           _EX_MEM_FUNC();
           _ID_EX_FUNC();
           _IF_ID_FUNC();
           printCycle();
       }

    if(tempTracker == 0) {
        cycleCounter++;
        printOutUpdatedState();
        _IF_ID_FUNC();
        printCycle();

        cycleCounter++;
        printOutUpdatedState();
        _ID_EX_FUNC();
        _IF_ID_FUNC();
        printCycle();

        cycleCounter++;
        printOutUpdatedState();
        _EX_MEM_FUNC();
        _ID_EX_FUNC();
        _IF_ID_FUNC();
        printCycle();

        cycleCounter++;
        printOutUpdatedState();
        _MEM_WB_FUNC();
        _EX_MEM_FUNC();
        _ID_EX_FUNC();
        _IF_ID_FUNC();
        printCycle();
    }

   }

   printf("********************************\n");
   printf("Total Number of cycles executed: %d\n", cycleCounter );
    printf("Total Number of stalls: %d\n",  totalStalls);
    printf("Total Number of branches: %d\n",  totalBranches);
    printf("I DID NOT ADD SUPPORT FOR BRANCH PREDICTION");

}


int main(int argc, char *argv[]){
    //Declare some trivial info
    char instructions[100];
    int cycles = 0;
    int PC = 0;
    int tracker = 0;
    int totalLines = 0;
    bool dataSegmentReached = false;
    typedef struct Instruction Instruction;


    //Create the first Instruction
    start = (Instruction *)malloc(sizeof(Instruction));

    //Set the current Instruction to the first
    current = start;
    current->next = NULL;

    //Initialize stall instruction , which just stalls
    stall = (Instruction *)malloc(sizeof(Instruction));
    stall->shouldStall = true;


    int q = 0;
    //Get instructions line by line and build it
    while(fgets(line, 50, stdin) != NULL){


     if(atoi(line) == 1){
         //If line is 1 then we have reached data segment
         dataSegmentReached = true;
     }else if(dataSegmentReached){
         //If so populate datamem
         dataMem[q-1] = atoi(line);
         q = q + 1;
     }else{
         //If not in dataSegment then add instruction to line

         tracker++;
         totalLines++;
     }
        //set memLocation
        current->memLocation = memLocation;
        memLocation += 4;

        //Build Instruction
        buildInstruction();

        //Determine Data Location Start
        memLocation = current->memLocation + 4;

        //Allocate space for next instruction
        current->next = (Instruction *)malloc(sizeof(Instruction));
        current = current->next;

    }

    //Start running cycles
    startCycles(totalLines);

    return 0;
    }





