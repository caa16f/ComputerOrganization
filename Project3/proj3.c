#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>
#include <unistd.h>
#include <math.h>
#include <inttypes.h>



//Cache
typedef struct Cache{
   int tag;
   int dirty;
   unsigned long long  LURage;
}Cache1;
Cache*  currentCache;

//Utility to do logs
int log2(int x) {
    int i = 0;
    for(i = 0; i < 31; i++) {
        if((x >> i) == 1)
            break;
    }
    return i;
}

//All program done in main
int main(int argc, char*argv[]) {
    char line[40];  //Line holder when parsing

    //Variables for given cache stats
    int blocksize = 0;
    int numOfSets = 0;
    int setAss = 0;

    //Token to hold tokenized string
    char *token;

    //Parallel arrays
    char instructHolder[100][256];
    int *referenceHolder = (int *) malloc(100 * sizeof(int));

    //count the number of references read in
    int referenceCounter = 0;


    //Populate the variables and parallel arrays
    int q = 0;
    while (fgets(line, 50, stdin) != NULL) {
        if (q == 0) {
            blocksize = atoi(line);
        } else if (q == 1) {
            numOfSets = atoi(line);
        } else if (q == 2) {
            setAss = atoi(line);
        } else {

            token = strtok(line, " \n");
            strcpy(instructHolder[q - 3], token);
            token = strtok(NULL, " \n");
            referenceHolder[q - 3] = atoi(token);
            referenceCounter++;
        }
        q++;
    }
    //Print out initial stats
    printf("Block Size: %d\n", blocksize);
    printf("Number of Sets: %d\n", numOfSets);
    printf("Associativity: %d\n", setAss);




    //Determine the number of index(set) and offset bits using log2 function
    int numOfOffsetBits;
    int numOfBlockBits;
    numOfBlockBits = log2(numOfSets);           //Bits per set
    numOfOffsetBits = log2(blocksize);          //Bits per block

    //Shift ammount is the amount we have to shift to get tag
    int shiftAmount = numOfBlockBits + numOfOffsetBits;

    //Number of possible entries in cache
    int numOfEntriesInCache = numOfSets * setAss;

    //Dynamically allocate array of caches
    currentCache = (Cache*)malloc(numOfEntriesInCache * sizeof(Cache));


    //Print out bit numbers
    printf("Number of Offset bits: %d\n", numOfOffsetBits);
    printf("Number of Index bits: %d\n", numOfBlockBits);
    printf("Number of Tag bits: %d\n", 32 - numOfOffsetBits - numOfBlockBits);



    //Continue to write through no Write allocate
    //Print out the total number of references.
    printf("*************************************\n");
    printf("Write-through with No Write Allocate\n");
    printf("*************************************\n");
    printf("Total Number of References: %d\n", referenceCounter);


    //Start Cache simulation
    int totalMisses = 0;
    int totalHits = 0;
    int memoryReferences = 0;


    //Set up the cache. Make everything not dirty and set the LRU age to 1. Tag is set to -1 as a marker for empty slots
    int k;
    for(k = 0;k  < numOfEntriesInCache; k++){
        currentCache[k].dirty = 0;
        currentCache[k].tag = -1;
        currentCache[k].LURage = 1;
    }


    int i;
    for (i = 0; i < referenceCounter; i++) {

        //Calculate the tag , set and set range for each line referenced in our parrallel arrays
       long long int tag = referenceHolder[i] >> (shiftAmount);
       long long int set = ((referenceHolder[i] - (tag<<(numOfBlockBits+numOfOffsetBits))) >> numOfOffsetBits);

       int setLow = set * setAss;
       int setHigh = setLow + (setAss - 1);


        int j = 0;
        int hit = 0;
        //Once we have the tag and sets we can look through the set for each individual instruction
        for (j = setLow; j <= setHigh; j++) {
            //If the tag exist in the set then we have a hit.
            if (currentCache[j].tag == tag) {
                totalHits++;
                //set hit to 1 (keeps track of wether it was a hit or miss)
                hit = 1;
                memoryReferences++;

                //Increase the age of all items in slot
                int k;
                for (k = setLow; k <= setHigh; k++) {
                    currentCache[k].LURage++;
                }
                currentCache[j].LURage = 0;

                //If the hit was a write then update the tag for that slot
                if (!strcmp(instructHolder[j], "W")) {
                    currentCache[j].tag = tag;

                }
                //Break out after a hit
                break;
            }else{

            }
        }
        if (hit == 0) {
            //If hit is 0 then we have a miss
            totalMisses++;

            //If it is a W miss then just increment memory references
            if (!strcmp(instructHolder[i], "W")) {
                memoryReferences++;
            }else {
                //Otherwise it was a a R miss. Update the cache using LRU policy
                memoryReferences++;

                int t;
                int oldest = 0;
                int oldestSpot = 0;

                //Determine the LRU index
                for (t = setLow; t <= setHigh; t++) {
                    if (currentCache[t].LURage > oldest) {
                        oldest = currentCache[t].LURage;
                        oldestSpot = t;
                    }
                }
                //Replace it
                currentCache[oldestSpot].tag = tag;


                //Increment LURage of everything in the set
                for (t = setLow; t <= setHigh; t++) {
                    currentCache[t].LURage++;
                }
                currentCache[oldestSpot].LURage = 0;
            }
        }
    }



    //Print out stats
    printf("Hits: %d \n" , totalHits );
    printf("Misses: %d \n" , totalMisses );
    printf("Memory References: %d \n" , memoryReferences );


    //Write back with Write Allocate
    printf("*************************************\n");
    printf("Write-back with Write Allocate\n");
    printf("*************************************\n");
    printf("Total Number of References: %d\n", referenceCounter );

    //Reset trackers
    totalMisses = 0;
    totalHits = 0;
    memoryReferences = 0;



    //clear cache
    for(k = 0;k  < numOfEntriesInCache; k++){
        currentCache[k].dirty = 0;
        currentCache[k].tag = -1;
        currentCache[k].LURage = 1;
    }

    //Like before grab the tag and set + set ranges for each instruction line
    for (i = 0; i < referenceCounter; i++) {
        long long int tag = referenceHolder[i] >> (shiftAmount);
        long long int set = ((referenceHolder[i] - (tag<<(numOfBlockBits+numOfOffsetBits))) >> numOfOffsetBits);

        int setLow = set * setAss;
        int setHigh = setLow + (setAss - 1);

        int j = 0;
        int hit = 0;
        //Check the set for a tag
        for (j = setLow; j <= setHigh; j++) {

            if (currentCache[j].tag == tag) {
                //We have a Hit
                totalHits++;

                hit = 1;
                int k;
                for (k = setLow; k <= setHigh; k++) {
                    currentCache[k].LURage++;
                }
                currentCache[j].LURage = 0;

                //If it was a write Hit then set tag to dirty otherwise dont
                if (!strcmp(instructHolder[i], "W")) {
                    currentCache[j].tag = tag;
                    currentCache[j].dirty = 1;


                }
                break;

            }else{

            }
        }


        bool isFull = false;
        if (hit == 0) {
            //If hit == 0 then we have a miss
            totalMisses++;
            //If we have a write miss
            if (!strcmp(instructHolder[i], "W")) {

                    //Loop through the set and try and find a -1 tag. -1 Tag signifies an empty slot
                    for(k = setLow; k <= setHigh; k++){
                        if(currentCache[k].tag == -1){
                            isFull = false;
                            break;
                        }else{
                            //If no -1 tag can be found then the set must be full
                            isFull = true;
                        }
                    }
                    //If the set is full then do the LRU strategy
                    if(isFull) {

                    int t;
                    int oldest = 0;
                    int oldestSpot = 0;

                    for (t = setLow; t <= setHigh; t++) {
                        if (currentCache[t].LURage > oldest) {
                            oldest = currentCache[t].LURage;
                            oldestSpot = t;
                        }
                    }

                    if (currentCache[oldestSpot].dirty == 1) {
                        memoryReferences = memoryReferences + 2;
                    } else {
                        memoryReferences++;
                    }
                    //Set dirty to 1
                    currentCache[oldestSpot].tag = tag;
                    currentCache[oldestSpot].dirty = 1;

                    //Increment LRU age
                    for (t = setLow; t <= setHigh; t++) {
                        currentCache[t].LURage++;
                    }
                    currentCache[oldestSpot].LURage = 0;
                }else{
                        //Otherwise the cache is not full
                    memoryReferences++;

                    //Add the tag to the first slot with a -1 tag then break
                    for (j = setLow; j <= setHigh; j++) {
                    if(currentCache[j].tag == -1){
                       currentCache[j].tag = tag;
                       currentCache[j].dirty = 1;
                       break;
                    }

                    }
                }
            }else{
                //Otherwise if the command was a Read then check to see if the set is full
                isFull = false;

                for(k = setLow; k <= setHigh; k++){
                    if(currentCache[k].tag == -1){
                        isFull = false;
                        break;
                    }else{
                        isFull = true;
                    }
                }

                if(isFull) {
                    //If it is full then do LRU policy but do not set dirty to 1
                    int t;
                    int oldest = 0;
                    int oldestSpot = 0;

                    for (t = setLow; t <= setHigh; t++) {
                        if (currentCache[t].LURage > oldest) {
                            oldest = currentCache[t].LURage;
                            oldestSpot = t;
                        }
                    }

                    if (currentCache[oldestSpot].dirty == 1) {
                        memoryReferences = memoryReferences + 2;
                    } else {
                        memoryReferences++;
                    }

                    currentCache[oldestSpot].tag = tag;


                    for (t = setLow; t <= setHigh; t++) {
                        currentCache[t].LURage++;
                    }
                    currentCache[oldestSpot].LURage = 0;

                }else{
                    //Otherwise if the set is not full then just add the tag in the first avaliable empty slot
                    memoryReferences++;

                    for (j = setLow; j <= setHigh; j++) {
                        if(currentCache[j].tag == -1){
                            currentCache[j].tag = tag;
                            break;
                        }
                    }
                }
            }
        }




    }
    //Print out stats for second cache policy
    printf("Hits: %d \n" , totalHits );
    printf("Misses: %d \n" , totalMisses );
    printf("Memory References: %d \n" , memoryReferences );

    free(currentCache);
    free(referenceHolder);
    return 0;
}


