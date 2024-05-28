#include <algorithm>
#include <cstdint>
#include <limits>
#include <iostream>

#include "search.h"
#include "bitboard.h"
#include "board.h"
#include "constants.h"

int Search::evaluate() {
    uint64_t pieceBitboard = m_board->getPiecesBB(PieceType::Black - m_board->getTurn());

    int numEnemyPieces = 2;
    while (pieceBitboard) {
        popLSB(&pieceBitboard);
        numEnemyPieces++;
    }

    int earlyGame = numEnemyPieces;
    int endGame = 16 - earlyGame;

    int evaluation = 0;
    int piecePositionEval = 0;

    for (int typeOfPiece = 2; typeOfPiece < 12; typeOfPiece++) {
        pieceBitboard = m_board->getPiecesBB(typeOfPiece);
        while (pieceBitboard) {
            char square = popLSB(&pieceBitboard);
            piecePositionEval += constants::PIECE_SQUARE_TABLES_EARLY_GAME[typeOfPiece * 64 + square] * earlyGame;
            piecePositionEval += constants::PIECE_SQUARE_TABLES_END_GAME[typeOfPiece * 64 + square] * endGame;
            evaluation += constants::PIECE_VALUES[typeOfPiece];
        }
    }
    evaluation += piecePositionEval / 16;
    m_numPositions++;
    return evaluation + (-2 * evaluation * m_board->getTurn());
}

int Search::search(bool* cancelSearch, int depth, int plyFromRoot, int alpha, int beta, char numExtensions) {
    if (*cancelSearch) {
        return 0;
    }
    if (depth == 0) {
        return quiescenceSearch(plyFromRoot, alpha, beta);
    }
    
    int TTEval;
    unsigned char bestMove = 255;
    if (m_transpositionTable.probeHash(&TTEval, m_board->getZobristKey(m_board->getPly()), depth, alpha, beta, &bestMove)) {
        return findMateValue(TTEval, plyFromRoot);
    }

    char hashType = HashType::Alpha;

    unsigned char numLegalMoves;
    unsigned char legalMovesFrom[256];
    unsigned char legalMovesTo[256];
    unsigned char legalMovesFlags[256];
    unsigned int legalMovesOrder[256];

    m_board->getLegalMoves(&numLegalMoves, legalMovesFrom, legalMovesTo, legalMovesFlags);

    const int mateValue = std::numeric_limits<int>::max() / 2 - plyFromRoot - 1;

    if (numLegalMoves == 0) {
        if (m_board->inCheck()) {
            return -mateValue;
        }
        return 0;
    }

    orderMoves(legalMovesFrom, legalMovesTo, legalMovesFlags, legalMovesOrder, numLegalMoves, bestMove);

    bool extension = (numExtensions < 12) && m_board->inCheck();

    for (int moveNum = 0; moveNum < numLegalMoves; moveNum++) {
        unMakeMoveState prevMoveState;
        m_board->getUnMakeMoveState(&prevMoveState, legalMovesTo[legalMovesOrder[moveNum]]);
        m_board->makeMove(legalMovesFrom[legalMovesOrder[moveNum]], legalMovesTo[legalMovesOrder[moveNum]], legalMovesFlags[legalMovesOrder[moveNum]]);
        int evaluation;

        //detect 50 move rule
        if (m_board->get50MoveRule() >= 100) {
            evaluation = 0;
        }
        else {
            //detect a repetition
            int i = m_board->getPly() - 2;
            while ((i >= m_board->getLastTakeOrPawnMove()) && (m_board->getZobristKey(i) != m_board->getZobristKey(m_board->getPly()))) {
                i -= 2;
            }
            if (m_board->getZobristKey(i) == m_board->getZobristKey(m_board->getPly())) {
                evaluation = 0;
            }
            else {
                bool thisMoveExtension = extension || ((numExtensions < 12) && (m_board->getPiece(legalMovesTo[legalMovesOrder[moveNum]]) == (PieceType::BlackPawn - m_board->getTurn())) && ((legalMovesTo[legalMovesOrder[moveNum]] >= 48) || (legalMovesTo[legalMovesOrder[moveNum]] <= 15)));

                bool needsFullSearch = true;

                if ((moveNum >= 3) && (!thisMoveExtension) && (depth >= 3) && (prevMoveState.takenPieceType == PieceType::All)) {
                    evaluation = -search(cancelSearch, depth - 2 + thisMoveExtension, plyFromRoot + 1, -beta, -alpha, numExtensions + thisMoveExtension);

                    needsFullSearch = evaluation > alpha;
                }

                if (needsFullSearch) {
                    evaluation = -search(cancelSearch, depth - 1 + thisMoveExtension, plyFromRoot + 1, -beta, -alpha, numExtensions + thisMoveExtension);
                }
            }
        }

        m_board->unMakeMove(legalMovesFrom[legalMovesOrder[moveNum]], legalMovesTo[legalMovesOrder[moveNum]], legalMovesFlags[legalMovesOrder[moveNum]], &prevMoveState);
        if (*cancelSearch) {
            return 0;
        }
        if (evaluation >= beta) {
            m_transpositionTable.recordHash(m_board->getZobristKey(m_board->getPly()), depth, findMateDist(beta, plyFromRoot), HashType::Beta, legalMovesOrder[moveNum]);
            return beta;
        }
        
        if (evaluation > alpha) {
            bestMove = legalMovesOrder[moveNum];
            hashType = HashType::Exact;
            alpha = evaluation;
        }
    }

    m_transpositionTable.recordHash(m_board->getZobristKey(m_board->getPly()), depth, findMateDist(alpha, plyFromRoot), hashType, bestMove);
    return alpha;
}

int Search::quiescenceSearch(int plyFromRoot, int alpha, int beta) {
    int evaluation = evaluate();

    if (evaluation >= beta) {
        return beta;
    }

    alpha = max(alpha, evaluation);

    unsigned char numLegalMoves;
    unsigned char legalMovesFrom[256];
    unsigned char legalMovesTo[256];
    unsigned char legalMovesFlags[256];
    unsigned int legalMovesOrder[256];

    m_board->getCaptureMoves(&numLegalMoves, legalMovesFrom, legalMovesTo, legalMovesFlags);

    orderMoves(legalMovesFrom, legalMovesTo, legalMovesFlags, legalMovesOrder, numLegalMoves, 255);

    for (int moveNum = 0; moveNum < numLegalMoves; moveNum++) {
        unMakeMoveState prevMoveState;
        m_board->getUnMakeMoveState(&prevMoveState, legalMovesTo[legalMovesOrder[moveNum]]);

        m_board->makeMove(legalMovesFrom[legalMovesOrder[moveNum]], legalMovesTo[legalMovesOrder[moveNum]], legalMovesFlags[legalMovesOrder[moveNum]]);
        int evaluation = -quiescenceSearch(plyFromRoot + 1, -beta, -alpha);
        m_board->unMakeMove(legalMovesFrom[legalMovesOrder[moveNum]], legalMovesTo[legalMovesOrder[moveNum]], legalMovesFlags[legalMovesOrder[moveNum]], &prevMoveState);

        if (evaluation >= beta) {
            return beta;
        }
        alpha = max(alpha, evaluation);
    }

    return alpha;
}

void Search::rootSearch(bool* cancelSearch, unsigned char* from, unsigned char* to, unsigned char* flags, int depth, int* bestMoveNum, int* eval) {
    int alpha = -std::numeric_limits<int>::max() / 2;
    int beta = std::numeric_limits<int>::max() / 2;

    unsigned char bestMove = 255;
    int TTEval;
    m_transpositionTable.probeHash(&TTEval, m_board->getZobristKey(m_board->getPly()), depth, alpha, beta, &bestMove);
    
    unsigned char numLegalMoves;
    unsigned char legalMovesFrom[256];
    unsigned char legalMovesTo[256];
    unsigned char legalMovesFlags[256];
    unsigned int legalMovesOrder[256];

    m_board->getLegalMoves(&numLegalMoves, legalMovesFrom, legalMovesTo, legalMovesFlags);

    orderMoves(legalMovesFrom, legalMovesTo, legalMovesFlags, legalMovesOrder, numLegalMoves, bestMove);
    
    //unsigned char newBestMoveIndex = 255;

    for (int moveNum = 0; moveNum < numLegalMoves; moveNum++) {
        unMakeMoveState prevMoveState;
        m_board->getUnMakeMoveState(&prevMoveState, legalMovesTo[legalMovesOrder[moveNum]]);
        m_board->makeMove(legalMovesFrom[legalMovesOrder[moveNum]], legalMovesTo[legalMovesOrder[moveNum]], legalMovesFlags[legalMovesOrder[moveNum]]);
        int evaluation;

        //detect 50 move rule
        if (m_board->get50MoveRule() >= 100) {
            evaluation = 0;
        }
        else {
            //detect three-fold repetition
            char numRepetitions = 1;
            for (int i = m_board->getPly() - 2; i >= m_board->getLastTakeOrPawnMove(); i -= 2) {
                numRepetitions += (m_board->getZobristKey(i) == m_board->getZobristKey(m_board->getPly()));
            }
            if (numRepetitions > 2) {
                evaluation = 0;
            }
            else {
                bool thisMoveExtension = (m_board->getPiece(legalMovesTo[legalMovesOrder[moveNum]]) == (PieceType::BlackPawn - m_board->getTurn()) && ((legalMovesTo[legalMovesOrder[moveNum]] >= 48) || (legalMovesTo[legalMovesOrder[moveNum]] <= 15)));

                bool needsFullSearch = true;

                evaluation = -search(cancelSearch, depth - 1 + thisMoveExtension, 1, -beta, -alpha, thisMoveExtension);
            }
        }

        m_board->unMakeMove(legalMovesFrom[legalMovesOrder[moveNum]], legalMovesTo[legalMovesOrder[moveNum]], legalMovesFlags[legalMovesOrder[moveNum]], &prevMoveState);
        if (*cancelSearch) {
            m_transpositionTable.recordHash(m_board->getZobristKey(m_board->getPly()), depth - 1, findMateDist(alpha, 1), HashType::Exact, *bestMoveNum);
            *eval = alpha;
            return;
        }
        //if the evaluation is a new high, set the hash type in the tt to be exact, as the value calculated will be the exact evaluation
        if (evaluation > alpha) {
            *from = legalMovesFrom[legalMovesOrder[moveNum]];
            *to = legalMovesTo[legalMovesOrder[moveNum]];
            *flags = legalMovesFlags[legalMovesOrder[moveNum]];
            *bestMoveNum = legalMovesOrder[moveNum];
            alpha = evaluation;
        }
    }

    m_transpositionTable.recordHash(m_board->getZobristKey(m_board->getPly()), depth, alpha, HashType::Exact, *bestMoveNum);

    *eval = alpha;

    *cancelSearch = true;
}

void Search::orderMoves(unsigned char* from, unsigned char* to, unsigned char* flags, unsigned int* moveScores, unsigned char numMoves, unsigned char ttBestMove) {
    for (unsigned char move = 0; move < numMoves; move++) {
        //initialise the move score to be a large value so that it remains positive
        //it must be positive because I am shifting it left and storing the index of the move using the least significant bits
        moveScores[move] = 16384; //scores less than 16384 mean better move; vice versa

        if (m_board->getPiece(to[move]) < 12) {
            //give value to taking a high value piece with a low value piece
            int captureMaterialDelta = (constants::PIECE_VALUES[m_board->getPiece(to[move])] + constants::PIECE_VALUES[m_board->getPiece(from[move])]) * (m_board->getTurn() * -2 + 1);
            moveScores[move] += captureMaterialDelta;

            bool opponentCanRecapture = (m_board->getAttackingSquares() >> to[move]) & 1ull;
            moveScores[move] += ((captureMaterialDelta > 0) && opponentCanRecapture) * 800 - 400;
        }

        //give value to promotions
        moveScores[move] += (constants::PIECE_VALUES[flags[move]]) * (flags[move] > 0);

        //prioritise the best move from the transposition table
        moveScores[move] *= move != ttBestMove;

        //add the move array index
        moveScores[move] = (moveScores[move] << 16) | move;
    }

    //sort the moves
    std::sort(moveScores, moveScores + numMoves);

    //shift the bits a further 16 places, left and then 16 places right
    //this removes the move score from the integer and leaves just the array index of the move
    for (unsigned char move = 0; move < numMoves; move++) {
        moveScores[move] = moveScores[move] << 16;
        moveScores[move] = moveScores[move] >> 16;
    }
}

bool Search::checkForSingleLegalMove(unsigned char* from, unsigned char* to, unsigned char* flags) {
    unsigned char numLegalMoves;
    unsigned char legalMovesFrom[256];
    unsigned char legalMovesTo[256];
    unsigned char legalMovesFlags[256];

    m_board->getLegalMoves(&numLegalMoves, legalMovesFrom, legalMovesTo, legalMovesFlags);

    *from = legalMovesFrom[0];
    *to = legalMovesTo[0];
    *flags = legalMovesFlags[0];

    return numLegalMoves == 1;
}

inline int Search::findMateDist(int eval, int plyFromRoot) {
    return eval
        + plyFromRoot * (eval >= std::numeric_limits<int>::max() / 2 - constants::MAX_DEPTH - 1)
        - plyFromRoot * (eval <= -std::numeric_limits<int>::max() / 2 + constants::MAX_DEPTH + 1);
}

inline int Search::findMateValue(int eval, int plyFromRoot) {
    return eval
        - (plyFromRoot - 1) * (eval >= std::numeric_limits<int>::max() / 2 - constants::MAX_DEPTH - 1)
        + (plyFromRoot - 1) * (eval <= -std::numeric_limits<int>::max() / 2 + constants::MAX_DEPTH + 1);
}