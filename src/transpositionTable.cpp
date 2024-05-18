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
	
	m_table = new ttEntry[m_numEntries];

	for (unsigned long long i = 0; i < m_numEntries; i++) {
		m_table[i].flags = HashType::Beta;
		m_table[i].depth = 0;
	}
}

void TranspositionTable::recordHash(uint64_t hash, short depth, int value, char flag, unsigned char bestMoveIndex) {
	uint64_t index = hash >> m_keySize;

	uint16_t newKey = hash;

	//if (!((m_table[index].flags == HashType::Exact) && (flag == HashType::Beta))) {
	//	if (((m_table[index].flags != HashType::Exact) && (flag == HashType::Exact)) || (depth >= m_table[index].depth)) {
			m_table[index].key = newKey;
			m_table[index].depth = depth;
			//m_table[index].bestMoveIndex = bestMoveIndex;
			m_table[index].flags = flag;
			m_table[index].value = value;
	//	}
	//}
}

bool TranspositionTable::probeHash(int* value, uint64_t hash , short depth, int alpha, int beta) {
	uint64_t index = hash >> m_keySize;
	uint64_t keyCheck = hash << 48 >> 48;
	if (m_table[index].key == keyCheck) {
		if (m_table[index].depth >= depth) {
			if (m_table[index].flags == HashType::Exact) {
				*value = m_table[index].value;
				return true;
			}
			if ((m_table[index].flags == HashType::Alpha) && (m_table[index].value <= alpha)) {
				*value = alpha;
				return true;
			}
			if ((m_table[index].flags == HashType::Beta) && (m_table[index].value >= beta)) {
				*value = beta;
				return true;
			}
		}
		else {
			if (m_table[index].flags != HashType::Beta) {
				*value = m_table[index].bestMoveIndex;
				return false;
			}
		}
	}

	//set the vaule to 255 as this represents that the transposition table can't provide information on move ordering
	*value = 255;
	return false;
}