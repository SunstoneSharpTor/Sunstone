#include <iostream>

#include "transpositionTable.h"

using namespace std;

TranspositionTable::~TranspositionTable() {
	delete[] m_table;
}

TranspositionTable::TranspositionTable(unsigned long long size) {
	unsigned long long maxNumEntries = size * 1024 * 1024 / sizeof(ttEntry);
	m_numEntries = 1;
	maxNumEntries = maxNumEntries >> 1;
	m_keySize = 64;
	while (maxNumEntries) {
		maxNumEntries = maxNumEntries >> 1;
		m_numEntries = m_numEntries << 1;
		m_keySize--;
	}
	std::cout << m_numEntries << std::endl;
	
	m_table = new ttEntry[m_numEntries];

	for (unsigned long long i = 0; i < m_numEntries; i++) {
		m_table[i].flags = HashType::Beta;
		m_table[i].depth = 0;
	}
}

void TranspositionTable::recordHash(uint64_t hash, short depth, int eval, char flag, unsigned char bestMoveIndex) {
	uint64_t index = hash >> m_keySize;

	uint16_t newKey = hash;
	m_table[index].key = newKey;
	m_table[index].depth = depth;
	m_table[index].bestMoveIndex = bestMoveIndex;
	m_table[index].flags = flag;
	m_table[index].eval = eval;
}

bool TranspositionTable::probeHash(int* eval, uint64_t hash , short depth, int alpha, int beta, unsigned char* bestMoveIndex) {
	uint64_t index = hash >> m_keySize;
	uint64_t keyCheck = hash << 48 >> 48;
	if (m_table[index].key == keyCheck) {
		*bestMoveIndex = m_table[index].bestMoveIndex;
		if (m_table[index].depth >= depth) {
			if (m_table[index].flags == HashType::Exact) {
				*eval = m_table[index].eval;
				return true;
			}
			if ((m_table[index].flags == HashType::Alpha) && (m_table[index].eval <= alpha)) {
				*eval = alpha;
				return true;
			}
			if ((m_table[index].flags == HashType::Beta) && (m_table[index].eval >= beta)) {
				*eval = beta;
				return true;
			}
		}
		else {
			if (m_table[index].flags != HashType::Beta) {
				*eval = m_table[index].bestMoveIndex;
				return false;
			}
		}
	}

	return false;
}