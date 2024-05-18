#pragma once

#include <string>

#include "board.h"

using namespace std;

class Engine {
private:
	Board m_board;
	void iterativeDeepeningSearch(int* currentDepth, bool* cancelSearch, int* eval, unsigned char* bestMoveFrom, unsigned char* bestMoveTo, unsigned char* bestMoveFlags);

public:
	void receiveCommand(string command);
};