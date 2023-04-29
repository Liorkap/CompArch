/* 046267 Computer Architecture - Winter 20/21 - HW #1                  */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include <vector>
#include <bitset>
#include <cmath>

const int INST_SIZE = 30;
const int FSM_STATE_SIZE = 2;
const int VALID_SIZE = 1;
enum FSM_STATES{
	SNT = 0,
	WNT = 1,
	WT  = 2,
	ST  = 3
};

SIM_stats stats;


void initHist(bool* history, unsigned historySize){
	for(unsigned i = 0; i < historySize; i++){
		history[i] = false;
	}
}

void initFsmTable(FSM_STATES* fsmTable, unsigned fsmState, unsigned historySize){
	for(int i = 0; i < pow(2,historySize); i++){
		fsmTable[i] = FSM_STATES(fsmState);
	}
}

class BP_ENTRY{
public:
	int  tag;
	int  target;
	bool* history;
	FSM_STATES* fsmTable;
	bool isBranch;
	bool isGlobalHist;
	bool isGlobalTable;

	BP_ENTRY(unsigned historySize,unsigned fsmState,bool isGlobalTable, bool isGlobalHist){
		isBranch = false;
		tag = 0;
		target = 0;
		this->isGlobalTable = isGlobalTable;
		this->isGlobalHist = isGlobalHist;
		if(!isGlobalHist){
			history = new bool [historySize];
			initHist(history, historySize);
		}
		if(!isGlobalTable){
			int fsmTableSize = pow(2,historySize);
			fsmTable = new FSM_STATES[fsmTableSize];
			initFsmTable(fsmTable, fsmState, historySize);
		}
	}

	~BP_ENTRY(){
		if(!isGlobalHist){
			delete [] history;
		}
		if(!isGlobalTable){
			delete [] fsmTable;
		}
	}
};



class BP{
public:

	std::vector<BP_ENTRY*> _btb;
	bool*               _gHistory;
	FSM_STATES*         _gFsmTable;
	unsigned            _btbSize;
	unsigned            _historySize;
	unsigned            _tagSize;
	unsigned            _fsmState;
	int                 _shared;
	int                 _btbIndexSize;
	bool                _isGlobalHist;
	bool                _isGlobalTable;

	BP(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared){
		for( unsigned i = 0; i < btbSize; i++){
			BP_ENTRY* new_BP_ENTRY = new BP_ENTRY(historySize,fsmState,isGlobalTable,isGlobalHist);
			_btb.push_back(new_BP_ENTRY);
		}
		_btbSize = btbSize;
		_historySize = historySize;
		_tagSize = tagSize;
		_fsmState = fsmState;
		_shared = Shared;
		_btbIndexSize = log2(btbSize);
		_isGlobalHist = isGlobalHist;
		_isGlobalTable = isGlobalTable;
		if(isGlobalHist){
			_gHistory = new bool[historySize];
			initHist(_gHistory, historySize);
		}
		if(isGlobalTable){
			int fsmTableSize = pow(2,historySize);
			_gFsmTable = new FSM_STATES[fsmTableSize];
			initFsmTable(_gFsmTable, fsmState, historySize);
		}
	}

	BP(const BP& other){
		_btb = other._btb;
		_gHistory = other._gHistory;
		_gFsmTable = other._gFsmTable;
		_btbSize = other._btbSize;
		_historySize = other._historySize;
		_tagSize = other._tagSize;
		_fsmState = other._fsmState;
		_fsmState = other._fsmState;
		_shared = other._shared;
		_btbIndexSize = other._btbIndexSize;
		_isGlobalHist = other._isGlobalHist;
		_isGlobalTable = other._isGlobalTable;
	}

	~BP(){
		if(_isGlobalHist){
			delete [] _gHistory;
		}
		if(_isGlobalTable){
			delete [] _gFsmTable;
		}
		for (unsigned i = 0; i < _btbSize; i++) {
			delete _btb[i];
		}
	}

	BP operator=(const BP& other){
		if(this != &other){
			_btb = other._btb;
			_gHistory = other._gHistory;
			_gFsmTable = other._gFsmTable;
			_btbSize = other._btbSize;
			_historySize = other._historySize;
			_tagSize = other._tagSize;
			_fsmState = other._fsmState;
			_fsmState = other._fsmState;
			_shared = other._shared;
			_btbIndexSize = other._btbIndexSize;
			_isGlobalHist = other._isGlobalHist;
			_isGlobalTable = other._isGlobalTable;
		}
		return *this;
	}
};

BP* branchPredictor = new BP(0,0,0,0,false,false,0);

int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared){
	stats.br_num = 0;
	stats.flush_num = 0;
	stats.size = 0;
	BP* newBranchPredictor = new BP(btbSize, historySize, tagSize, fsmState, isGlobalHist, isGlobalTable, Shared);
	if(newBranchPredictor == NULL){
		return -1;
	}
	branchPredictor = newBranchPredictor;
	return 0;
}

int shareHandler(int shared, unsigned historySize, uint32_t pc){
	int mask = 1;
	for(unsigned i = 1; i < historySize; i++){
		mask = (mask << 1) + 1;
	}
	if(shared == 0){
		return 0;
	}
	else if (shared == 1){
		return (pc >> 2) & mask;
	}
	else{
		return (pc >> 16) & mask;
	}
}

int tableIdxHandler(bool* histArr, unsigned historySize, bool isGlobalTable, int shared, uint32_t pc){
	int idx = 0;
	for(unsigned i = 0; i < historySize - 1; i++){
		idx += histArr[i];
		idx = idx << 1;
	}
	idx += histArr[historySize - 1];
	int shareMask;
	if(isGlobalTable){
		shareMask = shareHandler(shared,historySize, pc);
		idx = idx ^ shareMask;
	}
	return idx;
}

bool fsmStateMapping(FSM_STATES state){
	if(state == ST || state == WT){
		return true;
	}
	else{
		return false;
	}
}

int pc2btbIndex(uint32_t pc, int btbIdxSize){
	int mask = 1;
	for(int i = 1; i < btbIdxSize; i++){
		mask = (mask << 1) + 1;
	}
	return (pc >> 2) & mask;
}

int pc2tag(uint32_t pc, int btbIdxSize, unsigned tagSize){
	int mask = 1;
	for(unsigned i = 1; i < tagSize; i++){
		mask = (mask << 1) + 1;
	}
	return (pc >> (2 + btbIdxSize)) & mask;
}

bool BP_predict(uint32_t pc, uint32_t *dst){
	int btbIndex = pc2btbIndex(pc, branchPredictor->_btbIndexSize);
	int tag = pc2tag(pc, branchPredictor->_btbIndexSize, branchPredictor->_tagSize);
	BP_ENTRY* entry = branchPredictor->_btb[btbIndex];
	bool* history = (branchPredictor->_isGlobalHist) ? branchPredictor->_gHistory : entry->history;
	int tableIdx = tableIdxHandler(history,branchPredictor->_historySize, branchPredictor->_isGlobalTable, branchPredictor->_shared, pc);
	FSM_STATES* fsmTable = (branchPredictor->_isGlobalTable) ? branchPredictor->_gFsmTable : entry->fsmTable;
	if(entry->isBranch == true && entry->tag == tag && fsmStateMapping(fsmTable[tableIdx]) == true){
		*dst = entry->target;
		return true;
	}
	else{
		*dst = pc + 4;
		return false;
	}
}

void updateFSM(FSM_STATES* fsmTable, int tableIdx, bool isCorrect){
	if(fsmTable[tableIdx] == ST){
		fsmTable[tableIdx] = (isCorrect) ? ST : WT;
	}
	else if(fsmTable[tableIdx] == WT){
		fsmTable[tableIdx] = (isCorrect) ? ST : WNT;
	}
	else if(fsmTable[tableIdx] == WNT){
		fsmTable[tableIdx] = (isCorrect) ? SNT : WT;
	}
	else{
		fsmTable[tableIdx] = (isCorrect) ? SNT : WNT;
	}
}

void updateHist(bool* history, unsigned historySize, bool taken){
	for(unsigned i = 1 ; i < historySize ; i++){
		history[i-1] = history[i];
	}
	history[historySize -1] = taken;
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
	stats.br_num++;
	int btbIndex = pc2btbIndex(pc, branchPredictor->_btbIndexSize);
	int tag = pc2tag(pc, branchPredictor->_btbIndexSize, branchPredictor->_tagSize);
	BP_ENTRY* entry = branchPredictor->_btb[btbIndex];
	bool* history = (branchPredictor->_isGlobalHist) ? branchPredictor->_gHistory : entry->history;
	FSM_STATES* fsmTable = (branchPredictor->_isGlobalTable) ? branchPredictor->_gFsmTable : entry->fsmTable;
	if(entry->isBranch != true || entry->tag != tag){
		entry->isBranch = true;
		entry->tag = tag;
		if(!branchPredictor->_isGlobalHist){
			initHist(history, branchPredictor->_historySize);
		}
		if(!branchPredictor->_isGlobalTable){
			initFsmTable(fsmTable, branchPredictor->_fsmState, branchPredictor->_historySize);
		}
	}
	entry->target = targetPc;
	int tableIdx = tableIdxHandler(history,branchPredictor->_historySize, branchPredictor->_isGlobalTable, branchPredictor->_shared, pc);
	if(fsmStateMapping(fsmTable[tableIdx]) != taken){
		updateFSM(fsmTable,tableIdx,false);
	}
	else{
		updateFSM(fsmTable,tableIdx,true);
	}
	if(taken && targetPc != pred_dst){
		stats.flush_num++;
	}
	else if(!taken && pred_dst != (pc + 4)){
		stats.flush_num++;
	}
	updateHist(history, branchPredictor->_historySize, taken);
}

void BP_GetStats(SIM_stats *curStats){
	curStats->br_num = stats.br_num;
	curStats->flush_num = stats.flush_num;
	if(branchPredictor->_isGlobalHist && branchPredictor->_isGlobalTable){
		curStats->size = branchPredictor->_btbSize*(branchPredictor->_tagSize + INST_SIZE + VALID_SIZE) +
				branchPredictor->_historySize + FSM_STATE_SIZE*pow(2,branchPredictor->_historySize);
	}
	else if(!branchPredictor->_isGlobalHist && branchPredictor->_isGlobalTable){
		curStats->size = branchPredictor->_btbSize*(branchPredictor->_tagSize + INST_SIZE + VALID_SIZE + branchPredictor->_historySize) +
				FSM_STATE_SIZE*pow(2,branchPredictor->_historySize);
	}
	else if(branchPredictor->_isGlobalHist && !branchPredictor->_isGlobalTable){
		curStats->size = branchPredictor->_btbSize*(branchPredictor->_tagSize + INST_SIZE + VALID_SIZE + FSM_STATE_SIZE*pow(2,branchPredictor->_historySize)) +
						branchPredictor->_historySize;
	}
	else{
		curStats->size = branchPredictor->_btbSize*(branchPredictor->_tagSize + INST_SIZE + VALID_SIZE +
				pow(2,branchPredictor->_historySize)*FSM_STATE_SIZE + branchPredictor->_historySize);
	}
	delete branchPredictor;
	return;
}

