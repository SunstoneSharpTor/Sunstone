#pragma once

#include <string>

#include "board.h"
#include "search.h"

class Engine {
private:
	Board m_board;
    Search m_search;

	void iterativeDeepeningSearch(int time, int* currentDepth, bool* cancelSearch, int* eval, unsigned char* bestMoveFrom, unsigned char* bestMoveTo, unsigned char* bestMoveFlags);
	void printInfo(int timeSearched, int currentDepth, int eval);
	void work(bool* cancelSearch, unsigned char* from, unsigned char* to, unsigned char* flags, int depth, int* bestMoveNum, int* eval);

public:
	Engine() : m_board(), m_search(&m_board) {}
	void receiveCommand(std::string command);
};