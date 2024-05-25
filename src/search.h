#pragma once

#include "board.h"

class Search {
private:
    Board* m_board;
    TranspositionTable m_transpositionTable;
    long long m_numPositions;

    //ai
    int evaluate();
    int search(int depth, int plyFromRoot, int alpha, int beta, char numExtensions);
    int quiescenceSearch(int plyFromRoot, int alpha, int beta);
    void orderMoves(unsigned char* from, unsigned char* to, unsigned char* flags, unsigned int* moveScores, unsigned char numMoves, unsigned char ttBestMove);
public:
    Search(Board* board) : m_board(board), m_transpositionTable(256), m_numPositions(0) {}
    void rootSearch(bool* cancelSearch, unsigned char* from, unsigned char* to, unsigned char* flags, int depth, int* bestMoveNum, int* eval);
    bool checkForSingleLegalMove(unsigned char* from, unsigned char* to, unsigned char* flags);

    inline void resetNodeCount() {
        m_numPositions = 0;
    }
    inline long long getNodeCount() {
        return m_numPositions;
    }
};