#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include<algorithm>
#include "functions.h"

using namespace std;


class Cache
{

public:

    int Size;
    int Associativity;
    int BlockSize;
    int VictimSize;
    int NumOfSets;
    int NumOfVBlocks;
    bool VictimEnabled;
    bool L2Enabled;

    Cache* L2;

    int IndexBits;
    int BlockOffsetBits;
    int TagBits;

    vector<vector<CacheBlock>> CacheSet;
    vector<VictimBlock> VictimCache;
    

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

    Cache(int S, int Assoc, int BS, int VS = 0, Cache* Ptr = nullptr){

        Size = S;
        Associativity = Assoc;
        BlockSize = BS;
        VictimSize = VS;
        L2 = Ptr;
        VictimEnabled = VictimSize>0 ? true : false;
        L2Enabled = (L2 == nullptr) ? false : true;
        NumOfSets = Size/(BlockSize*Associativity);
        NumOfVBlocks = VictimSize;
        IndexBits = (int)log2(NumOfSets);
        BlockOffsetBits = (int)log2(BlockSize);
        TagBits = 32 - IndexBits - BlockOffsetBits;

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

    };

    void read(int Address){
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
                        int LRUBlock = GetCacheLRU(CacheSet, AddressBits.Index, Associativity);
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
                    }
                    int CacheLRU = GetCacheLRU(CacheSet, AddressBits.Index, Associativity);
                    int VictimLRU = GetVictimLRU(VictimCache, NumOfVBlocks);
                    if(VictimCache[VictimLRU].isDirty){
                        WriteBacks++;
                        int WriteAddress = ConvertToAddress(VictimCache[VictimLRU].Index, VictimCache[VictimLRU].Tag, IndexBits, BlockOffsetBits);
                        if(L2Enabled)(*L2).write(WriteAddress);
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
                }
                int CacheLRU = GetCacheLRU(CacheSet, AddressBits.Index, Associativity);
                if(CacheSet[AddressBits.Index][CacheLRU].isDirty){
                    WriteBacks++;
                    int WriteAddress = ConvertToAddress(AddressBits.Index, CacheSet[AddressBits.Index][CacheLRU].Tag, IndexBits, BlockOffsetBits);
                    if(L2Enabled)(*L2).write(WriteAddress);
                }
                CacheSet[AddressBits.Index][CacheLRU].Tag = AddressBits.Tag;
                CacheSet[AddressBits.Index][CacheLRU].isDirty = false;
                CacheBlockAccess(CacheSet, AddressBits.Index, Associativity, CacheLRU);
            }
        }


    }
    void write(int Address){
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
                        int CacheLRU = GetCacheLRU(CacheSet, AddressBits.Index, Associativity);
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
                    }
                    int CacheLRU = GetCacheLRU(CacheSet, AddressBits.Index, Associativity);
                    int VictimLRU = GetVictimLRU(VictimCache, NumOfVBlocks);
                    if(VictimCache[VictimLRU].isDirty){
                        WriteBacks++;
                        int WriteAddress = ConvertToAddress(AddressBits.Index, AddressBits.Tag, IndexBits, BlockOffsetBits);
                        if(L2Enabled)(*L2).write(WriteAddress);
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
                 }
                int CacheLRU = GetCacheLRU(CacheSet, AddressBits.Index, Associativity);
                if(CacheSet[AddressBits.Index][CacheLRU].isDirty){
                    WriteBacks++;
                    int WriteAddress = ConvertToAddress(AddressBits.Index, AddressBits.Tag, IndexBits, BlockOffsetBits);
                    if(L2Enabled)(*L2).write(WriteAddress);
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
            printf("set 0:");

            vector<VictimBlock> sortedVictimCache = VictimCache;
            sort(sortedVictimCache.begin(), sortedVictimCache.end(), [](const VictimBlock &a, const VictimBlock &b) {
                return a.Counter < b.Counter; 
            });

            for (int i = 0; i < NumOfVBlocks; i++) {
                printf("%x ", ConvertToAddress(sortedVictimCache[i].Index, sortedVictimCache[i].Tag, IndexBits, BlockOffsetBits));
                if (sortedVictimCache[i].isDirty) {
                    printf("D  ");
                } else {
                    printf("   ");
                }
            }
            printf("\n\n");
        }
    }


    void printResult(){
        printf("Reads: %d\n", Reads);
        //printf("Read Hits: %d\n", ReadHits);
        printf("Read Misses: %d\n", ReadMisses);
        printf("Writes: %d\n", Writes);
        //printf("Write Hits: %d\n", WriteHits);
        printf("Write Misses: %d\n", WriteMisses);
        printf("Swap Requests: %d\n", SwapRequests);
        printf("Swaps: %d\n", VictimSwaps);
        printf("Writebacks: %d\n\n", WriteBacks);
    }
};