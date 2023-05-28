/* 046267 Computer Architecture - Winter 20/21 - HW #2 */

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cmath>
#include <stdint.h>

#define ADDR_SIZE 32

using std::FILE;
using std::string;
using std::cout;
using std::endl;
using std::cerr;
using std::ifstream;
using std::stringstream;

class Entry{
public:
	int offset;
	int tag;
	bool valid;
	bool dirty;
	int LRU;
	int address;

	Entry(int LRU){
		this->offset = 0;
		this->tag = 0;
		this->valid = false;
		this->dirty = false;
		this->LRU = LRU;
		this->address = 0;
	}

};

class Cache{
public:
	int cacheID;
	int size;
	int cycles;
	int num_ways;
	int blockSize;
	int blockAmount;
	int entryAmount;
	bool writeAlloc;
	int offsetSize;
	int tagSize;
	int setSize;
	std::vector<Entry>* ways;

	Cache(int size, int cycles, int num_ways, int blockSize, bool writeAlloc, int cacheID){
		this->cacheID = cacheID;
		this->size = size;
		this->cycles = cycles;
		this->num_ways = num_ways;
		this->blockSize = blockSize;
		this->blockAmount = size / blockSize;
		this->entryAmount = this->blockAmount / num_ways;
		this->writeAlloc = writeAlloc;
		this->offsetSize = log2(blockSize);
		this->setSize = log2(this->entryAmount);
		this->tagSize = ADDR_SIZE - this->offsetSize - this->setSize;
		this->ways = new std::vector<Entry>[num_ways];
		for(int i=0; i < num_ways; i++){
			this->ways[i] =  std::vector<Entry>(this->entryAmount, Entry(i));
		}
	}

	Cache(const Cache& other){
		this->cacheID = other.cacheID;
		this->size = other.size;
		this->cycles = other.cycles;
		this->num_ways = other.num_ways;
		this->blockSize = other.blockSize;
		this->blockAmount = other.blockAmount;
		this->entryAmount = other.entryAmount;
		this->writeAlloc = other.writeAlloc;
		this->offsetSize = other.offsetSize;
		this->setSize = other.setSize;
		this->tagSize = other.tagSize;
		this->ways = new std::vector<Entry>[this->num_ways];
		for(int i=0 ; i < this->num_ways; i++){
			this->ways[i] = other.ways[i];
		}
	}

	Cache operator=(const Cache& other){
		this->cacheID = other.cacheID;
		this->size = other.size;
		this->cycles = other.cycles;
		this->num_ways = other.num_ways;
		this->blockSize = other.blockSize;
		this->blockAmount = other.blockAmount;
		this->entryAmount = other.entryAmount;
		this->writeAlloc = other.writeAlloc;
		this->offsetSize = other.offsetSize;
		this->setSize = other.setSize;
		this->tagSize = other.tagSize;
		delete [] this->ways;
		this->ways = new std::vector<Entry>[this->num_ways];
		for(int i=0 ; i < this->num_ways; i++){
			this->ways[i] = other.ways[i];
		}

		return *this;
	}

	void addressHandler(int address, int* tag, int* set, int* offset){
		int offset_mask = 1;
		int tag_mask = 1;
		int set_mask = 1;

		for(int i=1; i < this->offsetSize; i++){
			offset_mask = (offset_mask << 1) + 1;
		}

		for(int i=1; i < this->setSize; i++){
			set_mask = (set_mask << 1) + 1;
		}

		for(int i=1; i < this->tagSize; i++){
			tag_mask = (tag_mask << 1) + 1;
		}

		*tag = (address >> (this->offsetSize + this->setSize)) & tag_mask;
		*set = (this->setSize == 0) ? 0 : ((address >> (this->offsetSize)) & set_mask);
		*offset = address & offset_mask;
	}

	bool exists(int set, int tag){

		for(int i=0; i < this->num_ways; i++){
			if(this->ways[i][set].valid && this->ways[i][set].tag == tag){
				return true;
			}
		}

		return false;

	}

	int getLRU(int set){
		for(int i = 0; i < this->num_ways; i++){
			if(this->ways[i][set].LRU == 0){
				return i;
			}
		}

		return -1;
	}

	void updateLRU(int set, int tag){

		int idxInSet;
		int currLRU;
		for(int i =0; i < this->num_ways; i++){
			if(this->ways[i][set].tag == tag){
				idxInSet = i;
				break;
			}
		}
		currLRU = this->ways[idxInSet][set].LRU;
		this->ways[idxInSet][set].LRU = num_ways -1;
		for(int i = 0 ; i < this->num_ways; i++){
			if(i != idxInSet && this->ways[i][set].LRU > currLRU){
				this->ways[i][set].LRU--;
			}
		}
	}

	int findEmptyInSet(int set){
		for(int i = 0; i < this->num_ways; i++){
			if(this->ways[i][set].valid == false){
				return i;
			}
		}

		return -1;
	}

//	bool updateCache(int address, bool write, Entry* evicted, int* evictedSet, bool* is_evicted){
//		int set;
//		int tag;
//		int offset;
//
//		this->addressHandler(address, &tag, &set, &offset);
//
//		int emptyInSet = this->findEmptyInSet(set);
//		int oldestLRU = this->getOldestLRU(set);
//		int idxInSet;
//		bool lowerNeedsUpdate = false;
//
//		if(emptyInSet != -1){
//			idxInSet = emptyInSet;
//		}
//		else{
//			idxInSet = oldestLRU;
//			*evicted = this->ways[oldestLRU][set];
//			*evictedSet = set;
//			if(this->ways[oldestLRU][set].dirty){
//				lowerNeedsUpdate = true;
//			}
//		}
//
//		this->ways[idxInSet][set].valid = true;
//		this->ways[idxInSet][set].tag = tag;
//		this->ways[idxInSet][set].offset = offset;
//		this->updateLRU(address);
//		if(write){
//			this->ways[idxInSet][set].dirty = true;
//		}
//		else{
//			this->ways[idxInSet][set].dirty = false;
//		}
//
//		return lowerNeedsUpdate;
//	}

	bool findEmpty(int address, int* emptyIdx){
		int set;
		int tag;
		int offset;

		this->addressHandler(address, &tag, &set, &offset);

		for(int i = 0; i < this->num_ways; i++){
			if(this->ways[i][set].valid == false){
				*emptyIdx = i;
				return true;
			}
		}
		return false;
	}

	Entry invalidate(int address){
		int tag;
		int set;
		int offset;

		this->addressHandler(address, &tag, &set, &offset);

		int oldestLRU;
		for(int i = 0; i < this->num_ways; i++){
			if(this->ways[i][set].LRU == 0){
				oldestLRU = i;
			}
		}

		ways[oldestLRU][set].valid = false;
		ways[oldestLRU][set].dirty = false;

		return ways[oldestLRU][set];
	}

	bool isEmptySpace(int set){
		for (int i = 0; i<this->num_ways; i++){
			if(this->ways[i][set].valid == false){
				return true;
			}
		}
		return false;
	}

	void addToCache(int set, int tag, int address){
		int  emptyIdx;
		for (int i = 0; i<this->num_ways; i++){
			if(this->ways[i][set].valid == false){
				emptyIdx = i;
				break;
			}
		}
		this->ways[emptyIdx][set].valid = true;
		this->ways[emptyIdx][set].tag= tag;
		this->ways[emptyIdx][set].dirty = false;
		this->ways[emptyIdx][set].address = address;
	}

	void remove(int set, int LRU){
		this->ways[LRU][set].dirty = false;
		this->ways[LRU][set].valid = false;
	}
};

class SimResults{
public:
	uint32_t L1_hits;
	uint32_t L1_misses;
	uint32_t L2_hits;
	uint32_t L2_misses;
	uint32_t mem_access;
	uint32_t mem_cycels;

	SimResults(uint32_t mem_cycles){
		this->L1_hits = 0;
		this->L1_misses = 0;
		this->L2_hits = 0;
		this->L2_misses = 0;
		this->mem_access = 0;
		this->mem_cycels = mem_cycles;
	}
};

Cache L1 = Cache(0,0,1,1,false,0);
Cache L2 = Cache(0,0,1,1,false,0);
SimResults results =  SimResults(0);

void initSim(int L1_size, int L2_size, int L1_ways, int L2_ways, int L1_cycles, int L2_cycles, bool writeAlloc, int blockSize, int mem_cycles){
	Cache L1_init = Cache(pow(2,L1_size), L1_cycles, pow(2,L1_ways), pow(2,blockSize), writeAlloc, 1);
	Cache L2_init = Cache(pow(2,L2_size), L2_cycles, pow(2,L2_ways), pow(2,blockSize), writeAlloc, 2);
	L1 = L1_init;
	L2 = L2_init;
	SimResults results_init = SimResults(mem_cycles);
	results = results_init;
}



void cacheMemHandler(int operation, int address){

	//TODO :: maby no need to seperate operation

	int L1tag, L1set, L1offset;
	int L2tag, L2set, L2offset;
	int LRU1;
	int LRU2;

	L1.addressHandler(address, &L1tag, &L1set, &L1offset);
	L2.addressHandler(address, &L2tag, &L2set, &L2offset);

	if(L1.exists(L1set, L1tag)){
		results.L1_hits++;
		L1.updateLRU(L1set, L1tag);
		if(operation == 'w'){
			for(int i = 0; i < L1.num_ways; i++){
				if(L1.ways[i][L1set].tag == L1tag){
					L1.ways[i][L1set].dirty = true;
					break;
				}
			}
		}
	}
	else{
		results.L1_misses++;
		if(L2.exists(L2set, L2tag)){
			results.L2_hits++;
			L2.updateLRU(L2set, L2tag);
			if((operation == 'w' && L1.writeAlloc == true) || (operation == 'r')){
				if(L1.isEmptySpace(L1set)){
					L1.addToCache(L1set, L1tag,address);
					L1.updateLRU(L1set, L1tag);
				}
				else{
					LRU1 = L1.getLRU(L1set);
					if(L1.ways[LRU1][L1set].dirty == true){
						int dirtySet,dirtyTag,dirtyOffset;
						L2.addressHandler(L1.ways[LRU1][L1set].address, &dirtyTag, &dirtySet, &dirtyOffset);
						for(int i = 0; i < L2.num_ways; i++){
							if(L2.ways[i][dirtySet].tag == dirtyTag){
								L2.ways[i][dirtySet].dirty = true;
								L2.updateLRU(dirtySet,dirtyTag);
								break;
							}
						}
					}
					L1.remove(L1set,LRU1);
					L1.addToCache(L1set,L1tag,address);
					L1.updateLRU(L1set,L1tag);
				}
				if(operation == 'w'){
					for(int i = 0; i < L1.num_ways; i++){
						if(L1.ways[i][L1set].tag == L1tag){
							L1.ways[i][L1set].dirty = true;
							break;
						}
					}
				}
			}
		}
		else{
			results.L2_misses++;
			results.mem_access++;
			if((operation == 'w' && L1.writeAlloc == true) || (operation = 'r')){
				if(L2.isEmptySpace(L2set)){
					L2.addToCache(L2set,L2tag, address);
					L2.updateLRU(L2set,L2tag);
				}
				else{
					LRU2 = L2.getLRU(L2set);
					int removedSet, removedTag, removedOffset;
					L1.addressHandler(L2.ways[LRU2][L2set].address, &removedTag, &removedSet, &removedOffset);
					if(L1.exists(removedSet, removedTag)){
						int idx;
						for(int i = 0; i < L1.num_ways; i++){
							if(L1.ways[i][removedSet].tag == removedTag){
								idx = i;
								break;
							}
						}
						if(L1.ways[idx][removedSet].dirty == true){
							L2.ways[LRU2][L2set].dirty = true;
							L2.updateLRU(removedSet, removedTag);
						}
						L1.remove(removedSet,idx);
					}
					L2.remove(L2set, LRU2);
					L2.addToCache(L2set, L2tag, address);
					L2.updateLRU(L2set, L2tag);
				}


				if(L1.isEmptySpace(L1set)){
					L1.addToCache(L1set, L1tag,address);
					L1.updateLRU(L1set,L1tag);
				}
				else{
					LRU1 = L1.getLRU(L1set);
					if(L1.ways[LRU1][L1set].dirty == true){
						int dirtySet,dirtyTag,dirtyOffset;
						L2.addressHandler(L1.ways[LRU1][L1set].address, &dirtyTag, &dirtySet, &dirtyOffset);
						for(int i = 0; i < L2.num_ways; i++){
							if(L2.ways[i][dirtySet].tag == dirtyTag){
								L2.ways[i][dirtySet].dirty = true;
								L2.updateLRU(dirtySet, dirtyTag);
								break;
							}
						}
					}
					L1.remove(L1set,LRU1);
					L1.addToCache(L1set,L1tag,address);
					L1.updateLRU(L1set, L1tag);
				}
				if(operation == 'w'){
					for(int i = 0; i < L1.num_ways; i++){
						if(L1.ways[i][L1set].tag == L1tag){
							L1.ways[i][L1set].dirty = true;
							break;
						}
					}
				}
			}
		}
	}
}

int main(int argc, char **argv) {

	if (argc < 19) {
		cerr << "Not enough arguments" << endl;
		return 0;
	}

	// Get input arguments

	// File
	// Assuming it is the first argument
	char* fileString = argv[1];
	ifstream file(fileString); //input file stream
	string line;
	if (!file || !file.good()) {
		// File doesn't exist or some other error
		cerr << "File not found" << endl;
		return 0;
	}

	unsigned MemCyc = 0, BSize = 0, L1Size = 0, L2Size = 0, L1Assoc = 0,
			L2Assoc = 0, L1Cyc = 0, L2Cyc = 0, WrAlloc = 0;

	for (int i = 2; i < 19; i += 2) {
		string s(argv[i]);
		if (s == "--mem-cyc") {
			MemCyc = atoi(argv[i + 1]);
		} else if (s == "--bsize") {
			BSize = atoi(argv[i + 1]);
		} else if (s == "--l1-size") {
			L1Size = atoi(argv[i + 1]);
		} else if (s == "--l2-size") {
			L2Size = atoi(argv[i + 1]);
		} else if (s == "--l1-cyc") {
			L1Cyc = atoi(argv[i + 1]);
		} else if (s == "--l2-cyc") {
			L2Cyc = atoi(argv[i + 1]);
		} else if (s == "--l1-assoc") {
			L1Assoc = atoi(argv[i + 1]);
		} else if (s == "--l2-assoc") {
			L2Assoc = atoi(argv[i + 1]);
		} else if (s == "--wr-alloc") {
			WrAlloc = atoi(argv[i + 1]);
		} else {
			cerr << "Error in arguments" << endl;
			return 0;
		}
	}

	initSim(L1Size, L2Size, L1Assoc, L2Assoc, L1Cyc, L2Cyc, WrAlloc, BSize, MemCyc);

	while (getline(file, line)) {

		stringstream ss(line);
		string address;
		char operation = 0; // read (R) or write (W)
		if (!(ss >> operation >> address)) {
			// Operation appears in an Invalid format
			cout << "Command Format error" << endl;
			return 0;
		}

		// DEBUG - remove this line
//		cout << "operation: " << operation;

		string cutAddress = address.substr(2); // Removing the "0x" part of the address

		// DEBUG - remove this line
//		cout << ", address (hex)" << cutAddress;

		unsigned long int num = 0;
		num = strtoul(cutAddress.c_str(), NULL, 16);

//		 DEBUG - remove this line
//		cout << " (dec) " << num << endl;

		cacheMemHandler(operation,num);

	}

	double L1MissRate = ((double)(results.L1_misses)) / ((double)(results.L1_hits + results.L1_misses));
	double L2MissRate = ((double)(results.L2_misses)) / ((double)(results.L2_hits + results.L2_misses));
	double avgAccTime = ((double)((results.L1_hits + results.L1_misses)*L1.cycles + results.L1_misses*L2.cycles + results.mem_access*results.mem_cycels)) / ((double)(results.L1_hits + results.L1_misses));


	printf("L1miss=%.03f ", L1MissRate);
	printf("L2miss=%.03f ", L2MissRate);
	printf("AccTimeAvg=%.03f\n", avgAccTime);

	return 0;
}
