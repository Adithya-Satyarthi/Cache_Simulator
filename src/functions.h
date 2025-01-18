#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>
#include "policies.h"

using namespace std;


struct AddressField{

    int Tag;
    int Index;
    int BlockOffset;

};

AddressField ConvertToFields(unsigned int Address, int NumOfSets, int BlockSize){

    AddressField Result;
    int IndexBits = (int)log2(NumOfSets);
    int BlockOffsetBits = (int)log2(BlockSize);
    int TagBits = 32 - IndexBits - BlockOffsetBits;

    unsigned int BlockOffsetMask = (1 << BlockOffsetBits) - 1;
    unsigned int IndexMask = (1 << IndexBits) - 1;

    unsigned int BlockOffset = Address & BlockOffsetMask;
    unsigned int Index = (Address >> BlockOffsetBits) & IndexMask;
    unsigned int Tag = (Address >> (BlockOffsetBits + IndexBits));

    Result.Tag = Tag;
    Result.Index = Index;
    Result.BlockOffset = BlockOffset;
    return Result;
}

unsigned int ConvertToAddress(unsigned int Index, unsigned int Tag, int IndexBits, int BlockOffsetBits){
    return (Tag << (IndexBits + BlockOffsetBits)) | (Index << BlockOffsetBits);
}




