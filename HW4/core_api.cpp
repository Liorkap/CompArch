/* 046267 Computer Architecture - HW #4 */

#include "core_api.h"
#include "sim_api.h"

#include <stdio.h>

#define NUM_REGS 8

//holds the thread data
class ThreadData{
public:
	int threadId;
	int sbTimer;
	bool done;
	uint32_t instId;
};

//holds statistics and regs picture per thread
class Stats{
public:
	int cyclesNum;
	int instNum;
	int numOfThreads;
	tcontext* regsPerThread;

	Stats(int numOfThreads){
		cyclesNum = 0;
		instNum = 0;
		this->numOfThreads = numOfThreads;
		regsPerThread = new tcontext[numOfThreads];
	}
};

//returns true if there are still threads that are not done
bool isRunningThreads(ThreadData* threads, int numOfThreads){
	for(int i = 0; i < numOfThreads; i++){
		if(!threads[i].done){
			return true;
		}
	}
	return false;
}

// decreases the stand-by timer of all threads by the specified time(doesnt have to be switch time)
void sbTimerHandler(ThreadData* threads, int numOfthreads, int switchTime){
	int newTime;
	for(int i = 0; i < numOfthreads; i++){
		newTime = threads[i].sbTimer - switchTime;
		if(newTime > 0){
			threads[i].sbTimer = newTime;
		}
		else{
			threads[i].sbTimer = 0;
		}
	}
}

//returns the next thread according to RR policy
int RRpolicy(int threadID, int numOfThreads){
	int nextThreadID;
	if(threadID == numOfThreads - 1){
		nextThreadID = 0;
	}
	else{
		nextThreadID = threadID + 1;
	}

	return nextThreadID;
}

//returns next thread that available with shortest time to stand-by
int getNextThread(int threadID, ThreadData* threads, int numOfThreads){
	int nextThreadID = threadID;
	int minTimer;
	int minTimerThread = -1;
	for(int i = 0; i < numOfThreads; i++){
		if(!threads[nextThreadID].done){
			minTimer = threads[nextThreadID].sbTimer;
			minTimerThread = nextThreadID;
			break;
		}
		nextThreadID = RRpolicy(nextThreadID, numOfThreads);
	}
	nextThreadID = RRpolicy(nextThreadID, numOfThreads);
	for(int i = 0; i < numOfThreads; i++){
		if(!threads[nextThreadID].done && threads[nextThreadID].sbTimer < minTimer){
			minTimer  = threads[nextThreadID].sbTimer;
			minTimerThread = nextThreadID;
		}
		nextThreadID = RRpolicy(nextThreadID, numOfThreads);
	}

	return minTimerThread;
}

//statistics instances for blocked and fine, global
Stats* statsBlocked;
Stats* statsFine;

void CORE_BlockedMT() {
	Instruction* inst = new Instruction();
	int numOfThreads = SIM_GetThreadsNum();
	ThreadData* threads = new ThreadData[numOfThreads];
	for(int i = 0; i < numOfThreads; i++){
		threads[i].threadId = i;
		threads[i].sbTimer = 0; //all threads are ready at first
		threads[i].done = false;
		threads[i].instId = 0;
	}
	int threadID = 0; //first thread is 0 in RR
	Stats* stats_init = new Stats(numOfThreads);
	for(int i = 0; i < numOfThreads; i++){
		for(int j = 0; j < NUM_REGS; j++){
			stats_init->regsPerThread[i].reg[j] = 0; // init all regs in all threads to 0
		}
	}
	statsBlocked = stats_init;

	int switchTime = SIM_GetSwitchCycles();
	int src2; //src2 can be a reg value or imm, so seperate variable is given to handle it
	int mem_read; //load read result
	int addr; //address to write/read from in load/store
	int val; //value to write in store cmd
	int currThreadID; //will save the current thread

	while(isRunningThreads(threads, numOfThreads)){ //while some threads are still running
		if(threads[threadID].sbTimer == 0){ //if the curr thread is ready. execute the current instruction of the thread
			statsBlocked->instNum++;
			SIM_MemInstRead(threads[threadID].instId, inst, threadID);

			currThreadID = threadID;

			// each cmd gets its treatment
			switch (inst->opcode){
			case CMD_HALT:
				threads[threadID].sbTimer = 1;
				threads[threadID].done = true;
				threadID = getNextThread(threadID, threads, numOfThreads);
				// if the new thread is not ready yet, idle cycles are executed
				while(threads[threadID].sbTimer != 0){
					sbTimerHandler(threads, numOfThreads, 1);
					statsBlocked->cyclesNum ++;
				}
				// when its ready, switch penalty
				statsBlocked->cyclesNum += switchTime;
				sbTimerHandler(threads, numOfThreads, switchTime);
				break;
			case CMD_LOAD:
				src2 = (inst->isSrc2Imm) ? (inst->src2_index_imm) : (statsBlocked->regsPerThread[threadID].reg[inst->src2_index_imm]);
				threads[threadID].sbTimer = SIM_GetLoadLat();
				addr = statsBlocked->regsPerThread[threadID].reg[inst->src1_index] + src2;
				SIM_MemDataRead(addr , &mem_read);
				statsBlocked->regsPerThread[threadID].reg[inst->dst_index] = mem_read;
				threadID = getNextThread(threadID, threads, numOfThreads);
				// if the new thread is not ready yet, idle cycles are executed
				while(threads[threadID].sbTimer != 0){
					sbTimerHandler(threads, numOfThreads, 1);
					statsBlocked->cyclesNum ++;
				}
				// when its ready, switch penalty if switching
				if(currThreadID != threadID){
					sbTimerHandler(threads, numOfThreads, switchTime);
					statsBlocked->cyclesNum += switchTime;
				}
				break;
			case CMD_STORE:
				src2 = (inst->isSrc2Imm) ? (inst->src2_index_imm) : (statsBlocked->regsPerThread[threadID].reg[inst->src2_index_imm]);
				threads[threadID].sbTimer = SIM_GetStoreLat();
				addr = statsBlocked->regsPerThread[threadID].reg[inst->dst_index] + src2;
				val = statsBlocked->regsPerThread[threadID].reg[inst->src1_index];
				SIM_MemDataWrite(addr, val);
				threadID = getNextThread(threadID, threads, numOfThreads);
				// if the new thread is not ready yet, idle cycles are executed
				while(threads[threadID].sbTimer != 0){
					sbTimerHandler(threads, numOfThreads, 1);
					statsBlocked->cyclesNum ++;
				}
				// when its ready, switch penalty if switching
				if(currThreadID != threadID){
					sbTimerHandler(threads, numOfThreads, switchTime);
					statsBlocked->cyclesNum += switchTime;
				}
				break;
			case CMD_ADD:
				threads[threadID].sbTimer = 1;
				statsBlocked->regsPerThread[threadID].reg[inst->dst_index] = statsBlocked->regsPerThread[threadID].reg[inst->src1_index] + statsBlocked->regsPerThread[threadID].reg[inst->src2_index_imm];
				break;
			case CMD_ADDI:
				threads[threadID].sbTimer = 1;
				src2 = (inst->isSrc2Imm) ? (inst->src2_index_imm) : (statsBlocked->regsPerThread[threadID].reg[inst->src2_index_imm]);
				statsBlocked->regsPerThread[threadID].reg[inst->dst_index] = statsBlocked->regsPerThread[threadID].reg[inst->src1_index] + src2;
				break;
			case CMD_SUB:
				threads[threadID].sbTimer = 1;
				statsBlocked->regsPerThread[threadID].reg[inst->dst_index] = statsBlocked->regsPerThread[threadID].reg[inst->src1_index] - statsBlocked->regsPerThread[threadID].reg[inst->src2_index_imm];
				break;
			case CMD_SUBI:
				threads[threadID].sbTimer = 1;
				src2 = (inst->isSrc2Imm) ? (inst->src2_index_imm) : (statsBlocked->regsPerThread[threadID].reg[inst->src2_index_imm]);
				statsBlocked->regsPerThread[threadID].reg[inst->dst_index] = statsBlocked->regsPerThread[threadID].reg[inst->src1_index] - src2;
				break;
			default:
				break;
			}
			// decreasing timers, one cycle is finished, and moving to next inst
			sbTimerHandler(threads, numOfThreads, 1);
			threads[currThreadID].instId++;
		}
		else { //if the thread is not ready yet just decrease timers (idle)
			sbTimerHandler(threads, numOfThreads, 1);
		}
		//increase cycle count
		statsBlocked->cyclesNum ++;
	}
	// when the simulation is ended, the last cmd was a HULT and switch penalty was payed but no switching is taking place
	statsBlocked->cyclesNum -= switchTime;
}

int getNextThreadFine(int threadID, ThreadData* threads, int numOfThreads){
	int nextThreadID = RRpolicy(threadID, numOfThreads); //starting the search from next thread because always switching
	int minTimer;
	int minTimerThread = -1;
	for(int i = 0; i < numOfThreads; i++){
		if(!threads[nextThreadID].done){
			minTimer = threads[nextThreadID].sbTimer;
			minTimerThread = nextThreadID;
			break;
		}
		nextThreadID = RRpolicy(nextThreadID, numOfThreads);
	}
	nextThreadID = RRpolicy(nextThreadID, numOfThreads);
	for(int i = 0; i < numOfThreads; i++){
		if(!threads[nextThreadID].done && threads[nextThreadID].sbTimer < minTimer){
			minTimer  = threads[nextThreadID].sbTimer;
			minTimerThread = nextThreadID;
		}
		nextThreadID = RRpolicy(nextThreadID, numOfThreads);
	}

	return minTimerThread;
}

//similar to BLOCKED
void CORE_FinegrainedMT() {
	Instruction* inst = new Instruction();
	int numOfThreads = SIM_GetThreadsNum();
	ThreadData* threads = new ThreadData[numOfThreads];
	for(int i = 0; i < numOfThreads; i++){
		threads[i].threadId = i;
		threads[i].sbTimer = 0;
		threads[i].done = false;
		threads[i].instId = 0;
	}
	int threadID = 0;
	Stats* stats_init = new Stats(numOfThreads);
	for(int i = 0; i < numOfThreads; i++){
		for(int j = 0; j < NUM_REGS; j++){
			stats_init->regsPerThread[i].reg[j] = 0;
		}
	}
	statsFine = stats_init;
	int src2;
	int mem_read;
	int addr;
	int val;
	int currThreadID;

	while(isRunningThreads(threads, numOfThreads)){
		if(threads[threadID].sbTimer == 0){
			statsFine->instNum++;
			SIM_MemInstRead(threads[threadID].instId, inst, threadID);

			currThreadID = threadID;

			switch (inst->opcode){
			case CMD_HALT:
				threads[threadID].sbTimer = 1;
				threads[threadID].done = true;
				break;
			case CMD_LOAD:
				src2 = (inst->isSrc2Imm) ? (inst->src2_index_imm) : (statsFine->regsPerThread[threadID].reg[inst->src2_index_imm]);
				threads[threadID].sbTimer = SIM_GetLoadLat() + 1;
				addr = statsFine->regsPerThread[threadID].reg[inst->src1_index] + src2;
				SIM_MemDataRead(addr , &mem_read);
				statsFine->regsPerThread[threadID].reg[inst->dst_index] = mem_read;
				break;
			case CMD_STORE:
				src2 = (inst->isSrc2Imm) ? (inst->src2_index_imm) : (statsFine->regsPerThread[threadID].reg[inst->src2_index_imm]);
				threads[threadID].sbTimer = SIM_GetStoreLat() + 1;
				addr = statsFine->regsPerThread[threadID].reg[inst->dst_index] + src2;
				val = statsFine->regsPerThread[threadID].reg[inst->src1_index];
				SIM_MemDataWrite(addr, val);
				break;
			case CMD_ADD:
				threads[threadID].sbTimer = 1;
				statsFine->regsPerThread[threadID].reg[inst->dst_index] = statsFine->regsPerThread[threadID].reg[inst->src1_index] + statsFine->regsPerThread[threadID].reg[inst->src2_index_imm];
				break;
			case CMD_ADDI:
				threads[threadID].sbTimer = 1;
				src2 = (inst->isSrc2Imm) ? (inst->src2_index_imm) : (statsFine->regsPerThread[threadID].reg[inst->src2_index_imm]);
				statsFine->regsPerThread[threadID].reg[inst->dst_index] = statsFine->regsPerThread[threadID].reg[inst->src1_index] + src2;
				break;
			case CMD_SUB:
				threads[threadID].sbTimer = 1;
				statsFine->regsPerThread[threadID].reg[inst->dst_index] = statsFine->regsPerThread[threadID].reg[inst->src1_index] - statsFine->regsPerThread[threadID].reg[inst->src2_index_imm];
				break;
			case CMD_SUBI:
				threads[threadID].sbTimer = 1;
				src2 = (inst->isSrc2Imm) ? (inst->src2_index_imm) : (statsFine->regsPerThread[threadID].reg[inst->src2_index_imm]);
				statsFine->regsPerThread[threadID].reg[inst->dst_index] = statsFine->regsPerThread[threadID].reg[inst->src1_index] - src2;
				break;
			default:
				break;
			}
			sbTimerHandler(threads, numOfThreads, 1);
			threadID = getNextThreadFine(threadID, threads, numOfThreads);
			threads[currThreadID].instId++;
		}
		else{
			sbTimerHandler(threads, numOfThreads, 1);
		}
		statsFine->cyclesNum ++;
	}
}

double CORE_BlockedMT_CPI(){
	return (double)statsBlocked->cyclesNum/(double)statsBlocked->instNum;
}

double CORE_FinegrainedMT_CPI(){
	return (double)statsFine->cyclesNum/(double)statsFine->instNum;
}

void CORE_BlockedMT_CTX(tcontext* context, int threadid) {
	for(int j = 0; j < NUM_REGS; j++){
		context[threadid].reg[j] = statsBlocked->regsPerThread[threadid].reg[j];
	}
}

void CORE_FinegrainedMT_CTX(tcontext* context, int threadid) {
	for(int j = 0; j < NUM_REGS; j++){
		context[threadid].reg[j] = statsFine->regsPerThread[threadid].reg[j];
	}
}
