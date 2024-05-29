#pragma once

#include <string>
#include <cstdint>
#include <bit>

#include "transpositionTable.h"

using namespace std;

enum PieceType {
    WhiteKing = 0, BlackKing, WhiteQueen, BlackQueen, WhiteBishop, BlackBishop, WhiteKnight, BlackKnight, WhiteRook, BlackRook, WhitePawn, BlackPawn,
    All, White, Black
};

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
private:
    //board representation
    //pieces
    uint64_t m_pieces[15]; //bitboards
    char m_eightByEight[64]; //array of 64 pieces containing which type of piece is on each square
    //en passant
    uint64_t m_enPassantBitboard; //square that enemy pawn can move to to capture
    char m_enPassantSquare; //position of pawn that has just moved 2 spaces
    //castling
    bool m_castleRights[4];
    bool m_castled[2];
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

    //zobrist
    uint64_t m_zobristRandoms[781];
    void initZobristRandoms();
    uint64_t* m_zobristKeys;
    void setZobristKey();

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
    uint64_t getRookLegalMovesCapturesOnly(char square);
    uint64_t getBishopLegalMovesCapturesOnly(char square);
    uint64_t getQueenLegalMovesCapturesOnly(char square);
    uint64_t getWhitePawnLegalMovesCapturesOnly(char square, char kingPosition);
    uint64_t getBlackPawnLegalMovesCapturesOnly(char square, char kingPosition);
    uint64_t getKnightLegalMovesCapturesOnly(char square);
    uint64_t getKingLegalMovesCapturesOnly(char square);
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
    void makeMove(unsigned char from, unsigned char to, unsigned char flags);
    void getUnMakeMoveState(unMakeMoveState* prevMoveState, char to);
    void unMakeMove(unsigned char from, unsigned char to, unsigned char flags, unMakeMoveState* prevBoardInfo);
    void getLegalMoves(unsigned char* numLegalMoves, unsigned char* legalMovesFrom, unsigned char* legalMovesTo, unsigned char* legalMovesFlags);
    void getCaptureMoves(unsigned char* numLegalMoves, unsigned char* legalMovesFrom, unsigned char* legalMovesTo, unsigned char* legalMovesFlags);
    uint64_t getLegalMovesBitboardForSquare(char square, unsigned char* numLegalMoves, unsigned char* legalMovesFrom, unsigned char* legalMovesTo);
    bool isMovePromotion(unsigned char from, unsigned char to, unsigned char* numLegalMoves, unsigned char* legalMovesFrom, unsigned char* legalMovesTo, unsigned char* legalMovesFlags);

    void calculateMagic(uint64_t* magic, char* shift, uint64_t* blockerBitboards, int numBlockerBitboards, uint64_t* keys, uint64_t* fullPieceMoves);

    unsigned long long perft(int depth);

    string getSquareName(char squareNum);

    inline char getSquareNumFromString(string squareName) {
        return 56 - (squareName[1] - '1') * 8 + squareName[0] - 'a';
    }

    string getMoveName(char from, char to, char flags);

    inline bool getTurn() {
        return m_turn;
    }
    inline short getPly() {
        return m_ply;
    }
    inline uint64_t getPiecesBB(int pieceType) {
        return m_pieces[pieceType];
    }
    inline char getPiece(unsigned char squareIndex) {
        return m_eightByEight[squareIndex];
    }
    inline uint64_t getZobristRandom(short index) {
        return m_zobristRandoms[index];
    }
    inline uint64_t getZobristKey(short ply) {
        return m_zobristKeys[ply];
    }
    inline bool inCheck() {
        return m_check;
    }
    inline char get50MoveRule() {
        return m_50MoveRule;
    }
    inline short getLastTakeOrPawnMove() {
        return m_lastTakeOrPawnMove;
    }
    inline uint64_t getAttackingSquares() {
        return m_attackingSquares;
    }
    inline int getCastleScore() {
        return (m_castleRights[0] || m_castleRights[1] || m_castled[0])
            - (m_castleRights[2] || m_castleRights[3] || m_castled[1]);
    }
};