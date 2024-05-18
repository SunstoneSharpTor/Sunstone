#pragma once

#include <string>
#include <cstdint>
#include <bit>

#include "transpositionTable.h"

using namespace std;

struct unMakeMoveState {
    uint64_t enPassantBitboard;
    char enPassantSquare;
    char takenPieceType;
    unsigned short lastTakeOrPawnMove;
    char last50MoveRule;
    //castling
    bool castleRights[4];
};

class Board {
public:
    enum PieceType {
        WhiteKing = 0, BlackKing, WhiteQueen, BlackQueen, WhiteBishop, BlackBishop, WhiteKnight, BlackKnight, WhiteRook, BlackRook, WhitePawn, BlackPawn,
        All, White, Black
    };
public:
    //board representation
    //pieces
    uint64_t m_pieces[15]; //bitboards
    char m_eightByEight[64]; //array of 64 pieces containing which type of piece is on each square
    //en passant
    uint64_t m_enPassantBitboard; //square that enemy pawn can move to to capture
    char m_enPassantSquare; //position of pawn that has just moved 2 spaces
    //castling
    bool m_castleRights[4];
    //turns
    bool m_turn;
    unsigned short m_ply;
    //last possible repetition
    unsigned short m_lastTakeOrPawnMove;
    //50 move rule
    char m_50MoveRule;

    unsigned char getSquareNum(unsigned char x, unsigned char y);

    //attacking squares
    uint64_t m_attackingSquares;
    uint64_t m_pawnAttackingSquares;

    //bitboard manipulation
    inline unsigned long lsb(uint64_t bitboard) {
        unsigned long LSBbitboard;
        // _BitScanForward64(&LSBbitboard, bitboard);
        // return LSBbitboard;
        return std::__countr_zero(bitboard);
    }

    //remove the least significant bit and return which bit it was (0, 1, 2, 3, ...)
    inline int popLSB(uint64_t* bitboard) {
        const int square = lsb(*bitboard);
        *bitboard &= *bitboard - 1;
        return square;
    }

    //zobrist
    uint64_t m_zobristRandoms[781];
    void initZobristRandoms();
    uint64_t* m_zobristKeys;
    void getZobristKey();

    TranspositionTable* m_transpositionTable;

    //ai
    int evaluate();
    int search(int depth, int plyFromRoot, int alpha, int beta, char numExtensions);
    int quiescenceSearch(int plyFromRoot, int alpha, int beta);
    int findBestMove(bool* cancelSearch, unsigned char* from, unsigned char* to, unsigned char* flags, int depth, int* bestMoveNum);
    void orderMoves(unsigned char* from, unsigned char* to, unsigned char* flags, unsigned int* moveScores, unsigned char numMoves, unsigned char ttBestMove);
    void orderMovesByMoveScores(unsigned char* from, unsigned char* to, unsigned char* flags, unsigned int* moveScores, unsigned char numMoves, int* lastMoveScores);

    //move generation
    uint64_t* m_rookMovesLookup[64];
    uint64_t m_rookMovementMasks[64];
    uint64_t* m_bishopMovesLookup[64];
    uint64_t m_bishopMovementMasks[64];
    uint64_t m_castlingEmptySquareBitboards[4]; //squares that must be empty to be allowed to castle
    uint64_t m_castlingAttackingSquareBitboards[4]; //squares that must not be attacked to be allowed to castle
    uint64_t m_castlingRookToggleBitboards[4]; //squares that the rook moves from and to while castling
    uint64_t m_knightMoves[64];
    uint64_t m_kingMoves[64];
    uint64_t m_whitePawn1ForwardMoves[64];
    uint64_t m_blackPawn1ForwardMoves[64];
    uint64_t m_whitePawn2ForwardMoves[64];
    uint64_t m_blackPawn2ForwardMoves[64];
    uint64_t m_whitePawnTakesMoves[64];
    uint64_t m_blackPawnTakesMoves[64];
    uint64_t m_whitePawnEnPassantMoves[64];
    uint64_t m_blackPawnEnPassantMoves[64];
    uint64_t m_pinnedPieces;
    uint64_t* m_alignMasks[64];
    bool m_check;
    bool m_doubleCheck;
    char m_checkingPiece;
    void initPieceMovementMasksAndTables();
    void initCastlingBitboards();
    void calculatePawnMoves();
    void calculateKnightMoves();
    void calculateKingMoves();
    uint64_t calculateRookMoves(unsigned char rookPosition, uint64_t blockerBitboard);
    uint64_t calculateBishopMoves(unsigned char bishopPosition, uint64_t blockerBitboard);
    void createRookLookupTable();
    void createBishopLookupTable();
    void generateRookMovementMasks(uint64_t* masks);
    void generateBishopMovementMasks(uint64_t* masks);
    void calculateAlignMasks();
    uint64_t* createAllBlockerBitboards(uint64_t movementMask, int* numBlockerBitboards);
    uint64_t getRookLegalMoves(char square);
    uint64_t getBishopLegalMoves(char square);
    uint64_t getQueenLegalMoves(char square);
    uint64_t getWhitePawnLegalMoves(char square, char kingPosition);
    uint64_t getBlackPawnLegalMoves(char square, char kingPosition);
    uint64_t getKnightLegalMoves(char square);
    uint64_t getKingLegalMoves(char square);
    uint64_t getRookLegalMovesCapturesAndChecksOnly(char square);
    uint64_t getBishopLegalMovesCapturesAndChecksOnly(char square);
    uint64_t getQueenLegalMovesCapturesAndChecksOnly(char square);
    uint64_t getWhitePawnLegalMovesCapturesAndChecksOnly(char square, char kingPosition);
    uint64_t getBlackPawnLegalMovesCapturesAndChecksOnly(char square, char kingPosition);
    uint64_t getKnightLegalMovesCapturesAndChecksOnly(char square);
    uint64_t getKingLegalMovesCapturesAndChecksOnly(char square);
    uint64_t getRookAttacks(char square);
    uint64_t getBishopAttacks(char square);
    uint64_t getQueenAttacks(char square);
    uint64_t getWhitePawnAttacks(char square);
    uint64_t getBlackPawnAttacks(char square);
    uint64_t getKnightAttacks(char square);
    uint64_t getKingAttacks(char square);
    bool inCheckAfterEnPassant(char friendlyPawnSquare, char kingPosition);
    void updateAttackingSquares();
    void updatePinnedPieces();
public:
    Board();
    void loadFromFen(string fen);
    inline char getPiece(unsigned char squareIndex) {
        return m_eightByEight[squareIndex];
    }
    void makeMove(unsigned char from, unsigned char to, unsigned char flags);
    void unMakeMove(unsigned char from, unsigned char to, unsigned char flags, unMakeMoveState* prevBoardInfo);
    void getLegalMoves(unsigned char* numLegalMoves, unsigned char* legalMovesFrom, unsigned char* legalMovesTo, unsigned char* legalMovesFlags);
    void getCaptureAndCheckMoves(unsigned char* numLegalMoves, unsigned char* legalMovesFrom, unsigned char* legalMovesTo, unsigned char* legalMovesFlags);
    uint64_t getLegalMovesBitboardForSquare(char square, unsigned char* numLegalMoves, unsigned char* legalMovesFrom, unsigned char* legalMovesTo);
    bool isMovePromotion(unsigned char from, unsigned char to, unsigned char* numLegalMoves, unsigned char* legalMovesFrom, unsigned char* legalMovesTo, unsigned char* legalMovesFlags);

    void calculateMagic(uint64_t* magic, char* shift, uint64_t* blockerBitboards, int numBlockerBitboards, uint64_t* keys, uint64_t* fullPieceMoves);

    void getUnMakeMoveState(unMakeMoveState* prevMoveState, char to);

    unsigned long long perft(int depth);

    unsigned long long numPositions;

    string getSquareName(char squareNum);

    inline char getSquareNumFromString(string squareName) {
        return 56 - (squareName[1] - '1') * 8 + squareName[0] - 'a';
    }

    string getMoveName(char from, char to, char flags);
};