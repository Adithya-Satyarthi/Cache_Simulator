#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include "src/cache.h"


using namespace std;

int main(int argc, char* argv[]){

    srand(time(0));

    if(argc != 9){
        cout << "Error: Number of Parameters not Correct!\n";
        printf("%d\n", argc);
        printf("Usage:  <l1_size> <l1_assoc> <l1_block_size> <vc_num_blocks> <l2_size> <l2_assoc> <trace_file> <policy>\n");
        return 1;
    }
    int L1Size = atoi(argv[1]);
    int L1Assoc = atoi(argv[2]);
    int L1Blocksize = atoi(argv[3]);
    int VictimSize = atoi(argv[4]);
    int L2Size = atoi(argv[5]);
    int L2Assoc = atoi(argv[6]);  
    FILE *trace_file = fopen(argv[7], "r");
    string Policy = (argc > 8) ? argv[8] : "LRU";

    Cache L2(L2Size, L2Assoc, L1Blocksize, 0, Policy);
    Cache L1(L1Size, L1Assoc, L1Blocksize, VictimSize, Policy);
    if(L2Size>0){
        L1.ConnectToMem(&L2);
    }

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

    if(L2Size>0){
        printf("===== L1 contents =====\n");
        L1.printCache();
        L1.printVictim();
        printf("===== L2 contents =====\n");
        L2.printCache();
        printf("Cache L1\n");
        Result L1Result = L1.getResult();
        printf("Reads:                %d\n", L1Result.Reads);
        printf("Read Misses:          %d\n", L1Result.ReadMisses);
        printf("Writes:               %d\n", L1Result.Writes);
        printf("Write Misses:         %d\n", L1Result.WriteMisses);
        printf("Swap Requests:        %d\n", L1Result.SwapRequests);
        float SwapRequestRate = (float)L1Result.SwapRequests/(L1Result.Reads+L1Result.Writes);
        printf("Swap Request Rate:    %f\n", SwapRequestRate);
        printf("Swaps:                %d\n", L1Result.Swaps);
        float CombinedMissRate = (float)(L1Result.ReadMisses+L1Result.ReadMisses-L1Result.Swaps)/(L1Result.Reads+L1Result.Writes);
        printf("Combined Miss Rate:   %f\n", CombinedMissRate);
        printf("Writebacks:           %d\n\n", L1Result.WriteBacks);
        printf("Cache L2\n");
        Result L2Result = L2.getResult();
        printf("Reads:                %d\n", L2Result.Reads);
        printf("Read Misses:          %d\n", L2Result.ReadMisses);
        printf("Writes:               %d\n", L2Result.Writes);
        printf("Write Misses:         %d\n", L2Result.WriteMisses);
        float MissRate = (float)L2Result.ReadMisses/(L2Result.Reads);
        printf("Miss Rate:            %f\n", MissRate);
        printf("Writebacks:           %d\n", L2Result.WriteBacks);
        printf("Total Memory Traffic: %d\n\n", L2Result.ReadMisses+L2Result.WriteMisses+L2Result.WriteBacks);
    }
    else{
        printf("===== L1 contents =====\n");
        L1.printCache();
        L1.printVictim();
        printf("Cache L1\n");
        Result L1Result = L1.getResult();
        printf("Reads:                %d\n", L1Result.Reads);
        printf("Read Misses:          %d\n", L1Result.ReadMisses);
        printf("Writes:               %d\n", L1Result.Writes);
        printf("Write Misses:         %d\n", L1Result.WriteMisses);
        printf("Swap Requests:        %d\n", L1Result.SwapRequests);
        float SwapRequestRate = (float)L1Result.SwapRequests/(L1Result.Reads+L1Result.Writes);
        printf("Swap Request Rate:    %f\n", SwapRequestRate);
        printf("Swaps:                %d\n", L1Result.Swaps);
        float CombinedMissRate = (float)(L1Result.ReadMisses+L1Result.ReadMisses-L1Result.Swaps)/(L1Result.Reads+L1Result.Writes);
        printf("Combined Miss Rate:   %f\n", CombinedMissRate);
        printf("Writebacks:           %d\n", L1Result.WriteBacks);
        printf("Total Memory Traffic: %d\n\n", L1Result.ReadMisses+L1Result.ReadMisses-L1Result.Swaps+L1Result.WriteBacks);
    }

    return 0;
}