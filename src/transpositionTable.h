#pragma once

#include <cstdint>

struct ttEntry {
	char flags;
	uint16_t key;
	short depth;
	unsigned char bestMoveIndex;
	int eval;
};

enum HashType {
	Exact = 0, Alpha = 1, Beta = 2
};

class TranspositionTable {
private:
	ttEntry* m_table;
	unsigned long long m_numEntries;
	char m_keySize;

public:
	TranspositionTable(unsigned long long size);

	~TranspositionTable();
	
	void recordHash(uint64_t hash, short depth, int value, char flag, unsigned char bestMoveIndex);

	bool probeHash(int* value, uint64_t hash, short depth, int alpha, int beta);
};