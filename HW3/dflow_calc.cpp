/* 046267 Computer Architecture - HW #3 */
/* Implementation (skeleton)  for the dataflow statistics calculator */

#include "dflow_calc.h"
#include <stdio.h>

#define MAX_DEPENDENCIES 2
#define ENTRY_DEPENDENCE -1
#define NUM_OF_REGS 32
#define SRC1_DEPENDENCE 0
#define SRC2_DEPENDENCE 1

class InstData{
public:
	int  idx;
	InstInfo inst;
	int instLatency;
	int depth;
	int dependencies[MAX_DEPENDENCIES];

	InstData(){
		this->idx = 0;
		this->inst.dstIdx = 0;
		this->inst.opcode = 0;
		this->inst.src1Idx = 0;
		this->inst.src2Idx = 0;
		this->instLatency = 0;
		this->depth = 0;
		for(int i = 0; i < MAX_DEPENDENCIES; i++){
			this->dependencies[i] = ENTRY_DEPENDENCE;
		}
	}
};

class ProgData{
public:
	InstData* insts;
	int numOfInsts;
	int progDepth;

	ProgData(unsigned int numOfInsts){
		this->insts = new InstData[numOfInsts];
		this->numOfInsts = numOfInsts;
		this->progDepth = 0;
	}

	~ProgData(){
		delete[] this->insts;
	}
};

class RegUsage{
public:
	int idx;
	int instLatency;
	int depth;
	bool valid;

	RegUsage(){
		this->idx = 0;
		this->instLatency = 0;
		this->depth = 0;
		valid = false;
	}
};

ProgCtx analyzeProg(const unsigned int opsLatency[], const InstInfo progTrace[], unsigned int numOfInsts) {
	ProgData* progData = new ProgData(numOfInsts);
	RegUsage lastWrite[NUM_OF_REGS];
	int src1Latency = 0;
	int src2Latency = 0;

	for(unsigned int i = 0; i < numOfInsts; i++){
	//	printf("the inst %d\n", i);
		progData->insts[i].idx = i;
		progData->insts[i].inst = progTrace[i];
		progData->insts[i].instLatency = opsLatency[progTrace[i].opcode];
	//	printf("the opcode lat = %d\n", progData->insts[i].instLatency);

		if(lastWrite[progTrace[i].src1Idx].valid == true){
			progData->insts[i].dependencies[SRC1_DEPENDENCE] = lastWrite[progTrace[i].src1Idx].idx;
			src1Latency = lastWrite[progTrace[i].src1Idx].depth + lastWrite[progTrace[i].src1Idx].instLatency;
		}

		if(lastWrite[progTrace[i].src2Idx].valid == true){
		//	printf("src2 is dependent\n");
			progData->insts[i].dependencies[SRC2_DEPENDENCE] = lastWrite[progTrace[i].src2Idx].idx;
			src2Latency = lastWrite[progTrace[i].src2Idx].depth + lastWrite[progTrace[i].src2Idx].instLatency;
		//	printf("src2 depth = %d\n", lastWrite[progTrace[i].src2Idx].depth);
			//printf("src2 inst lat = %d\n", lastWrite[progTrace[i].src2Idx].instLatency);
		}

	//	printf("lat1 = %d, lat2 = %d\n", src1Latency, src2Latency);

		progData->insts[i].depth = (src1Latency >= src2Latency) ? src1Latency : src2Latency;

	//	printf("the depth = %d\n", progData->insts[i].depth);

		lastWrite[progTrace[i].dstIdx].valid = true;
		lastWrite[progTrace[i].dstIdx].idx = i;
		lastWrite[progTrace[i].dstIdx].instLatency = opsLatency[progTrace[i].opcode];
		lastWrite[progTrace[i].dstIdx].depth = (src1Latency >= src2Latency) ? src1Latency : src2Latency;

		if(progData->insts[i].depth + progData->insts[i].instLatency > progData->progDepth){
			progData->progDepth = progData->insts[i].depth + progData->insts[i].instLatency;
		}
	}

    return progData;
}

void freeProgCtx(ProgCtx ctx) {
	ProgData* progData = (ProgData*)ctx;
	delete progData;
}

int getInstDepth(ProgCtx ctx, unsigned int theInst) {
	ProgData* progData = (ProgData*)ctx;
	if((int)theInst >= progData->numOfInsts){
		return -1;
	}
	return progData->insts[theInst].depth;
}

int getInstDeps(ProgCtx ctx, unsigned int theInst, int *src1DepInst, int *src2DepInst) {
	ProgData* progData = (ProgData*)ctx;
	if((int)theInst >= progData->numOfInsts){
		return -1;
	}
	*src1DepInst = progData->insts[theInst].dependencies[SRC1_DEPENDENCE];
	*src2DepInst = progData->insts[theInst].dependencies[SRC2_DEPENDENCE];
	return 0;
}

int getProgDepth(ProgCtx ctx) {
	ProgData* progData = (ProgData*)ctx;
    return progData->progDepth;
}


