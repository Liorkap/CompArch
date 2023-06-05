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

// Cell in a set of the cache

class Cell{
public:
	int offset;
	int set;
	int tag;
	bool dirty;
	bool valid;
	int address;
	int LRU;

	Cell(){
		tag = 0;
		offset = 0;
		set = 0;
		dirty = false;
		valid = false;
		address = 0;
		LRU = 0;
	}
};


// An entry (row/det)
class Entry{
public:
	Cell* cells;
};


// GP Cache
class Cache{
public:
	Entry* entries;
	int cacheID;
	int size;
	int cycles;
	int numWays;
	int blockSize;
	int blockAmount;
	int entryAmount;
	bool writeAlloc;
	int offsetSize;
	int tagSize;
	int setSize;

	Cache(int size, int cycles, int numWays, int blockSize, bool writeAlloc, int cacheID){
		this->size = size;
		this->cycles = cycles;
		this->numWays = numWays;
		this->blockSize = blockSize;
		this->writeAlloc = writeAlloc;
		this->cacheID = cacheID;
		this->blockAmount = size/blockSize;
		this->entryAmount = blockAmount/numWays;
		this-> offsetSize = log2(blockSize);
		this->setSize = log2(entryAmount);
		this->tagSize = ADDR_SIZE - setSize - offsetSize;
		this->entries = new Entry[entryAmount];
		for(int i = 0; i < entryAmount; i++){
			entries[i].cells = new Cell[numWays];
			for(int j = 0; j < numWays; j++){
				entries[i].cells[j].LRU = j;
			}
		}
	}

	Cache(const Cache& other){
		this->size = other.size;
		this->cycles = other.cycles;
		this->numWays = other.numWays;
		this->blockSize = other.blockSize;
		this->writeAlloc = other.writeAlloc;
		this->cacheID = other.cacheID;
		this->blockAmount = other.blockAmount;
		this->entryAmount = other.entryAmount;
		this->offsetSize = other.offsetSize;
		this->setSize = other.setSize;
		this->tagSize = other.tagSize;
		this->entries = new Entry[entryAmount];
		for(int i = 0; i < entryAmount; i++){
			entries[i].cells = new Cell[numWays];
			for(int j = 0; j < numWays; j++){
				entries[i].cells[j].LRU = j;
			}
		}
	}

	Cache operator=(const Cache& other){
		this->cacheID = other.cacheID;
		this->size = other.size;
		this->cycles = other.cycles;
		this->numWays = other.numWays;
		this->blockSize = other.blockSize;
		this->blockAmount = other.blockAmount;
		this->entryAmount = other.entryAmount;
		this->writeAlloc = other.writeAlloc;
		this->offsetSize = other.offsetSize;
		this->setSize = other.setSize;
		this->tagSize = other.tagSize;
		delete [] this->entries;
		this->entries = new Entry[entryAmount];
		for(int i = 0; i < entryAmount; i++){
			this->entries[i].cells = new Cell[numWays];
			for(int j = 0; j < numWays; j++){
				this->entries[i].cells[j] = other.entries[i].cells[j];
			}
		}

		return *this;
	}

	//Extracts tag set and offset from address
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

	//returns true if tag exists in set
	bool exists(int tag, int set){
		for(int i = 0; i < numWays; i++){
			if(entries[set].cells[i].valid == true && entries[set].cells[i].tag == tag){
				return true;
			}
		}

		return false;
	}

	// updates LRU by tag and set
	void updateLRU(int tag, int set){
		int currLRU;
		for(int i =0; i < numWays; i++){
			if(entries[set].cells[i].tag == tag){
				currLRU = entries[set].cells[i].LRU;
				entries[set].cells[i].LRU = numWays - 1;
				break;
			}
		}
		for(int i = 0 ; i < numWays; i++){
			if((entries[set].cells[i].tag != tag || entries[set].cells[i].valid == false) && entries[set].cells[i].LRU > currLRU){
				entries[set].cells[i].LRU--;
			}
		}
	}

	// returns true if there is an empty cell in set
	bool isEmpty(int set, int* emptyCell){
		for( int i = 0; i < numWays; i++){
			if(entries[set].cells[i].valid == false){
				*emptyCell = i;
				return true;
			}
		}

		return false;
	}

	// evict by tag and set and update LRU
	void evict(int tag, int set, int* address){
		for(int i = 0; i < numWays; i++){
			if(entries[set].cells[i].tag == tag && entries[set].cells[i].valid == true){
				entries[set].cells[i].valid = false;
				for(int j = 0; j < numWays; j++){
					if(entries[set].cells[j].LRU < entries[set].cells[i].LRU){
						entries[set].cells[j].LRU++;
					}
				}
				entries[set].cells[i].LRU = 0;
				*address = entries[set].cells[i].address;
			}
			break;
		}
	}

	//evict by LRU
	bool evictLRU(int set, int*address){
		for(int i = 0; i < numWays; i++){
			if(entries[set].cells[i].LRU == 0 && entries[set].cells[i].valid == true){
				entries[set].cells[i].valid = false;
				*address = entries[set].cells[i].address;
				return entries[set].cells[i].dirty;
			}
		}
		return false;
	}


	//return the address of the LRU in set
	int getLRU(int set){
		for(int i = 0; i < numWays; i++){
			if(entries[set].cells[i].LRU == 0){
				return entries[set].cells[i].address;
			}
		}
		return 0;
	}

	// return true if tag in set is dirty
	bool isDirty(int tag, int set){
		for(int i = 0; i < numWays; i++){
			if(entries[set].cells[i].tag == tag){
				return entries[set].cells[i].dirty;
			}
		}
		return false;
	}

	// update tag in set to be dirty
	void updateDirty(int tag, int set){
		for(int i = 0; i < numWays; i++){
			if(entries[set].cells[i].tag == tag && entries[set].cells[i].valid == true){
				entries[set].cells[i].dirty = true;
				break;
			}
		}
	}
};


// the results of the simulation
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

//manages all the L1-L2-MEM access for read/write operations
class CacheMemHandler{
public:
	Cache* L1;
	Cache* L2;
	SimResults* results;

	CacheMemHandler(int L1_size, int L2_size, int L1_ways, int L2_ways, int L1_cycles, int L2_cycles, bool writeAlloc, int blockSize, int mem_cycles){
		L1 = new Cache(pow(2,L1_size), L1_cycles, pow(2,L1_ways), pow(2,blockSize), writeAlloc, 1);
		L2 = new Cache(pow(2,L2_size), L2_cycles, pow(2,L2_ways), pow(2,blockSize), writeAlloc, 2);
		results = new SimResults(mem_cycles);
	}

	//add block to L2 cache
	void addToL2(int tag, int set, int address){
		int emptyCell;
		int LRUAddress = L2->getLRU(set);
		int L1LRUtag, L1LRUset, L1LRUoffset;
		int dirtyL2Tag, dirtyL2Set, dirtyL2Offset, dirtyAddress;
		bool isDirtyL1 = false;
		// if no empty space in L2 in set then LRU from set should be evicted
		if(!L2->isEmpty(set, &emptyCell)){
			L1->addressHandler(LRUAddress, &L1LRUtag, &L1LRUset, &L1LRUoffset);
			//if exists also in L1 shoud be first evicted from there
			if(L1->exists(L1LRUtag, L1LRUset)){
				isDirtyL1 = L1->isDirty(L1LRUtag, L1LRUset);
				L1->evict(L1LRUtag,L1LRUset,&dirtyAddress);
			}
			// after L1 is handled, LRU from L2 can be evicted
			L2->evictLRU(set, &address);
			// get the new empty spot
			L2->isEmpty(set, &emptyCell);
		}
		// write to empty spot
		L2->entries[set].cells[emptyCell].tag = tag;
		L2->entries[set].cells[emptyCell].set = set;
		L2->entries[set].cells[emptyCell].address = address;
		L2->entries[set].cells[emptyCell].dirty = false;
		L2->entries[set].cells[emptyCell].valid = true;
		L2->updateLRU(tag,set);
		//if an eviction from L1 happened and it was dirty it should be written to L2
		if(isDirtyL1){
			L2->addressHandler(dirtyAddress, &dirtyL2Tag, &dirtyL2Set, &dirtyL2Offset);
			for(int i = 0; i < L2->numWays; i++){
				if(L2->entries[dirtyL2Set].cells[i].valid == true && L2->entries[dirtyL2Set].cells[i].tag == dirtyL2Tag){
					L2->entries[dirtyL2Set].cells[i].dirty = true;
					L2->updateLRU(dirtyL2Tag, dirtyL2Set);
				}
			}
		}
	}

	//add block to L1 cache
	void addToL1(int tag, int set, int address){
		int emptyCell;
		// if no empty space in L1 in set then  LRU from set should be evicted
		if(!L1->isEmpty(set, &emptyCell)){
			int dirtyL2Tag, dirtyL2Set, dirtyL2Offset, dirtyAddress;
			if(L1->evictLRU(set, &dirtyAddress)){
				L2->addressHandler(dirtyAddress, &dirtyL2Tag, &dirtyL2Set, &dirtyL2Offset);
				// if the evicted was dirty then write it to L2
				for(int i = 0; i < L2->numWays; i++){
					if(L2->entries[dirtyL2Set].cells[i].valid == true && L2->entries[dirtyL2Set].cells[i].tag == dirtyL2Tag){
						L2->entries[dirtyL2Set].cells[i].dirty = true;
						L2->updateLRU(dirtyL2Tag, dirtyL2Set);
					}
				}
			}
			// get new empty spot
			L1->isEmpty(set, &emptyCell);
		}
		//write to empty spot
		L1->entries[set].cells[emptyCell].tag = tag;
		L1->entries[set].cells[emptyCell].set = set;
		L1->entries[set].cells[emptyCell].address = address;
		L1->entries[set].cells[emptyCell].dirty = false;
		L1->entries[set].cells[emptyCell].valid = true;
		L1->updateLRU(tag,set);
	}

	// handles read cmd
	void readHandler(int address, int L1Tag, int L1Set, int L1Offset, int L2Tag, int L2Set, int L2Offset){
		// if in L1 then L1 hit update LRU
		if(L1->exists(L1Tag, L1Set)){
			results->L1_hits++;
			L1->updateLRU(L1Tag, L1Set);
		}
		// if not in L1 then L1 miss check L2
		else{
			results->L1_misses++;
			//if in L2 then L2 hit update LRU. print to L1
			if(L2->exists(L2Tag, L2Set)){
				results->L2_hits++;
				L2->updateLRU(L2Tag, L2Set);
				addToL1(L1Tag, L1Set, address);
			}
			// if miss on both then mem access and bring to both
			else{
				results->L2_misses++;
				results->mem_access++;
				addToL2(L2Tag, L2Set, address);
				addToL1(L1Tag, L1Set, address);
			}
		}
	}

	// hanles write cmd
	void writeHandler(int address, int L1Tag, int L1Set, int L1Offset, int L2Tag, int L2Set, int L2Offset){
		//if in L1 then L1 hit update LRU and update dirty
		if(L1->exists(L1Tag, L1Set)){
			results->L1_hits++;
			L1->updateDirty(L1Tag, L1Set);
			L1->updateLRU(L1Tag, L1Set);
		}
		// iof not then L1 miss check L2
		else{
			results->L1_misses++;
			//if in L2 then L2 hit, update dirty and LRU and if writeAlloc then bring to L1
			if(L2->exists(L2Tag, L2Set)){
				results->L2_hits++;
				L2->updateDirty(L2Tag, L2Set);
				L2->updateLRU(L2Tag, L2Set); //diff
				if(L1->writeAlloc){
					addToL1(L1Tag, L1Set, address);
					L1->updateDirty(L1Tag, L1Set);
				}
			}
			// if miss on both then memory access and if write alloc bring to both and update dirty
			else{
				results->L2_misses++;
				results->mem_access++;
				if(L1->writeAlloc){
					addToL2(L2Tag, L2Set, address);
					L2->updateDirty(L2Tag, L2Set);
					addToL1(L1Tag, L1Set, address);
					L1->updateDirty(L1Tag, L1Set);
				}
			}
		}
	}

	//Handles cmds
	void cmdParser(char operation, int address){
		int L1Tag, L1Set, L1Offset;
		int L2Tag, L2Set, L2Offset;

		L1->addressHandler(address, &L1Tag, &L1Set, &L1Offset);
		L2->addressHandler(address, &L2Tag, &L2Set, &L2Offset);

		if(operation == 'r'){
			readHandler(address, L1Tag, L1Set, L1Offset, L2Tag, L2Set, L2Offset);
		}
		else if(operation == 'w'){
			writeHandler(address, L1Tag, L1Set, L1Offset, L2Tag, L2Set, L2Offset);
		}
	}
};

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

	// initialize the controller
	CacheMemHandler controller = CacheMemHandler(L1Size, L2Size, L1Assoc, L2Assoc, L1Cyc, L2Cyc, WrAlloc, BSize, MemCyc);

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

		// DEBUG - remove this line
//		cout << " (dec) " << num << endl;

		//handle cmd
		controller.cmdParser(operation, num);

	}

	double L1MissRate = ((double)(controller.results->L1_misses)) / ((double)(controller.results->L1_hits + controller.results->L1_misses));
	double L2MissRate = ((double)(controller.results->L2_misses)) / ((double)(controller.results->L2_hits + controller.results->L2_misses));
	double avgAccTime = ((double)((controller.results->L1_hits + controller.results->L1_misses)*controller.L1->cycles + controller.results->L1_misses*controller.L2->cycles + controller.results->mem_access*controller.results->mem_cycels)) / ((double)(controller.results->L1_hits + controller.results->L1_misses));


	printf("L1miss=%.03f ", L1MissRate);
	printf("L2miss=%.03f ", L2MissRate);
	printf("AccTimeAvg=%.03f\n", avgAccTime);

	return 0;
}
