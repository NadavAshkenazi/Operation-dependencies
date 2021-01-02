//
// Created by nadav ashkenazi on 02/01/2021.
//

#include "dflow_calc.h"
#include "vector"
#include "list"
#include "map"
#include <algorithm>

using namespace std;
#define ENTRY -1
#define EXIT -2

class Operation{
public:
    InstInfo info;
    int dependency1;
    int dependency2;
    int opDepth;
    bool isDependent(int index){
        return (dependency1 == index || dependency2 == index);
    }
};



class dflow_calc{
private:
    map<int,Operation*>* dependenciesGraph;
    vector<int>* sources;
    vector<int>* leaves;
    map<int,int>* registers;
    const unsigned int* opsLatency;
    void removeFromLeaves(int index);
public:
    dflow_calc(const unsigned int* opsLatency, unsigned int numOfInsts);
    ~dflow_calc();
    unsigned int numOfInsts;
    bool isSource(int index);
    void addOperation(InstInfo info);
    vector<int> getDependencies(InstInfo info);
    vector<int> getDependencies(int index);
    int longestPath(int index);
    int getDepth();
};
dflow_calc::dflow_calc(const unsigned int* opsLatency, unsigned int numOfInsts):opsLatency(opsLatency), numOfInsts(numOfInsts){
    dependenciesGraph = new map<int,Operation*>;
    sources = new vector<int>;
    leaves = new vector<int>;
    registers = new map<int,int>;
    for (int i = 0; i < MAX_OPS; i++){
        (*registers)[i] = ENTRY;
    }
}

dflow_calc::~dflow_calc(){
    delete dependenciesGraph;
    delete sources;
    delete leaves;
    delete registers;
}

bool dflow_calc::isSource(int index) {
    vector<int>::iterator it;
    for (it = sources->begin(); it != sources->end(); ++it){
        if (*it == index)
            return true;
    }
    return false;
}

void dflow_calc::removeFromLeaves(int index){
    vector<int>::iterator it;
    for (it = leaves->begin(); it != leaves->end(); ++it){
        if (*it == index)
            leaves->erase(it);
    }
}

void dflow_calc::addOperation(InstInfo info) {
    int index = dependenciesGraph->size();
    Operation* op = new Operation();
    op->info = info;
    vector<int> dep = getDependencies(info);
    if(dep[0] == ENTRY && dep[1] == ENTRY){
        sources->push_back(index);
        op->dependency1 = ENTRY;
        op->dependency2 = ENTRY;
        op->opDepth = 0;
    }
    else if(dep[0] == ENTRY) {
        op->dependency1 = ENTRY;
        op->dependency2 = dep[1];
        removeFromLeaves(dep[1]);
        op->opDepth = (*dependenciesGraph)[dep[1]]->opDepth + opsLatency[(*dependenciesGraph)[dep[1]]->info.opcode];
    }
    else if(dep[1] == ENTRY) {
        op->dependency1 = dep[0];
        removeFromLeaves(dep[0]);
        op->dependency2 = ENTRY;
        op->opDepth = (*dependenciesGraph)[dep[0]]->opDepth + opsLatency[(*dependenciesGraph)[dep[0]]->info.opcode];
    }
    else {
        op->dependency1 = dep[0];
        removeFromLeaves(dep[0]);
        op->dependency2 = dep[1];
        removeFromLeaves(dep[1]);
        op->opDepth = max((*dependenciesGraph)[dep[0]]->opDepth + opsLatency[(*dependenciesGraph)[dep[0]]->info.opcode],
                          (*dependenciesGraph)[dep[1]]->opDepth + opsLatency[(*dependenciesGraph)[dep[1]]->info.opcode]);
    }
    leaves->push_back(index);
    (*registers)[info.dstIdx] = index;
    (*dependenciesGraph)[index] = op;
}

vector<int> dflow_calc::getDependencies(InstInfo info){
    vector<int> dep = vector<int>();
    dep.push_back((*registers)[info.src1Idx]);
    dep.push_back((*registers)[info.src2Idx]);
    return dep;
}

vector<int> dflow_calc::getDependencies(int index){
    vector<int> dep = vector<int>();
    dep.push_back((*dependenciesGraph)[index]->dependency1);
    dep.push_back((*dependenciesGraph)[index]->dependency2);
    return dep;
}

int dflow_calc::longestPath(int index){
    return (*dependenciesGraph)[index]->opDepth;
}

int dflow_calc::getDepth(){
    int depth = 0;
    for(int i = 0; i < leaves->size(); i++){
        Operation* op = (*dependenciesGraph)[(*leaves)[i]];
        int newDepth = op->opDepth + opsLatency[op->info.opcode];
        depth = max(depth,newDepth);
    }
    return depth;
}



ProgCtx analyzeProg(const unsigned int opsLatency[], const InstInfo progTrace[], unsigned int numOfInsts) {
    dflow_calc* ctx = new dflow_calc(opsLatency, numOfInsts);
    for (int i = 0; i < numOfInsts; i++){
        ctx->addOperation(progTrace[i]);
    }
    return ctx;
//    return PROG_CTX_NULL;
}

void freeProgCtx(ProgCtx ctx) {
    delete ctx;
}

int getInstDepth(ProgCtx ctx, unsigned int theInst) {
    dflow_calc* calc = (dflow_calc*) ctx;
    return calc->longestPath(theInst);
}

int getInstDeps(ProgCtx ctx, unsigned int theInst, int *src1DepInst, int *src2DepInst) {
    dflow_calc* calc = (dflow_calc*) ctx;
    if (theInst < 0 || theInst >= calc->numOfInsts)
        return -1;
    vector<int> dep = calc->getDependencies(theInst);
    *src1DepInst = dep[0];
    *src2DepInst = dep[1];
    if(calc->isSource(theInst))
        return -1;
    return 0;
}

int getProgDepth(ProgCtx ctx) {
    dflow_calc* calc = (dflow_calc*) ctx;
    return calc->getDepth();
}

