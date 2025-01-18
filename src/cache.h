#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include "functions.h"

using namespace std;

struct Result{
    int Reads;
    int ReadMisses;
    int Writes;
    int WriteMisses;
    int SwapRequests;
    int Swaps;
    int WriteBacks;
};


class Cache
{

public:

    int Size;
    int Associativity;
    int BlockSize;
    int VictimSize;
    int NumOfSets;
    int NumOfVBlocks;
    bool VictimEnabled = false;
    bool L2Enabled = false;

    Cache* L2 = nullptr;

    int IndexBits;
    int BlockOffsetBits;
    int TagBits;

    vector<vector<CacheBlock>> CacheSet;
    vector<VictimBlock> VictimCache;

    unique_ptr<ReplacementPolicy> Policy;
    

    int Misses = 0;
    int Hits = 0;
    int Reads = 0;
    int Writes = 0;
    int ReadHits = 0;
    int ReadMisses = 0;
    int WriteHits = 0;
    int WriteMisses = 0;
    int SwapRequests = 0;
    int VictimSwaps = 0;
    int WriteBacks = 0;

    Cache(int S, int Assoc, int BS, int VS = 0, string policy = "LRU"){
        if(S>0 && Assoc>0 && BS>0){
            Size = S;
            Associativity = Assoc;
            BlockSize = BS;
            VictimSize = VS;
            VictimEnabled = VictimSize>0 ? true : false;
            NumOfSets = Size/(BlockSize*Associativity);
            NumOfVBlocks = VictimSize;
            IndexBits = (int)log2(NumOfSets);
            BlockOffsetBits = (int)log2(BlockSize);
            TagBits = 32 - IndexBits - BlockOffsetBits;

            Policy = InitiatePolicy(policy, Associativity, NumOfSets, NumOfVBlocks);

            CacheSet = vector<vector<CacheBlock>>(NumOfSets, vector<CacheBlock>(Associativity));
            VictimCache = vector<VictimBlock>(NumOfVBlocks);

            for(int i = 0; i < NumOfSets; i++){
                for(int j = 0; j < Associativity; j++){
                    CacheSet[i][j].Counter = j;
                }
            }
            for(int i = 0; i < NumOfVBlocks; i++){
                VictimCache[i].Counter = i;
            }
        }
    };

    void CacheBlockAccess(vector<vector<CacheBlock>> &CacheSet, unsigned int Index, int Assoc, int Block){
        Policy->CacheBlockAccess(CacheSet, Index, Assoc, Block);
    }
    void VictimBlockAccess(vector<VictimBlock> &VictimCache, int NumOfVBlocks, int Block){
        Policy->VictimBlockAccess(VictimCache, NumOfVBlocks, Block);
    }
    int CacheBlockEvict(vector<vector<CacheBlock>> &CacheSet, unsigned int Index, int Assoc){
        return Policy->CacheBlockEvict(CacheSet, Index, Assoc);
    }
    int VictimBlockEvict(vector<VictimBlock> &VictimCache, int NumOfVBlocks){
        return Policy->VictimBlockEvict(VictimCache, NumOfVBlocks);
    }
    
    void ConnectToMem(Cache *Ptr){
        L2 = Ptr;
        L2Enabled = true;
    }

    void read(unsigned int Address){
        Reads++;
        AddressField AddressBits = ConvertToFields(Address, NumOfSets, BlockSize);
        bool MatchFound = false;
        bool CacheFull = true;
        for(int i = 0; i < Associativity; i++){
            if(CacheSet[AddressBits.Index][i].Tag == AddressBits.Tag){
                ReadHits++;
                CacheBlockAccess(CacheSet, AddressBits.Index, Associativity, i);
                MatchFound = true;
                break;
            }
            if(CacheSet[AddressBits.Index][i].Tag == -1) CacheFull = false;
        }
        if(!MatchFound){
            ReadMisses++;
            if(VictimEnabled && CacheFull){
                SwapRequests++;
                bool VictimMatchFound = false;
                for(int i = 0; i < NumOfVBlocks; i++){
                    if(VictimCache[i].Tag == AddressBits.Tag && VictimCache[i].Index == AddressBits.Index){
                        VictimSwaps++;
                        int LRUBlock = CacheBlockEvict(CacheSet, AddressBits.Index, Associativity);
                        CacheBlock LRU = CacheSet[AddressBits.Index][LRUBlock];
                        CacheSet[AddressBits.Index][LRUBlock].Tag = VictimCache[i].Tag;
                        CacheSet[AddressBits.Index][LRUBlock].isDirty = VictimCache[i].isDirty;
                        CacheBlockAccess(CacheSet, AddressBits.Index, Associativity, LRUBlock);
                        VictimCache[i].Index = AddressBits.Index;
                        VictimCache[i].Tag = LRU.Tag;
                        VictimCache[i].isDirty = LRU.isDirty;
                        VictimBlockAccess(VictimCache, NumOfVBlocks, i);
                        VictimMatchFound = true;
                        break;
                    }
                }
                if(!VictimMatchFound){
                    if(L2Enabled){
                    (*L2).read(Address);
                    //printf("%x\n", Address);
                    }
                    int CacheLRU = CacheBlockEvict(CacheSet, AddressBits.Index, Associativity);
                    int VictimLRU = VictimBlockEvict(VictimCache, NumOfVBlocks);
                    if(VictimCache[VictimLRU].isDirty){
                        WriteBacks++;
                        unsigned int WriteAddress = ConvertToAddress(VictimCache[VictimLRU].Index, VictimCache[VictimLRU].Tag, IndexBits, BlockOffsetBits);
                        if(L2Enabled)(*L2).write(WriteAddress);
                        //printf("%x\n", WriteAddress);
                    }
                    VictimCache[VictimLRU].Index = AddressBits.Index;
                    VictimCache[VictimLRU].Tag =  CacheSet[AddressBits.Index][CacheLRU].Tag;
                    VictimCache[VictimLRU].isDirty = CacheSet[AddressBits.Index][CacheLRU].isDirty;
                    VictimBlockAccess(VictimCache, NumOfVBlocks, VictimLRU);
                    CacheSet[AddressBits.Index][CacheLRU].Tag = AddressBits.Tag;
                    CacheSet[AddressBits.Index][CacheLRU].isDirty = false;
                    CacheBlockAccess(CacheSet, AddressBits.Index, Associativity, CacheLRU);
                }
            }
            else{
                if(L2Enabled){
                    (*L2).read(Address);
                    //printf("%x\n", Address);
                }
                int CacheLRU = CacheBlockEvict(CacheSet, AddressBits.Index, Associativity);
                if(CacheSet[AddressBits.Index][CacheLRU].isDirty){
                    WriteBacks++;
                    unsigned int WriteAddress = ConvertToAddress(AddressBits.Index, CacheSet[AddressBits.Index][CacheLRU].Tag, IndexBits, BlockOffsetBits);
                    if(L2Enabled)(*L2).write(WriteAddress);
                    //printf("%x\n", WriteAddress);
                }
                CacheSet[AddressBits.Index][CacheLRU].Tag = AddressBits.Tag;
                CacheSet[AddressBits.Index][CacheLRU].isDirty = false;
                CacheBlockAccess(CacheSet, AddressBits.Index, Associativity, CacheLRU);
            }
        }


    }
    void write(unsigned int Address){
        Writes++;
        bool MatchFound = false;
        bool CacheFull = true;
        AddressField AddressBits = ConvertToFields(Address, NumOfSets, BlockSize);
        for(int i = 0; i < Associativity; i++){
            if(CacheSet[AddressBits.Index][i].Tag == AddressBits.Tag){
                WriteHits++;
                CacheSet[AddressBits.Index][i].isDirty = true;
                CacheBlockAccess(CacheSet, AddressBits.Index, Associativity, i);
                MatchFound = true;
                break;
            }
            if(CacheSet[AddressBits.Index][i].Tag == -1) CacheFull = false;
        }
        if(!MatchFound){
            WriteMisses++;
            if(VictimEnabled && CacheFull){
                bool VictimMatchFound = false;
                SwapRequests++;
                for(int i = 0; i < NumOfVBlocks; i++){
                    if(VictimCache[i].Tag == AddressBits.Tag && VictimCache[i].Index == AddressBits.Index){
                        VictimSwaps++;
                        int CacheLRU = CacheBlockEvict(CacheSet, AddressBits.Index, Associativity);
                        CacheBlock LRU = CacheSet[AddressBits.Index][CacheLRU];
                        CacheSet[AddressBits.Index][CacheLRU].Tag = VictimCache[i].Tag;
                        CacheSet[AddressBits.Index][CacheLRU].isDirty = true;
                        CacheBlockAccess(CacheSet, AddressBits.Index, Associativity, CacheLRU);
                        VictimCache[i].Index = AddressBits.Index;
                        VictimCache[i].Tag = LRU.Tag;
                        VictimCache[i].isDirty = LRU.isDirty;
                        VictimBlockAccess(VictimCache, NumOfVBlocks, i);
                        VictimMatchFound = true;
                        break;
                    }
                }
                if(!VictimMatchFound){
                    if(L2Enabled){
                    (*L2).read(Address);
                    //printf("%x\n", Address);
                    }
                    int CacheLRU = CacheBlockEvict(CacheSet, AddressBits.Index, Associativity);
                    int VictimLRU = VictimBlockEvict(VictimCache, NumOfVBlocks);
                    if(VictimCache[VictimLRU].isDirty){
                        WriteBacks++;
                        unsigned int WriteAddress = ConvertToAddress(AddressBits.Index, AddressBits.Tag, IndexBits, BlockOffsetBits);
                        if(L2Enabled)(*L2).write(WriteAddress);
                        //printf("%x\n", WriteAddress);
                    }
                    VictimCache[VictimLRU].Index = AddressBits.Index;
                    VictimCache[VictimLRU].Tag =  CacheSet[AddressBits.Index][CacheLRU].Tag;
                    VictimCache[VictimLRU].isDirty = CacheSet[AddressBits.Index][CacheLRU].isDirty;
                    VictimBlockAccess(VictimCache, NumOfVBlocks, VictimLRU);
                    CacheSet[AddressBits.Index][CacheLRU].Tag = AddressBits.Tag;
                    CacheSet[AddressBits.Index][CacheLRU].isDirty = true;
                    CacheBlockAccess(CacheSet, AddressBits.Index, Associativity, CacheLRU);
                }
            }
            else{
                if(L2Enabled){
                    (*L2).read(Address);
                    //printf("%x\n", Address);
                 }
                int CacheLRU = CacheBlockEvict(CacheSet, AddressBits.Index, Associativity);
                if(CacheSet[AddressBits.Index][CacheLRU].isDirty){
                    WriteBacks++;
                    unsigned int WriteAddress = ConvertToAddress(AddressBits.Index, AddressBits.Tag, IndexBits, BlockOffsetBits);
                    if(L2Enabled)(*L2).write(WriteAddress);
                    //printf("%x\n", WriteAddress);
                }
                CacheSet[AddressBits.Index][CacheLRU].Tag = AddressBits.Tag;
                CacheSet[AddressBits.Index][CacheLRU].isDirty = true;
                CacheBlockAccess(CacheSet, AddressBits.Index, Associativity, CacheLRU);
            }
        }


    }

    void printCache(){
    for (int i = 0; i < NumOfSets; i++) {
        printf("Set %d: ", i);

        vector<CacheBlock> sortedSet = CacheSet[i];
        sort(sortedSet.begin(), sortedSet.end(), [](const CacheBlock &a, const CacheBlock &b) {
            return a.Counter < b.Counter; 
        });

        for (int j = 0; j < Associativity; j++) {
            printf("%x ", sortedSet[j].Tag);
            if (sortedSet[j].isDirty) {
                printf("D  ");
            } else {
                printf("   ");
            }
        }
        printf("\n");
    }
    printf("\n");
    }

    void printVictim(){
        if (VictimEnabled) {
            printf("===== VC contents ===== \n");
            printf("set 0: ");

            vector<VictimBlock> sortedVictimCache = VictimCache;
            sort(sortedVictimCache.begin(), sortedVictimCache.end(), [](const VictimBlock &a, const VictimBlock &b) {
                return a.Counter < b.Counter; 
            });

            for (int i = 0; i < NumOfVBlocks; i++) {
                printf("%x ", (sortedVictimCache[i].Tag << (IndexBits)) | sortedVictimCache[i].Index);
                if (sortedVictimCache[i].isDirty) {
                    printf("D  ");
                } else {
                    printf("   ");
                }
            }
            printf("\n\n");
        }
    }

    Result getResult(){
        Result Results = {Reads, ReadMisses, Writes, WriteMisses, SwapRequests, VictimSwaps, WriteBacks};
        return Results;
    }
};