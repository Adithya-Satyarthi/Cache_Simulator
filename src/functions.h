#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>

using namespace std;

struct CacheBlock{
    int Tag = -1;
    int Counter = 0;
    bool isValid = false;
    bool isDirty = false;
};

struct VictimBlock{
    int Index = -1;
    int Tag = -1;
    int Counter = 0;
    bool isValid = false;
    bool isDirty = false;
};

struct AddressField{

    int Tag;
    int Index;
    int BlockOffset;

};

AddressField ConvertToFields(int Address, int NumOfSets, int BlockSize){

    AddressField Result;
    int IndexBits = (int)log2(NumOfSets);
    int BlockOffsetBits = (int)log2(BlockSize);
    int TagBits = 32 - IndexBits - BlockOffsetBits;

    unsigned int BlockOffsetMask = (1 << BlockOffsetBits) - 1;
    unsigned int IndexMask = (1 << IndexBits) - 1;
    unsigned int TagMask = (1 << TagBits) - 1;

    int BlockOffset = Address & BlockOffsetMask;
    int Index = (Address >> BlockOffsetBits) & IndexMask;
    int Tag = (Address >> (BlockOffsetBits + IndexBits)) & TagMask;

    Result = {Tag, Index, BlockOffset};
    return Result;
}

int ConvertToAddress(int Index, int Tag, int IndexBits, int BlockOffsetBits){
    return (Tag << (IndexBits + BlockOffsetBits)) | (Index << BlockOffsetBits);
}

void CacheBlockAccess(vector<vector<CacheBlock>> &CacheSet, int Index, int Assoc, int Block){
    int AccessedCounter = CacheSet[Index][Block].Counter;
    for(int i = 0; i < Assoc; i++){
        if(CacheSet[Index][i].Counter < AccessedCounter){
            CacheSet[Index][i].Counter++;
        }
    }
    CacheSet[Index][Block].Counter = 0;
}

void VictimBlockAccess(vector<VictimBlock> &VictimCache, int NumOfVBlocks, int Block){
    int AccessedCounter = VictimCache[Block].Counter;
    for(int i = 0; i < NumOfVBlocks; i++){
        if(VictimCache[i].Counter < AccessedCounter){
            VictimCache[i].Counter++;
        }
    }
    VictimCache[Block].Counter = 0;
}

int GetCacheLRU(vector<vector<CacheBlock>> &CacheSet, int Index, int Assoc){
    int LRUCounter = 0;
    int LRU = 0;
    for(int i = 0; i < Assoc; i++){
        if(CacheSet[Index][i].Counter > LRUCounter){
            LRUCounter = CacheSet[Index][i].Counter;
            LRU = i;
        }
    }
    return LRU;
}

int GetVictimLRU(vector<VictimBlock> &VictimCache, int NumOfVBlocks){
    int LRUCounter = 0;
    int LRU = 0;
    for(int i = 0; i < NumOfVBlocks; i++){
        if(VictimCache[i].Counter > LRUCounter){
            LRUCounter = VictimCache[i].Counter;
            LRU = i;
        }
    }
    return LRU;
}



