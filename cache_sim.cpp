#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include "cache.h"

using namespace std;

int main(int argc, char* argv[]){

    if(argc != 8){
        cout << "Error: Number of Parameters not Correct!\n";
        printf("Usage:  <l1_size> <l1_assoc> <l1_block_size> <l1_vc_num_blocks>  <l2_size> <l2_assoc>  \n");
        return 1;
    }
    int L1Size = atoi(argv[1]);
    int L1Assoc = atoi(argv[2]);
    int L1Blocksize = atoi(argv[3]);
    int VictimSize = atoi(argv[4]);
    int L2Size = atoi(argv[5]);
    int L2Assoc = atoi(argv[6]);  
    FILE *trace_file = fopen(argv[7], "r");

    Cache* L2 = nullptr;
    Cache L1 = (L2Size > 0) 
            ? Cache(L1Size, L1Assoc, L1Blocksize, VictimSize, (L2 = new Cache(L2Size, L2Assoc, L1Blocksize)))
            : Cache(L1Size, L1Assoc, L1Blocksize, VictimSize);
    
    char Operation;   
    unsigned int Address;
    while (fscanf(trace_file, "%c %x\n", &Operation, &Address) != EOF){
        if(Operation == 'r'){
            L1.read(Address);
        }
        else{
            L1.write(Address);
        }
    }
    fclose(trace_file);

    printf("===== Simulator configuration =====\n");
    printf("L1_SIZE      :%d \n",L1Size);
    printf("L1_ASSOC     :%d \n",L1Assoc);
    printf("L1_BLOCKSIZE :%d \n",L1Blocksize);
    printf("VC_NUM_BLOCKS:%d \n",VictimSize);
    printf("L2_SIZE      :%d \n",L2Size);
    printf("L2_ASSOC     :%d \n",L2Assoc);
    printf("TRACE_FILE   :%s\n\n", argv[7]);
    printf("===== L1 contents =====\n");
    L1.printCache();
    L1.printVictim();
    if(L2Size>0){
        printf("===== L2 contents =====\n");
        (*L2).printCache();
    }
    L1.printResult();
    if(L2Size>0){
        L2->printResult();
    }

    return 0;
}