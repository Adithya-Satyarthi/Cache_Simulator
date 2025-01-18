#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <memory>
#include <queue>

using namespace std;

struct CacheBlock{
    int Tag = -1;
    int Counter = 0;
    bool isValid = false;
    bool isDirty = false;
};

struct VictimBlock{
    unsigned int Index = -1;
    int Tag = -1;
    int Counter = 0;
    bool isValid = false;
    bool isDirty = false;
};



class ReplacementPolicy{

public:
    virtual ~ReplacementPolicy() = default;

    virtual void CacheBlockAccess(vector<vector<CacheBlock>> &CacheSet, unsigned int Index, int Assoc, int Block) = 0;
    virtual void VictimBlockAccess(vector<VictimBlock> &VictimCache, int NumOfVBlocks, int Block) = 0;

    virtual int CacheBlockEvict(vector<vector<CacheBlock>> &CacheSet, unsigned int Index, int Assoc) = 0;
    virtual int VictimBlockEvict(vector<VictimBlock> &VictimCache, int NumOfVBlocks) = 0; 

};

class LRUPolicy : public ReplacementPolicy{
public:

    vector<vector<int>> CacheCounter;
    vector<int> VictimCounter;

    LRUPolicy(int Assoc, int NumOfSets, int NumOfVBlocks){
        CacheCounter = vector<vector<int>>(NumOfSets, vector<int>(Assoc));
        VictimCounter = vector<int>(NumOfVBlocks);
        for(int i = 0; i < NumOfSets; i++){
            for(int j = 0; j < Assoc; j++){
                CacheCounter[i][j] = j;
            }
        }
        for(int i = 0; i < NumOfVBlocks; i++){
            VictimCounter[i] = i;
        }
    }

    void CacheBlockAccess(vector<vector<CacheBlock>> &CacheSet, unsigned int Index, int Assoc, int Block) override {
        int AccessedCounter = CacheCounter[Index][Block];
        for(int i = 0; i < Assoc; i++){
            if(CacheCounter[Index][i] < AccessedCounter){
                CacheCounter[Index][i]++;
            }
        }
        CacheCounter[Index][Block] = 0;
    }

    void VictimBlockAccess(vector<VictimBlock> &VictimCache, int NumOfVBlocks, int Block) override {
        int AccessedCounter = VictimCounter[Block];
        for(int i = 0; i < NumOfVBlocks; i++){
            if(VictimCounter[i] < AccessedCounter){
                VictimCounter[i]++;
            }
        }
        VictimCounter[Block] = 0;
    }

    int CacheBlockEvict(vector<vector<CacheBlock>> &CacheSet, unsigned int Index, int Assoc) override {
        int LRUCounter = 0;
        int LRU = 0;
        for(int i = 0; i < Assoc; i++){
            if(CacheCounter[Index][i] > LRUCounter){
                LRUCounter = CacheCounter[Index][i];
                LRU = i;
            }
        }
        return LRU;
    }

    int VictimBlockEvict(vector<VictimBlock> &VictimCache, int NumOfVBlocks) override {
        int LRUCounter = 0;
        int LRU = 0;
        for(int i = 0; i < NumOfVBlocks; i++){
            if(VictimCounter[i] > LRUCounter){
                LRUCounter = VictimCounter[i];
                LRU = i;
            }
        }
        return LRU;
    }
};

class RandomPolicy : public ReplacementPolicy{
public:
    void CacheBlockAccess(vector<vector<CacheBlock>> &CacheSet, unsigned int Index, int Assoc, int Block) override{}
    void VictimBlockAccess(vector<VictimBlock> &VictimCache, int NumOfVBlocks, int Block) override{}
    int CacheBlockEvict(vector<vector<CacheBlock>> &CacheSet, unsigned int Index, int Assoc) override{
        return rand()%Assoc;
    }
    int VictimBlockEvict(vector<VictimBlock> &VictimCache, int NumOfVBlocks) override{
        return rand()%NumOfVBlocks;
    }
    
};

class PseudoLRUPolicy : public ReplacementPolicy{
    vector<vector<bool>> CacheTree;
    vector<bool> VictimTree;
public:
    PseudoLRUPolicy(int NumOfSets, int Assoc, int NumOfVBlocks){
        CacheTree = vector<vector<bool>>(NumOfSets, vector<bool>(Assoc-1, false));
        VictimTree = vector<bool>(NumOfVBlocks-1, false);
    }
    void CacheBlockAccess(vector<vector<CacheBlock>> &CacheSet, unsigned int Index, int Assoc, int Block) override{
        int Node = 0;
        int level = (int)log2(Assoc);
        int i = 0;
        while(Node < Assoc-1){
            if(Block & (1<<level-i-1)){
                CacheTree[Index][Node] = true;
                Node = 2*Node + 2;
                i++;
            }
            else{
                CacheTree[Index][Node] = false;
                Node = 2*Node + 1;
                i++;
            }
        }
    }

    void VictimBlockAccess(vector<VictimBlock> &VictimCache, int NumOfVBlocks, int Block) override{
        int Node = 0;
        int level = (int)log2(NumOfVBlocks);
        int i = 0;
        while(Node < NumOfVBlocks-1){
            if(Block & (1<<level-i-1)){
                VictimTree[Node] = true;
                Node = 2*Node + 2;
                i++;
            }
            else{
                VictimTree[Node] = false;
                Node = 2*Node + 1;
                i++;
            }
        }
    }

    int CacheBlockEvict(vector<vector<CacheBlock>> &CacheSet, unsigned int Index, int Assoc) override {
        int Node = 0;
        while(Node < Assoc-1){
            if(CacheTree[Index][Node] == false){
                CacheTree[Index][Node] = true;
                Node = 2*Node + 2;
            }
            else{
                CacheTree[Index][Node] = false;
                Node = 2*Node + 1;
            }
        }
        return Node - (Assoc-1);
    }

    int VictimBlockEvict(vector<VictimBlock> &VictimCache, int NumOfVBlocks) override {
        int Node = 0;
        while(Node < NumOfVBlocks-1){
            if(VictimTree[Node] == false){
                VictimTree[Node] = true;
                Node = 2*Node + 2;
            }
            else{
                VictimTree[Node] = false;
                Node = 2*Node + 1;
            }
        }
        return Node - (NumOfVBlocks-1);
    }
};

class RoundRobinPolicy : public ReplacementPolicy{
public:
    int CacheEvict = 0;
    int VictimEvict = 0;
    void CacheBlockAccess(vector<vector<CacheBlock>> &CacheSet, unsigned int Index, int Assoc, int Block) override{}
    void VictimBlockAccess(vector<VictimBlock> &VictimCache, int NumOfVBlocks, int Block) override{}
    int CacheBlockEvict(vector<vector<CacheBlock>> &CacheSet, unsigned int Index, int Assoc) override{
        CacheEvict += 1;
        if(CacheEvict == Assoc) CacheEvict = 0;
        return CacheEvict;
    }
    int VictimBlockEvict(vector<VictimBlock> &VictimCache, int NumOfVBlocks) override{
        VictimEvict += 1;
        if(VictimEvict == NumOfVBlocks) VictimEvict = 0;
        return VictimEvict;
    }
    
};

class FIFO : public ReplacementPolicy{
    vector<queue<int>> CacheAccessOrder;
    queue<int> VictimAccessOrder;
public:

    FIFO(int NumOfSets, int Assoc, int NumOfVBlocks){
        
        CacheAccessOrder.resize(NumOfSets);

        for(int i = 0; i < NumOfSets; i++){
            for(int j = 0; j < Assoc; j++){
                CacheAccessOrder[i].push(j);
            }
        }
        for(int j = 0; j < NumOfVBlocks; j++){
            VictimAccessOrder.push(j);
        }
    }

    void CacheBlockAccess(vector<vector<CacheBlock>> &CacheSet, unsigned int Index, int Assoc, int Block) override{
        CacheAccessOrder[Index].push(Block);
    }
    void VictimBlockAccess(vector<VictimBlock> &VictimCache, int NumOfVBlocks, int Block) override{
        VictimAccessOrder.push(Block);
    }
    int CacheBlockEvict(vector<vector<CacheBlock>> &CacheSet, unsigned int Index, int Assoc) override{
        int Block = CacheAccessOrder[Index].front();
        CacheAccessOrder[Index].pop();
        return Block;
    }
    int VictimBlockEvict(vector<VictimBlock> &VictimCache, int NumOfVBlocks) override{
        int Block = VictimAccessOrder.front();
        VictimAccessOrder.pop();
        return Block;
    }
};

unique_ptr<ReplacementPolicy> InitiatePolicy(const string& Policy, int Assoc, int NumOfSets, int NumOfVBlocks) {
    if (Policy == "LRU") {
        return make_unique<LRUPolicy>(Assoc, NumOfSets, NumOfVBlocks);
    } else if (Policy == "Random") {
        return make_unique<RandomPolicy>();
    } else if (Policy == "FIFO"){
        return make_unique<FIFO>(NumOfSets, Assoc, NumOfVBlocks);
    } else if (Policy == "RoundRobin"){
        return make_unique<RoundRobinPolicy>();
    } else if (Policy == "PseudoLRU"){
        return make_unique<PseudoLRUPolicy>(NumOfSets, Assoc, NumOfVBlocks);
    }
    throw invalid_argument("Unknown replacement policy: " + Policy);
}