#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <sstream>
#include <cstdint>

#include "board.h"
#include "bitboard.h"
#include "constants.h"

Board::Board() {
    for (unsigned char i = 0; i < 15; i++) {
        m_pieces[i] = 0ull;
    }
    m_pieces[PieceType::All] = ~m_pieces[PieceType::All];
    for (unsigned char i = 0; i < 64; i++) {
        m_eightByEight[i] = 12;
    }
    m_turn = 0;
    m_ply = 0;
    m_enPassantSquare = 64;
    m_enPassantBitboard = 0ull;
    for (unsigned int i = 0; i < 4; i++) {
        m_castleRights[i] = true;
    }

    m_zobristKeys = new uint64_t[11800];

    initPieceMovementMasksAndTables();
    initZobristRandoms();
}

void Board::loadFromFen(string fen) {
    for (unsigned char i = 0; i < 15; i++) {
        m_pieces[i] = 0ull;
    }
    m_pieces[PieceType::All] = ~m_pieces[PieceType::All];

    stringstream stream(fen);
    string fenBoard;
    stream >> fenBoard;

    uint64_t square = 1;
    unsigned char squareIndex = 0;
    for (int i = 0; i < fenBoard.size(); i++) {
        if (fen[i] != '/') {
            if (isdigit(fen[i])) {
                int blanks = fen[i] - '0';
                while (blanks > 0) {
                    blanks--;
                    square = square << 1;
                    m_eightByEight[squareIndex] = PieceType::All;
                    squareIndex++;
                }
            }
            else {
                unsigned char ii = 0;
                while ((ii < 12) && (fen[i] != constants::PIECE_LETTERS[ii])) {
                    ii++;
                }
                if (ii > 11) {
                    square = square << 1;
                    squareIndex++;
                    continue;
                }
                m_pieces[ii] |= square;
                m_pieces[PieceType::All] ^= square;
                m_eightByEight[squareIndex] = ii;
                square = square << 1;
                squareIndex++;
            }
        }
    }

    //load who's turn it is
    stream >> fenBoard;
    if (fenBoard == "b") {
        m_turn = 1;
    }
    else {
        m_turn = 0;
    }

    //load castling rights
    stream >> fenBoard;
    for (int i = 0; i < 4; i++) {
        m_castleRights[i] = false;
    }
    if (fenBoard.find('Q') != string::npos) {
        m_castleRights[0] = true;
    }
    if (fenBoard.find('K') != string::npos) {
        m_castleRights[1] = true;
    }
    if (fenBoard.find('q') != string::npos) {
        m_castleRights[2] = true;
    }
    if (fenBoard.find('k') != string::npos) {
        m_castleRights[3] = true;
    }

    //load en passant square
    stream >> fenBoard;
    if (fenBoard == "-") {
        m_enPassantSquare = 64;
        m_enPassantBitboard = 0;
    }
    else {
        char enPassantSquare = getSquareNumFromString(fenBoard);
        m_enPassantBitboard = 1ull << enPassantSquare;
        m_enPassantSquare = enPassantSquare + 8 - 16 * m_turn;
    }

    //TODO
    //load the half move clock
    stream >> fenBoard;
    m_50MoveRule = stoi(fenBoard);
    //TODO
    //load the number of moves
    stream >> fenBoard;
    m_ply = 0;
    m_lastTakeOrPawnMove = 0;

    //update bitboards of only white and only black pieces
    for (char side = 0; side < 2; side++) {
        for (char piece = 0; piece < 6; piece++) {
            m_pieces[PieceType::White + side] |= m_pieces[piece * 2 + side];
        }
    }

    //get the zobrist hash
    setZobristKey();
}

void Board::makeMove(unsigned char from, unsigned char to, unsigned char flags) {
    //copy the zobrist key from the last position
    m_zobristKeys[m_ply + 1] = m_zobristKeys[m_ply];

    m_ply++;
    //apply the zobrist number to switch who's turn it is
    m_zobristKeys[m_ply] ^= m_zobristRandoms[768];

    //if the move was a take or a pawn move, update m_lastTakeOrPawnMove
    bool takeOrPawnMove = (m_eightByEight[to] < 12) || (m_eightByEight[from] == PieceType::WhitePawn + m_turn);
    m_lastTakeOrPawnMove = m_lastTakeOrPawnMove * (!takeOrPawnMove) + (m_ply * takeOrPawnMove);
    //update 50 move rule counter
    m_50MoveRule = 0 * takeOrPawnMove + (m_50MoveRule + 1) * (!takeOrPawnMove);

    //detect castling
    bool castle[4];
    castle[0] = m_castleRights[0] && (from == 60) && (to == 58);
    castle[1] = m_castleRights[1] && (from == 60) && (to == 62);
    castle[2] = m_castleRights[2] && (from == 4) && (to == 2);
    castle[3] = m_castleRights[3] && (from == 4) && (to == 6);

    //move rook when castling
    if (castle[0] || castle[1] || castle[2] || castle[3]) {
        uint64_t toggleBitboard = m_castlingRookToggleBitboards[0] * castle[0];
        m_pieces[PieceType::WhiteRook] ^= toggleBitboard;
        m_pieces[PieceType::All] ^= toggleBitboard;
        m_eightByEight[56] = (PieceType::All * castle[0]) + (m_eightByEight[56] * !castle[0]);
        m_eightByEight[59] = (m_eightByEight[59] * !castle[0]) + (PieceType::WhiteRook * castle[0]);
        m_zobristKeys[m_ply] ^= m_zobristRandoms[PieceType::WhiteRook * 64 + 56] * castle[0];
        m_zobristKeys[m_ply] ^= m_zobristRandoms[PieceType::WhiteRook * 64 + 59] * castle[0];
        toggleBitboard = m_castlingRookToggleBitboards[1] * castle[1];
        m_pieces[PieceType::WhiteRook] ^= toggleBitboard;
        m_pieces[PieceType::All] ^= toggleBitboard;
        m_eightByEight[63] = (PieceType::All * castle[1]) + (m_eightByEight[63] * !castle[1]);
        m_eightByEight[61] = (m_eightByEight[61] * !castle[1]) + (PieceType::WhiteRook * castle[1]);
        m_zobristKeys[m_ply] ^= m_zobristRandoms[PieceType::WhiteRook * 64 + 63] * castle[1];
        m_zobristKeys[m_ply] ^= m_zobristRandoms[PieceType::WhiteRook * 64 + 61] * castle[1];
        toggleBitboard = m_castlingRookToggleBitboards[2] * castle[2];
        m_pieces[PieceType::BlackRook] ^= toggleBitboard;
        m_pieces[PieceType::All] ^= toggleBitboard;
        m_eightByEight[0] = (PieceType::All * castle[2]) + (m_eightByEight[0] * !castle[2]);
        m_eightByEight[3] = (m_eightByEight[3] * !castle[2]) + (PieceType::BlackRook * castle[2]);
        m_zobristKeys[m_ply] ^= m_zobristRandoms[PieceType::BlackRook * 64 + 0] * castle[2];
        m_zobristKeys[m_ply] ^= m_zobristRandoms[PieceType::BlackRook * 64 + 3] * castle[2];
        toggleBitboard = m_castlingRookToggleBitboards[3] * castle[3];
        m_pieces[PieceType::BlackRook] ^= toggleBitboard;
        m_pieces[PieceType::All] ^= toggleBitboard;
        m_eightByEight[7] = (PieceType::All * castle[3]) + (m_eightByEight[7] * !castle[3]);
        m_eightByEight[5] = (m_eightByEight[5] * !castle[3]) + (PieceType::BlackRook * castle[3]);
        m_zobristKeys[m_ply] ^= m_zobristRandoms[PieceType::BlackRook * 64 + 7] * castle[3];
        m_zobristKeys[m_ply] ^= m_zobristRandoms[PieceType::BlackRook * 64 + 5] * castle[3];
    }

    //update castling rights
    m_zobristKeys[m_ply] ^= m_zobristRandoms[769] * m_castleRights[0];
    m_zobristKeys[m_ply] ^= m_zobristRandoms[770] * m_castleRights[1];
    m_zobristKeys[m_ply] ^= m_zobristRandoms[771] * m_castleRights[2];
    m_zobristKeys[m_ply] ^= m_zobristRandoms[772] * m_castleRights[3];

    m_castleRights[0] = m_castleRights[0] && !((from == 56) || (to == 56) || (from == 60));
    m_castleRights[1] = m_castleRights[1] && !((from == 63) || (to == 63) || (from == 60));
    m_castleRights[2] = m_castleRights[2] && !((from == 0) || (to == 0) || (from == 4));
    m_castleRights[3] = m_castleRights[3] && !((from == 7) || (to == 7) || (from == 4));

    m_zobristKeys[m_ply] ^= m_zobristRandoms[769] * m_castleRights[0];
    m_zobristKeys[m_ply] ^= m_zobristRandoms[770] * m_castleRights[1];
    m_zobristKeys[m_ply] ^= m_zobristRandoms[771] * m_castleRights[2];
    m_zobristKeys[m_ply] ^= m_zobristRandoms[772] * m_castleRights[3];

    //create mask for the square being moved to
    uint64_t mask = 1ull << to;
    uint64_t invertedMask = ~mask;

    //detect en passant
    bool enPassant = (mask == m_enPassantBitboard) && ((m_eightByEight[from] == PieceType::WhitePawn) || (m_eightByEight[from] == PieceType::BlackPawn));

    //take en passant pawn
    m_pieces[BlackPawn - m_turn] ^= (1ull << m_enPassantSquare) * enPassant;
    m_zobristKeys[m_ply] ^= m_zobristRandoms[64 * (PieceType::BlackPawn - m_turn) + m_enPassantSquare] * enPassant;
    m_pieces[All] ^= (1ull << m_enPassantSquare) * enPassant;
    m_eightByEight[m_enPassantSquare] = (PieceType::All * enPassant) + (m_eightByEight[m_enPassantSquare] * !enPassant);

    //update bitboards for taken piece and piece being moved to square
    m_pieces[m_eightByEight[to]] &= invertedMask;
    m_zobristKeys[m_ply] ^= m_zobristRandoms[64 * m_eightByEight[to] + to] * (m_eightByEight[to] < 12);
    m_pieces[(m_eightByEight[from]) * !(flags) + !!(flags) * (flags + m_turn)] |= mask;
    m_zobristKeys[m_ply] ^= m_zobristRandoms[64 * ((m_eightByEight[from]) * !(flags) + !!(flags) * (flags + m_turn)) + to];

    //create mask for the square being moved from
    mask = 1ull << from;
    invertedMask = ~mask;

    //update bitboards for the piece being moved from the square (and the bitboard of empty squares)
    m_pieces[m_eightByEight[from]] &= invertedMask;
    m_zobristKeys[m_ply] ^= m_zobristRandoms[64 * m_eightByEight[from] + from];
    m_pieces[PieceType::All] |= mask;

    //calculate whether a pawn has just moved 2 spaces forward
    enPassant = ((m_eightByEight[from] == PieceType::WhitePawn) && ((from - to) == 16)) //if the pawn is white and has moved 2 spaces forward
        || ((m_eightByEight[from] == PieceType::BlackPawn) && ((to - from) == 16)); //or the pawn is black and has moved to spaces forward

    //update en passant square and bitboard
    m_zobristKeys[m_ply] ^= m_zobristRandoms[773 + m_enPassantSquare % 8] * (m_enPassantSquare < 64);
    m_enPassantSquare = (to * enPassant) + (64 * !enPassant);
    m_zobristKeys[m_ply] ^= m_zobristRandoms[773 + m_enPassantSquare % 8] * (m_enPassantSquare < 64);
    m_enPassantBitboard = (1ull << (to + 8 - 16 * m_turn)) * enPassant; //location of en passant square (square that enemy pawn can move to to capture)

    //update bitboards of all white and all black pieces
    m_pieces[PieceType::White] = m_pieces[PieceType::WhiteBishop] | m_pieces[PieceType::WhiteKing] | m_pieces[PieceType::WhiteKnight] | m_pieces[PieceType::WhitePawn] | m_pieces[PieceType::WhiteQueen] | m_pieces[PieceType::WhiteRook];
    m_pieces[PieceType::Black] = m_pieces[PieceType::BlackBishop] | m_pieces[PieceType::BlackKing] | m_pieces[PieceType::BlackKnight] | m_pieces[PieceType::BlackPawn] | m_pieces[PieceType::BlackQueen] | m_pieces[PieceType::BlackRook];

    //update eight by eight board representation
    m_eightByEight[to] = (m_eightByEight[from] * !flags) + (!!(flags) * (flags + m_turn));
    m_eightByEight[from] = PieceType::All;

    m_turn = !m_turn;
}

void Board::unMakeMove(unsigned char from, unsigned char to, unsigned char flags, unMakeMoveState* prevBoardInfo) {
    m_zobristKeys[m_ply] ^= m_zobristRandoms[769] * m_castleRights[0];
    m_zobristKeys[m_ply] ^= m_zobristRandoms[770] * m_castleRights[1];
    m_zobristKeys[m_ply] ^= m_zobristRandoms[771] * m_castleRights[2];
    m_zobristKeys[m_ply] ^= m_zobristRandoms[772] * m_castleRights[3];

    m_castleRights[0] = prevBoardInfo->castleRights[0];
    m_castleRights[1] = prevBoardInfo->castleRights[1];
    m_castleRights[2] = prevBoardInfo->castleRights[2];
    m_castleRights[3] = prevBoardInfo->castleRights[3];

    m_zobristKeys[m_ply] ^= m_zobristRandoms[769] * m_castleRights[0];
    m_zobristKeys[m_ply] ^= m_zobristRandoms[770] * m_castleRights[1];
    m_zobristKeys[m_ply] ^= m_zobristRandoms[771] * m_castleRights[2];
    m_zobristKeys[m_ply] ^= m_zobristRandoms[772] * m_castleRights[3];

    m_turn = !m_turn;

    //update eight by eight board representation
    m_eightByEight[from] = (m_eightByEight[to] * !flags) + (!(!flags) * (PieceType::WhitePawn + m_turn));
    m_eightByEight[to] = prevBoardInfo->takenPieceType;

    //update en passant square and bitboard
    m_zobristKeys[m_ply] ^= m_zobristRandoms[773 + m_enPassantSquare % 8] * (m_enPassantSquare < 64);
    m_enPassantSquare = prevBoardInfo->enPassantSquare;
    m_zobristKeys[m_ply] ^= m_zobristRandoms[773 + m_enPassantSquare % 8] * (m_enPassantSquare < 64);
    m_enPassantBitboard = prevBoardInfo->enPassantBitboard; //location of en passant square (square that enemy pawn can move to to capture)

    //create mask for the square being moved from
    uint64_t mask = 1ull << from;
    uint64_t invertedMask = ~mask;

    //update bitboards for the piece being moved from the square (and the bitboard of empty squares)
    m_pieces[m_eightByEight[from]] |= mask;
    m_zobristKeys[m_ply] ^= m_zobristRandoms[64 * m_eightByEight[from] + from];
    m_pieces[PieceType::All] &= invertedMask;

    //create mask for the square being moved to
    mask = 1ull << to;
    invertedMask = ~mask;

    //detect en passant
    bool enPassant = (mask == m_enPassantBitboard) && ((m_eightByEight[from] == PieceType::WhitePawn) || (m_eightByEight[from] == PieceType::BlackPawn));

    //un-take en passant pawn
    m_pieces[BlackPawn - m_turn] ^= (1ull << m_enPassantSquare) * enPassant;
    m_zobristKeys[m_ply] ^= m_zobristRandoms[64 * (PieceType::BlackPawn - m_turn) + m_enPassantSquare] * enPassant;
    m_pieces[All] ^= (1ull << m_enPassantSquare) * enPassant;
    m_eightByEight[m_enPassantSquare] = ((PieceType::BlackPawn - m_turn) * enPassant) + (m_eightByEight[m_enPassantSquare] * !enPassant);

    //update bitboards for taken piece and piece being moved to square
    m_pieces[prevBoardInfo->takenPieceType] |= mask;
    m_zobristKeys[m_ply] ^= m_zobristRandoms[64 * prevBoardInfo->takenPieceType + to] * (m_eightByEight[to] < 12);
    m_pieces[(m_eightByEight[from] * !flags) + (!(!flags) * (flags + m_turn))] &= invertedMask;
    m_zobristKeys[m_ply] ^= m_zobristRandoms[64 * ((m_eightByEight[from]) * !(flags)+!!(flags) * (flags + m_turn)) + to];

    //detect castling
    bool castle[4];
    castle[0] = m_castleRights[0] && (from == 60) && (to == 58);
    castle[1] = m_castleRights[1] && (from == 60) && (to == 62);
    castle[2] = m_castleRights[2] && (from == 4) && (to == 2);
    castle[3] = m_castleRights[3] && (from == 4) && (to == 6);

    //move rook when castling
    if (castle[0] || castle[1] || castle[2] || castle[3]) {
        uint64_t toggleBitboard = m_castlingRookToggleBitboards[0] * castle[0];
        m_pieces[PieceType::WhiteRook] ^= toggleBitboard;
        m_pieces[PieceType::All] ^= toggleBitboard;
        m_eightByEight[56] = (PieceType::WhiteRook * castle[0]) + (m_eightByEight[56] * !castle[0]);
        m_eightByEight[59] = (m_eightByEight[59] * !castle[0]) + (PieceType::All * castle[0]);
        m_zobristKeys[m_ply] ^= m_zobristRandoms[PieceType::WhiteRook * 64 + 56] * castle[0];
        m_zobristKeys[m_ply] ^= m_zobristRandoms[PieceType::WhiteRook * 64 + 59] * castle[0];
        toggleBitboard = m_castlingRookToggleBitboards[1] * castle[1];
        m_pieces[PieceType::WhiteRook] ^= toggleBitboard;
        m_pieces[PieceType::All] ^= toggleBitboard;
        m_eightByEight[63] = (PieceType::WhiteRook * castle[1]) + (m_eightByEight[63] * !castle[1]);
        m_eightByEight[61] = (m_eightByEight[61] * !castle[1]) + (PieceType::All * castle[1]);
        m_zobristKeys[m_ply] ^= m_zobristRandoms[PieceType::WhiteRook * 64 + 63] * castle[1];
        m_zobristKeys[m_ply] ^= m_zobristRandoms[PieceType::WhiteRook * 64 + 61] * castle[1];
        toggleBitboard = m_castlingRookToggleBitboards[2] * castle[2];
        m_pieces[PieceType::BlackRook] ^= toggleBitboard;
        m_pieces[PieceType::All] ^= toggleBitboard;
        m_eightByEight[0] = (PieceType::BlackRook * castle[2]) + (m_eightByEight[0] * !castle[2]);
        m_eightByEight[3] = (m_eightByEight[3] * !castle[2]) + (PieceType::All * castle[2]);
        m_zobristKeys[m_ply] ^= m_zobristRandoms[PieceType::BlackRook * 64 + 0] * castle[2];
        m_zobristKeys[m_ply] ^= m_zobristRandoms[PieceType::BlackRook * 64 + 3] * castle[2];
        toggleBitboard = m_castlingRookToggleBitboards[3] * castle[3];
        m_pieces[PieceType::BlackRook] ^= toggleBitboard;
        m_pieces[PieceType::All] ^= toggleBitboard;
        m_eightByEight[7] = (PieceType::BlackRook * castle[3]) + (m_eightByEight[7] * !castle[3]);
        m_eightByEight[5] = (m_eightByEight[5] * !castle[3]) + (PieceType::All * castle[3]);
        m_zobristKeys[m_ply] ^= m_zobristRandoms[PieceType::BlackRook * 64 + 7] * castle[3];
        m_zobristKeys[m_ply] ^= m_zobristRandoms[PieceType::BlackRook * 64 + 5] * castle[3];
    }

    //update bitboards of all white and all black pieces
    m_pieces[PieceType::White] = m_pieces[PieceType::WhiteBishop] | m_pieces[PieceType::WhiteKing] | m_pieces[PieceType::WhiteKnight] | m_pieces[PieceType::WhitePawn] | m_pieces[PieceType::WhiteQueen] | m_pieces[PieceType::WhiteRook];
    m_pieces[PieceType::Black] = m_pieces[PieceType::BlackBishop] | m_pieces[PieceType::BlackKing] | m_pieces[PieceType::BlackKnight] | m_pieces[PieceType::BlackPawn] | m_pieces[PieceType::BlackQueen] | m_pieces[PieceType::BlackRook];

    m_lastTakeOrPawnMove = prevBoardInfo->lastTakeOrPawnMove;
    m_50MoveRule = prevBoardInfo->last50MoveRule;

    m_ply--;
}

unsigned char Board::getSquareNum(unsigned char x, unsigned char y) {
    return y * 8 + x;
}

void Board::generateRookMovementMasks(uint64_t* masks) {
    char positionXoffsets[4] = { -1, 1, 0, 0 };
    char positionYoffsets[4] = { 0, 0, -1, 1 };
    //the position the rook needs to be in for there to be no more moves when traveling in that direction:
    char xLimits[4] = { 0, 7, 8, 8 };
    char yLimits[4] = { 8, 8, 0, 7 };
    for (char square = 0; square < 64; square++) {
        uint64_t mask = 0;
        int position[2];
        for (unsigned char direction = 0; direction < 4; direction++) {
            position[0] = square % 8;
            position[1] = square / 8;
            if ((xLimits[direction] == position[0]) || (yLimits[direction] == position[1])) {
                position[0] -= positionXoffsets[direction];
                position[1] -= positionYoffsets[direction];
            }
            position[0] += positionXoffsets[direction];
            position[1] += positionYoffsets[direction];
            while (!((xLimits[direction] == position[0]) || (yLimits[direction] == position[1]))) {
                char squareNum = getSquareNum(position[0], position[1]);
                mask |= 1ull << squareNum;

                position[0] += positionXoffsets[direction];
                position[1] += positionYoffsets[direction];
            }
        }

        masks[square] = mask;
    }
}

void Board::generateBishopMovementMasks(uint64_t* masks) {
    char positionXoffsets[4] = { -1, 1, -1, 1 };
    char positionYoffsets[4] = { -1, -1, 1, 1 };
    //the position the bishop needs to be in for there to be no more moves when traveling in that direction:
    char xLimits[4] = { 0, 7, 0, 7 };
    char yLimits[4] = { 0, 0, 7, 7 };
    for (char square = 0; square < 64; square++) {
        uint64_t mask = 0;
        int position[2];
        for (unsigned char direction = 0; direction < 4; direction++) {
            position[0] = square % 8;
            position[1] = square / 8;
            if ((xLimits[direction] == position[0]) || (yLimits[direction] == position[1])) {
                position[0] -= positionXoffsets[direction];
                position[1] -= positionYoffsets[direction];
            }
            position[0] += positionXoffsets[direction];
            position[1] += positionYoffsets[direction];
            while (!((xLimits[direction] == position[0]) || (yLimits[direction] == position[1]))) {
                char squareNum = getSquareNum(position[0], position[1]);
                mask |= 1ull << squareNum;

                position[0] += positionXoffsets[direction];
                position[1] += positionYoffsets[direction];
            }
        }

        masks[square] = mask;
    }
}

uint64_t* Board::createAllBlockerBitboards(uint64_t movementMask, int* numBlockerBitboards) {
    //create a list of all the squares that the piece can move to according to the movement mask
    vector<unsigned char> moveSquareIndices;
    for (unsigned char square = 0; square < 64; square++) {
        if (movementMask & (1ull << square)) {
            moveSquareIndices.push_back(square);
        }
    }

    //calculate the total number of bitboards possible for the number of squares it can move to (2^n)
    *numBlockerBitboards = 1u << moveSquareIndices.size();

    //create all the bitboards
    uint64_t* blockerBitboards = new uint64_t[*numBlockerBitboards];
    for (int bitboardIndex = 0; bitboardIndex < *numBlockerBitboards; bitboardIndex++) {
        blockerBitboards[bitboardIndex] = 0ull;
        for (unsigned char bitIndex = 0; bitIndex < moveSquareIndices.size(); bitIndex++) {
            uint64_t bit = (static_cast<uint64_t>(bitboardIndex) >> bitIndex) & 1ull;
            blockerBitboards[bitboardIndex] |= bit << moveSquareIndices[bitIndex];
        }
    }

    return blockerBitboards;
}

//function to find a magic number for a given set of blocker bitboards and legal moves
void Board::calculateMagic(uint64_t* magic, char* shift, uint64_t* blockerBitboards, int numBlockerBitboards, uint64_t* keys, uint64_t* fullPieceMoves) {
    bool magicFound = false;
    uint64_t testMagic, testMagic2, r30, s30, t4;
    //generate random 64 bit unsigned integer
    r30 = RAND_MAX * rand() + rand();
    s30 = RAND_MAX * rand() + rand();
    t4 = rand() & 0xf;
    testMagic = (s30 << 34) + (r30 << 4) + t4;

    r30 = RAND_MAX * rand() + rand();
    s30 = RAND_MAX * rand() + rand();
    t4 = rand() & 0xf;
    testMagic2 = (s30 << 34) + (r30 << 4) + t4;

    testMagic = testMagic & testMagic2;

    r30 = RAND_MAX * rand() + rand();
    s30 = RAND_MAX * rand() + rand();
    t4 = rand() & 0xf;
    testMagic2 = (s30 << 34) + (r30 << 4) + t4;

    testMagic = testMagic & testMagic2;

    char minTestShift = *shift;
    char testShift = minTestShift;
    magicFound = true;
    while (magicFound) {
        for (int blockerBitboard = 0; blockerBitboard < numBlockerBitboards; blockerBitboard++) {
            //calculate the key that will be used as an index
            keys[blockerBitboard] = (blockerBitboards[blockerBitboard] * testMagic) >> testShift;
            //shift the key left, and use the least significant bits to store the index of the blockerbitboard
            keys[blockerBitboard] = keys[blockerBitboard] << testShift;
            keys[blockerBitboard] |= blockerBitboard;
        }

        //check if the magic number works
        std::sort(keys, keys + numBlockerBitboards);

        uint64_t lastKey = keys[0];
        for (int key = 1; key < numBlockerBitboards; key++) {
            if ((keys[key] >> testShift) == (lastKey >> testShift)) {
                //if they have the same key, check if they also have the same legal moves, if they do, then the magic still works
                if (fullPieceMoves[(keys[key] << 32) >> 32] != fullPieceMoves[(lastKey << 32) >> 32]) {
                    magicFound = false;
                    break;
                }
            }
            lastKey = keys[key];
        }

        testShift++;
    }

    if (testShift - 2 > minTestShift) {
        if (testShift - 2 > *shift) {
            *magic = testMagic;
            *shift = testShift - 2;
        }
        magicFound = true;
    }
}

void Board::createRookLookupTable() {
    int numBlockerBitboards;
    uint64_t* blockerBitboards;

    int numKeys = 0;
    for (int square = 0; square < 64; square++) {
        //generate all possible piece arrangements for the mask
        blockerBitboards = createAllBlockerBitboards(m_rookMovementMasks[square], &numBlockerBitboards);

        //find the largest key generated by the magic number - this will be the size of the table for this square
        uint64_t maxKey = 0ull;
        uint64_t key;
        for (int blockerBitboard = 0; blockerBitboard < numBlockerBitboards; blockerBitboard++) {
            key = (blockerBitboards[blockerBitboard] * constants::ROOK_MAGICS[square]) >> constants::ROOK_SHIFTS[square];
            if (key > maxKey) {
                maxKey = key;
            }
        }
        numKeys += maxKey + 1;

        //set up the array of move bitboards
        m_rookMovesLookup[square] = new uint64_t[maxKey + 1];//1ull << (64 - constants::ROOK_SHIFTS[square])];

        //generate all legal moves for each blockerBitboard
        for (int blockerBitboard = 0; blockerBitboard < numBlockerBitboards; blockerBitboard++) {
            key = (blockerBitboards[blockerBitboard] * constants::ROOK_MAGICS[square]) >> constants::ROOK_SHIFTS[square];
            m_rookMovesLookup[square][key] = calculateRookMoves(square, blockerBitboards[blockerBitboard]);
        }

        delete[] blockerBitboards;
    }
    //cout << numKeys << endl;
}

void Board::createBishopLookupTable() {
    int numBlockerBitboards;
    uint64_t* blockerBitboards;

    int numKeys = 0;
    for (int square = 0; square < 64; square++) {
        //generate all possible piece arrangements for the mask
        blockerBitboards = createAllBlockerBitboards(m_bishopMovementMasks[square], &numBlockerBitboards);

        //find the largest key generated by the magic number - this will be the size of the table for this square
        uint64_t maxKey = 0ull;
        uint64_t key;
        for (int blockerBitboard = 0; blockerBitboard < numBlockerBitboards; blockerBitboard++) {
            key = (blockerBitboards[blockerBitboard] * constants::BISHOP_MAGICS[square]) >> constants::BISHOP_SHIFTS[square];
            if (key > maxKey) {
                maxKey = key;
            }
        }
        numKeys += maxKey + 1;

        //set up the array of move bitboards
        m_bishopMovesLookup[square] = new uint64_t[maxKey + 1];//1ull << (64 - constants::ROOK_SHIFTS[square])];

        //generate all legal moves for each blockerBitboard
        for (int blockerBitboard = 0; blockerBitboard < numBlockerBitboards; blockerBitboard++) {
            key = (blockerBitboards[blockerBitboard] * constants::BISHOP_MAGICS[square]) >> constants::BISHOP_SHIFTS[square];
            m_bishopMovesLookup[square][key] = calculateBishopMoves(square, blockerBitboards[blockerBitboard]);
        }

        delete[] blockerBitboards;
    }
    //cout << numKeys << endl;
}

uint64_t Board::calculateRookMoves(unsigned char rookPosition, uint64_t blockerBitboard) {
    char positionXoffsets[4] = { -1, 1, 0, 0 };
    char positionYoffsets[4] = { 0, 0, -1, 1 };
    //the position the rook needs to be in for there to be no more moves when traveling in that direction:
    char xLimits[4] = { 0, 7, 8, 8 };
    char yLimits[4] = { 8, 8, 0, 7 };
    uint64_t currentSquareBitboard;
    uint64_t mask = 0ull;
    int position[2];
    for (unsigned char direction = 0; direction < 4; direction++) {
        position[0] = rookPosition % 8;
        position[1] = rookPosition / 8;
        while (!((xLimits[direction] == position[0]) || (yLimits[direction] == position[1]))) {
            position[0] += positionXoffsets[direction];
            position[1] += positionYoffsets[direction];

            char squareNum = getSquareNum(position[0], position[1]);
            currentSquareBitboard = 1ull << squareNum;
            mask |= currentSquareBitboard;

            if (currentSquareBitboard & blockerBitboard) {
                break;
            }
        }
    }

    return mask;
}

uint64_t Board::calculateBishopMoves(unsigned char bishopPosition, uint64_t blockerBitboard) {
    char positionXoffsets[4] = { -1, 1, -1, 1 };
    char positionYoffsets[4] = { -1, -1, 1, 1 };
    //the position the rook needs to be in for there to be no more moves when traveling in that direction:
    char xLimits[4] = { 0, 7, 0, 7 };
    char yLimits[4] = { 0, 0, 7, 7 };
    uint64_t currentSquareBitboard;
    uint64_t mask = 0ull;
    int position[2];
    for (unsigned char direction = 0; direction < 4; direction++) {
        position[0] = bishopPosition % 8;
        position[1] = bishopPosition / 8;
        while (!((xLimits[direction] == position[0]) || (yLimits[direction] == position[1]))) {
            position[0] += positionXoffsets[direction];
            position[1] += positionYoffsets[direction];

            char squareNum = getSquareNum(position[0], position[1]);
            currentSquareBitboard = 1ull << squareNum;
            mask |= currentSquareBitboard;

            if (currentSquareBitboard & blockerBitboard) {
                break;
            }
        }
    }

    return mask;
}

void Board::initPieceMovementMasksAndTables() {
    generateRookMovementMasks(m_rookMovementMasks);
    generateBishopMovementMasks(m_bishopMovementMasks);

    createRookLookupTable();
    createBishopLookupTable();

    calculatePawnMoves();
    calculateKnightMoves();
    calculateKingMoves();

    initCastlingBitboards();

    calculateAlignMasks();
}

//super inefficient subroutine but as it only runs once at the start of the program I'm keeping it simple
void Board::calculatePawnMoves() {
    //pawn 1 forward moves
    for (int square = 8; square < 64; square++) {
        m_whitePawn1ForwardMoves[square] = 1ull << (square - 8);
    }
    for (int square = 0; square < 8; square++) {
        m_whitePawn1ForwardMoves[square] = 0ull;
    }
    for (int square = 0; square < 56; square++) {
        m_blackPawn1ForwardMoves[square] = 1ull << (square + 8);
    }
    for (int square = 56; square < 64; square++) {
        m_blackPawn1ForwardMoves[square] = 0ull;
    }
    //pawn 2 forward moves
    for (int square = 48; square < 56; square++) {
        m_whitePawn2ForwardMoves[square] = 1ull << (square - 16);
    }
    for (int square = 56; square < 64; square++) {
        m_whitePawn2ForwardMoves[square] = 0ull;
    }
    for (int square = 0; square < 48; square++) {
        m_whitePawn2ForwardMoves[square] = 0ull;
    }
    for (int square = 8; square < 16; square++) {
        m_blackPawn2ForwardMoves[square] = 1ull << (square + 16);
    }
    for (int square = 0; square < 8; square++) {
        m_blackPawn2ForwardMoves[square] = 0ull;
    }
    for (int square = 16; square < 64; square++) {
        m_blackPawn2ForwardMoves[square] = 0ull;
    }
    //pawn takes
    for (int square = 8; square < 64; square++) {
        m_whitePawnTakesMoves[square] = 0ull;
        if ((square % 8) > 0) {
            m_whitePawnTakesMoves[square] |= 1ull << (square - 9);
        }
        if ((square % 8) < 7) {
            m_whitePawnTakesMoves[square] |= 1ull << (square - 7);
        }
    }
    for (int square = 0; square < 8; square++) {
        m_blackPawnTakesMoves[square] = 0ull;
    }
    for (int square = 8; square < 64; square++) {
        m_blackPawnTakesMoves[square] = 0ull;
        if ((square % 8) > 0) {
            m_blackPawnTakesMoves[square] |= 1ull << (square + 7);
        }
        if ((square % 8) < 7) {
            m_blackPawnTakesMoves[square] |= 1ull << (square + 9);
        }
    }
    for (int square = 0; square < 8; square++) {
        m_blackPawnTakesMoves[square] = 0ull;
    }
    //en passant
    for (int square = 0; square < 64; square++) {
        m_whitePawnEnPassantMoves[square] = 0ull;
    }
    for (int square = 24; square < 31; square++) {
        m_whitePawnEnPassantMoves[square] |= 1ull << (square - 7);
    }
    for (int square = 25; square < 32; square++) {
        m_whitePawnEnPassantMoves[square] |= 1ull << (square - 9);
    }
    for (int square = 0; square < 64; square++) {
        m_blackPawnEnPassantMoves[square] = 0ull;
    }
    for (int square = 32; square < 39; square++) {
        m_blackPawnEnPassantMoves[square] |= 1ull << (square + 9);
    }
    for (int square = 33; square < 40; square++) {
        m_blackPawnEnPassantMoves[square] |= 1ull << (square + 7);
    }
}

void Board::calculateKnightMoves() {
    for (int square = 0; square < 64; square++) {
        m_knightMoves[square] = 0ull;
        int rank = square / 8;
        int file = square % 8;
        if ((rank < 6) && (file < 7)) {
            m_knightMoves[square] |= 1ull << (square + 17);
        }
        if ((rank < 7) && (file < 6)) {
            m_knightMoves[square] |= 1ull << (square + 10);
        }
        if ((rank > 0) && (file < 6)) {
            m_knightMoves[square] |= 1ull << (square - 6);
        }
        if ((rank > 1) && (file < 7)) {
            m_knightMoves[square] |= 1ull << (square - 15);
        }
        if ((rank > 1) && (file > 0)) {
            m_knightMoves[square] |= 1ull << (square - 17);
        }
        if ((rank > 0) && (file > 1)) {
            m_knightMoves[square] |= 1ull << (square - 10);
        }
        if ((rank < 7) && (file > 1)) {
            m_knightMoves[square] |= 1ull << (square + 6);
        }
        if ((rank < 6) && (file > 0)) {
            m_knightMoves[square] |= 1ull << (square + 15);
        }
    }
}

void Board::calculateKingMoves() {
    for (int square = 0; square < 64; square++) {
        m_kingMoves[square] = 0ull;
        int rank = square / 8;
        int file = square % 8;
        if (rank < 7) {
            m_kingMoves[square] |= 1ull << (square + 8);
        }
        if ((rank < 7) && (file < 7)) {
            m_kingMoves[square] |= 1ull << (square + 9);
        }
        if (file < 7) {
            m_kingMoves[square] |= 1ull << (square + 1);
        }
        if ((rank > 0) && (file < 7)) {
            m_kingMoves[square] |= 1ull << (square - 7);
        }
        if (rank > 0) {
            m_kingMoves[square] |= 1ull << (square - 8);
        }
        if ((rank > 0) && (file > 0)) {
            m_kingMoves[square] |= 1ull << (square - 9);
        }
        if ((file > 0)) {
            m_kingMoves[square] |= 1ull << (square - 1);
        }
        if ((file > 0) && (rank < 7)) {
            m_kingMoves[square] |= 1ull << (square + 7);
        }
    }
}

void Board::initCastlingBitboards() {
    m_castlingEmptySquareBitboards[2] = 14ull;
    m_castlingEmptySquareBitboards[3] = 96ull;
    m_castlingEmptySquareBitboards[0] = 14ull << 56;
    m_castlingEmptySquareBitboards[1] = 96ull << 56;

    m_castlingRookToggleBitboards[2] = 9ull;
    m_castlingRookToggleBitboards[3] = 160ull;
    m_castlingRookToggleBitboards[0] = 9ull << 56;
    m_castlingRookToggleBitboards[1] = 160ull << 56;

    m_castlingAttackingSquareBitboards[2] = 28ull;
    m_castlingAttackingSquareBitboards[3] = 112ull;
    m_castlingAttackingSquareBitboards[0] = 28ull << 56;
    m_castlingAttackingSquareBitboards[1] = 112ull << 56;
}

uint64_t Board::getRookLegalMoves(char square) {
    uint64_t blockerBitboard = (~m_pieces[PieceType::All]) & m_rookMovementMasks[square];
    uint64_t key = (blockerBitboard * constants::ROOK_MAGICS[square]) >> constants::ROOK_SHIFTS[square];
    return m_rookMovesLookup[square][key] & (~m_pieces[PieceType::White + m_turn]);
}

uint64_t Board::getRookLegalMovesCapturesOnly(char square) {
    uint64_t blockerBitboard = (~m_pieces[PieceType::All]) & m_rookMovementMasks[square];
    uint64_t key = (blockerBitboard * constants::ROOK_MAGICS[square]) >> constants::ROOK_SHIFTS[square];
    return m_rookMovesLookup[square][key] & (~m_pieces[PieceType::White + m_turn]) & m_pieces[PieceType::Black - m_turn];
}

uint64_t Board::getRookAttacks(char square) {
    uint64_t blockerBitboard = ((~m_pieces[PieceType::All]) ^ m_pieces[PieceType::WhiteKing + m_turn]) & m_rookMovementMasks[square];
    uint64_t key = (blockerBitboard * constants::ROOK_MAGICS[square]) >> constants::ROOK_SHIFTS[square];
    return m_rookMovesLookup[square][key];
}

uint64_t Board::getBishopLegalMoves(char square) {
    uint64_t blockerBitboard = (~m_pieces[PieceType::All]) & m_bishopMovementMasks[square];
    uint64_t key = (blockerBitboard * constants::BISHOP_MAGICS[square]) >> constants::BISHOP_SHIFTS[square];
    return m_bishopMovesLookup[square][key] & (~m_pieces[PieceType::White + m_turn]);
}

uint64_t Board::getBishopLegalMovesCapturesOnly(char square) {
    uint64_t blockerBitboard = (~m_pieces[PieceType::All]) & m_bishopMovementMasks[square];
    uint64_t key = (blockerBitboard * constants::BISHOP_MAGICS[square]) >> constants::BISHOP_SHIFTS[square];
    return m_bishopMovesLookup[square][key] & (~m_pieces[PieceType::White + m_turn]) & m_pieces[PieceType::Black - m_turn];
}

uint64_t Board::getBishopAttacks(char square) {
    uint64_t blockerBitboard = ((~m_pieces[PieceType::All]) ^ m_pieces[PieceType::WhiteKing + m_turn]) & m_bishopMovementMasks[square];
    uint64_t key = (blockerBitboard * constants::BISHOP_MAGICS[square]) >> constants::BISHOP_SHIFTS[square];
    return m_bishopMovesLookup[square][key];
}

uint64_t Board::getQueenLegalMoves(char square) {
    uint64_t allPiecesBitboard = ~m_pieces[PieceType::All];
    uint64_t rookBlockerBitboard = allPiecesBitboard & m_rookMovementMasks[square];
    uint64_t rookKey = (rookBlockerBitboard * constants::ROOK_MAGICS[square]) >> constants::ROOK_SHIFTS[square];
    uint64_t bishopBlockerBitboard = allPiecesBitboard & m_bishopMovementMasks[square];
    uint64_t bishopKey = (bishopBlockerBitboard * constants::BISHOP_MAGICS[square]) >> constants::BISHOP_SHIFTS[square];
    return (m_rookMovesLookup[square][rookKey] | m_bishopMovesLookup[square][bishopKey]) & (~m_pieces[PieceType::White + m_turn]);
}

uint64_t Board::getQueenLegalMovesCapturesOnly(char square) {
    uint64_t allPiecesBitboard = ~m_pieces[PieceType::All];
    uint64_t rookBlockerBitboard = allPiecesBitboard & m_rookMovementMasks[square];
    uint64_t rookKey = (rookBlockerBitboard * constants::ROOK_MAGICS[square]) >> constants::ROOK_SHIFTS[square];
    uint64_t bishopBlockerBitboard = allPiecesBitboard & m_bishopMovementMasks[square];
    uint64_t bishopKey = (bishopBlockerBitboard * constants::BISHOP_MAGICS[square]) >> constants::BISHOP_SHIFTS[square];
    return (m_rookMovesLookup[square][rookKey] | m_bishopMovesLookup[square][bishopKey]) & (~m_pieces[PieceType::White + m_turn]) & m_pieces[PieceType::Black - m_turn];
}

uint64_t Board::getQueenAttacks(char square) {
    uint64_t allPiecesBitboard = (~m_pieces[PieceType::All]) ^ m_pieces[PieceType::WhiteKing + m_turn];
    uint64_t rookBlockerBitboard = allPiecesBitboard & m_rookMovementMasks[square];
    uint64_t rookKey = (rookBlockerBitboard * constants::ROOK_MAGICS[square]) >> constants::ROOK_SHIFTS[square];
    uint64_t bishopBlockerBitboard = allPiecesBitboard & m_bishopMovementMasks[square];
    uint64_t bishopKey = (bishopBlockerBitboard * constants::BISHOP_MAGICS[square]) >> constants::BISHOP_SHIFTS[square];
    return (m_rookMovesLookup[square][rookKey] | m_bishopMovesLookup[square][bishopKey]);
}

uint64_t Board::getWhitePawnLegalMoves(char square, char kingPosition) {
    uint64_t pieceMoves = m_whitePawn1ForwardMoves[square] & m_pieces[PieceType::All];
    pieceMoves |= m_whitePawn2ForwardMoves[square] & m_pieces[PieceType::All] & (pieceMoves >> 8);
    pieceMoves |= m_whitePawnTakesMoves[square] & m_pieces[PieceType::Black];
    pieceMoves |= (m_whitePawnEnPassantMoves[square] & m_enPassantBitboard) * !inCheckAfterEnPassant(square, kingPosition);
    return pieceMoves;
}

uint64_t Board::getWhitePawnLegalMovesCapturesOnly(char square, char kingPosition) {
    uint64_t pieceMoves = m_whitePawn1ForwardMoves[square] & m_pieces[PieceType::All];
    pieceMoves |= m_whitePawn2ForwardMoves[square] & m_pieces[PieceType::All] & (pieceMoves >> 8);
    pieceMoves |= m_whitePawnTakesMoves[square] & m_pieces[PieceType::Black];
    pieceMoves |= (m_whitePawnEnPassantMoves[square] & m_enPassantBitboard) * !inCheckAfterEnPassant(square, kingPosition);
    return pieceMoves & m_pieces[PieceType::Black - m_turn];
}

uint64_t Board::getWhitePawnAttacks(char square) {
    uint64_t pieceMoves = m_whitePawnTakesMoves[square];
    return pieceMoves;
}

uint64_t Board::getBlackPawnLegalMoves(char square, char kingPosition) {
    uint64_t pieceMoves = m_blackPawn1ForwardMoves[square] & m_pieces[PieceType::All];
    pieceMoves |= m_blackPawn2ForwardMoves[square] & m_pieces[PieceType::All] & (pieceMoves << 8);
    pieceMoves |= m_blackPawnTakesMoves[square] & m_pieces[PieceType::White];
    pieceMoves |= (m_blackPawnEnPassantMoves[square] & m_enPassantBitboard) * !inCheckAfterEnPassant(square, kingPosition);
    return pieceMoves;
}

uint64_t Board::getBlackPawnLegalMovesCapturesOnly(char square, char kingPosition) {
    uint64_t pieceMoves = m_blackPawn1ForwardMoves[square] & m_pieces[PieceType::All];
    pieceMoves |= m_blackPawn2ForwardMoves[square] & m_pieces[PieceType::All] & (pieceMoves << 8);
    pieceMoves |= m_blackPawnTakesMoves[square] & m_pieces[PieceType::White];
    pieceMoves |= (m_blackPawnEnPassantMoves[square] & m_enPassantBitboard) * !inCheckAfterEnPassant(square, kingPosition);
    return pieceMoves & m_pieces[PieceType::Black - m_turn];
}

uint64_t Board::getBlackPawnAttacks(char square) {
    uint64_t pieceMoves = m_blackPawnTakesMoves[square];
    return pieceMoves;
}

uint64_t Board::getKnightLegalMoves(char square) {
    return m_knightMoves[square] & (~m_pieces[PieceType::White + m_turn]);
}

uint64_t Board::getKnightLegalMovesCapturesOnly(char square) {
    return m_knightMoves[square] & (~m_pieces[PieceType::White + m_turn]) & m_pieces[PieceType::Black - m_turn];
}

uint64_t Board::getKnightAttacks(char square) {
    return m_knightMoves[square];
}

uint64_t Board::getKingLegalMoves(char square) {
    uint64_t pieceMoves = m_kingMoves[square] & (~m_pieces[PieceType::White + m_turn]);
    bool whitesTurn = !m_turn;
    pieceMoves |= (1ull << 58) * m_castleRights[0] * (((m_castlingEmptySquareBitboards[0] & m_pieces[PieceType::All]) == m_castlingEmptySquareBitboards[0]) && !(m_castlingAttackingSquareBitboards[0] & m_attackingSquares)) * whitesTurn;
    pieceMoves |= (1ull << 62) * m_castleRights[1] * (((m_castlingEmptySquareBitboards[1] & m_pieces[PieceType::All]) == m_castlingEmptySquareBitboards[1]) && !(m_castlingAttackingSquareBitboards[1] & m_attackingSquares)) * whitesTurn;
    pieceMoves |= (1ull << 2) * m_castleRights[2] * (((m_castlingEmptySquareBitboards[2] & m_pieces[PieceType::All]) == m_castlingEmptySquareBitboards[2]) && !(m_castlingAttackingSquareBitboards[2] & m_attackingSquares)) * m_turn;
    pieceMoves |= (1ull << 6) * m_castleRights[3] * (((m_castlingEmptySquareBitboards[3] & m_pieces[PieceType::All]) == m_castlingEmptySquareBitboards[3]) && !(m_castlingAttackingSquareBitboards[3] & m_attackingSquares)) * m_turn;
    //remove moves which put the king into check
    pieceMoves &= ~m_attackingSquares;
    return pieceMoves;
}

uint64_t Board::getKingLegalMovesCapturesOnly(char square) {
    uint64_t pieceMoves = m_kingMoves[square] & (~m_pieces[PieceType::White + m_turn]);
    bool whitesTurn = !m_turn;
    pieceMoves |= (1ull << 58) * m_castleRights[0] * (((m_castlingEmptySquareBitboards[0] & m_pieces[PieceType::All]) == m_castlingEmptySquareBitboards[0]) && !(m_castlingAttackingSquareBitboards[0] & m_attackingSquares)) * whitesTurn;
    pieceMoves |= (1ull << 62) * m_castleRights[1] * (((m_castlingEmptySquareBitboards[1] & m_pieces[PieceType::All]) == m_castlingEmptySquareBitboards[1]) && !(m_castlingAttackingSquareBitboards[1] & m_attackingSquares)) * whitesTurn;
    pieceMoves |= (1ull << 2) * m_castleRights[2] * (((m_castlingEmptySquareBitboards[2] & m_pieces[PieceType::All]) == m_castlingEmptySquareBitboards[2]) && !(m_castlingAttackingSquareBitboards[2] & m_attackingSquares)) * m_turn;
    pieceMoves |= (1ull << 6) * m_castleRights[3] * (((m_castlingEmptySquareBitboards[3] & m_pieces[PieceType::All]) == m_castlingEmptySquareBitboards[3]) && !(m_castlingAttackingSquareBitboards[3] & m_attackingSquares)) * m_turn;
    //remove moves which put the king into check
    pieceMoves &= ~m_attackingSquares;
    return pieceMoves & m_pieces[PieceType::Black - m_turn];
}

uint64_t Board::getKingAttacks(char square) {
    return m_kingMoves[square];
}

void Board::getLegalMoves(unsigned char* numLegalMoves, unsigned char* legalMovesFrom, unsigned char* legalMovesTo, unsigned char* legalMovesFlags) {
    updateAttackingSquares();
    updatePinnedPieces();

    char kingPosition = lsb(m_pieces[PieceType::WhiteKing + m_turn]);

    *numLegalMoves = 0;

    uint64_t pieceMoves;

    if (m_doubleCheck) {
        pieceMoves = getKingLegalMoves(kingPosition);

        while (pieceMoves) {
            legalMovesTo[*numLegalMoves] = popLSB(&pieceMoves);
            legalMovesFrom[*numLegalMoves] = kingPosition;
            legalMovesFlags[*numLegalMoves] = 0;

            (*numLegalMoves)++;
        }

        return;
    }

    char pieces[16];
    char numPieces = 0;
    //unpack the bitboard of the white / black pieces and store them in an array
    uint64_t piecesBitboard = m_pieces[PieceType::White + m_turn];
    while (piecesBitboard) {
        pieces[numPieces] = popLSB(&piecesBitboard);
        numPieces++;
    }

    if (m_check) {
        //calculate the line on which pieces have to block to stop the check
        uint64_t blockOrCaptureCheckMask = m_alignMasks[kingPosition][m_checkingPiece] & m_alignMasks[m_checkingPiece][kingPosition];
        //erase the line if the checking piece is not a sliding piece (check can't be blocked)
        bool checkingPieceIsSliding = ((m_eightByEight[m_checkingPiece] == (PieceType::BlackQueen - m_turn))
            || (m_eightByEight[m_checkingPiece] == (PieceType::BlackRook - m_turn))
            || (m_eightByEight[m_checkingPiece] == (PieceType::BlackBishop - m_turn)));
        blockOrCaptureCheckMask *= checkingPieceIsSliding;
        //add the position of the checking piece, as it can be taken to stop check
        blockOrCaptureCheckMask |= 1ull << m_checkingPiece;

        for (char piece = 0; piece < numPieces; piece++) {
            switch (m_eightByEight[pieces[piece]]) {
            case PieceType::WhiteRook:
                pieceMoves = getRookLegalMoves(pieces[piece]);
                break;
            case PieceType::BlackRook:
                pieceMoves = getRookLegalMoves(pieces[piece]);
                break;
            case PieceType::WhiteBishop:
                pieceMoves = getBishopLegalMoves(pieces[piece]);
                break;
            case PieceType::BlackBishop:
                pieceMoves = getBishopLegalMoves(pieces[piece]);
                break;
            case PieceType::WhiteQueen:
                pieceMoves = getQueenLegalMoves(pieces[piece]);
                break;
            case PieceType::BlackQueen:
                pieceMoves = getQueenLegalMoves(pieces[piece]);
                break;
            case PieceType::WhitePawn:
                pieceMoves = getWhitePawnLegalMoves(pieces[piece], kingPosition);
                break;
            case PieceType::BlackPawn:
                pieceMoves = getBlackPawnLegalMoves(pieces[piece], kingPosition);
                break;
            case PieceType::WhiteKnight:
                pieceMoves = getKnightLegalMoves(pieces[piece]);
                break;
            case PieceType::BlackKnight:
                pieceMoves = getKnightLegalMoves(pieces[piece]);
                break;
            case PieceType::WhiteKing:
                pieceMoves = getKingLegalMoves(pieces[piece]);
                break;
            case PieceType::BlackKing:
                pieceMoves = getKingLegalMoves(pieces[piece]);
                break;
            }

            //restrict movement of pinned pieces
            bool pinned = (1ull << pieces[piece]) & (m_pinnedPieces);
            pieceMoves &= (m_alignMasks[kingPosition][pieces[piece]] * pinned) + (~0ull * !pinned);

            //restrict movement of pieces to enforce stopping the check
            bool isKing = pieces[piece] == kingPosition;
            pieceMoves &= (blockOrCaptureCheckMask * !isKing) + (~0ull * isKing);

            //also restrict movement of the king to stop it moving along the line of check
            //pieceMoves &= (~(m_alignMasks[m_checkingPiece][kingPosition] * isKing * checkingPieceIsSliding)) + (~0ull * !isKing);

            //extract the piece's moves into the move arrays
            while (pieceMoves) {
                legalMovesTo[*numLegalMoves] = popLSB(&pieceMoves);
                legalMovesFrom[*numLegalMoves] = pieces[piece];
                //if promotion, add extra moves for promoting to a queen, rook, knight or bishop
                if (((m_eightByEight[legalMovesFrom[*numLegalMoves]] == PieceType::WhitePawn) && (legalMovesTo[*numLegalMoves] < 8))
                    || ((m_eightByEight[legalMovesFrom[*numLegalMoves]] == PieceType::BlackPawn) && (legalMovesTo[*numLegalMoves] > 55))) {
                    legalMovesFlags[*numLegalMoves] = PieceType::WhiteQueen;
                    legalMovesFrom[*numLegalMoves + 1] = legalMovesFrom[*numLegalMoves];
                    legalMovesTo[*numLegalMoves + 1] = legalMovesTo[*numLegalMoves];
                    legalMovesFlags[*numLegalMoves + 1] = PieceType::WhiteKnight;
                    legalMovesFrom[*numLegalMoves + 2] = legalMovesFrom[*numLegalMoves];
                    legalMovesTo[*numLegalMoves + 2] = legalMovesTo[*numLegalMoves];
                    legalMovesFlags[*numLegalMoves + 2] = PieceType::WhiteRook;
                    legalMovesFrom[*numLegalMoves + 3] = legalMovesFrom[*numLegalMoves];
                    legalMovesTo[*numLegalMoves + 3] = legalMovesTo[*numLegalMoves];
                    legalMovesFlags[*numLegalMoves + 3] = PieceType::WhiteBishop;

                    (*numLegalMoves) += 4;
                }
                else {
                    legalMovesFlags[*numLegalMoves] = 0;
                    (*numLegalMoves)++;
                }
            }
        }

        return;
    }

    for (char piece = 0; piece < numPieces; piece++) {
        switch (m_eightByEight[pieces[piece]]) {
        case PieceType::WhiteRook:
            pieceMoves = getRookLegalMoves(pieces[piece]);
            break;
        case PieceType::BlackRook:
            pieceMoves = getRookLegalMoves(pieces[piece]);
            break;
        case PieceType::WhiteBishop:
            pieceMoves = getBishopLegalMoves(pieces[piece]);
            break;
        case PieceType::BlackBishop:
            pieceMoves = getBishopLegalMoves(pieces[piece]);
            break;
        case PieceType::WhiteQueen:
            pieceMoves = getQueenLegalMoves(pieces[piece]);
            break;
        case PieceType::BlackQueen:
            pieceMoves = getQueenLegalMoves(pieces[piece]);
            break;
        case PieceType::WhitePawn:
            pieceMoves = getWhitePawnLegalMoves(pieces[piece], kingPosition);
            break;
        case PieceType::BlackPawn:
            pieceMoves = getBlackPawnLegalMoves(pieces[piece], kingPosition);
            break;
        case PieceType::WhiteKnight:
            pieceMoves = getKnightLegalMoves(pieces[piece]);
            break;
        case PieceType::BlackKnight:
            pieceMoves = getKnightLegalMoves(pieces[piece]);
            break;
        case PieceType::WhiteKing:
            pieceMoves = getKingLegalMoves(pieces[piece]);
            break;
        case PieceType::BlackKing:
            pieceMoves = getKingLegalMoves(pieces[piece]);
            break;
        }

        //restrict movement of pinned pieces
        bool pinned = (1ull << pieces[piece]) & (m_pinnedPieces);
        pieceMoves &= (m_alignMasks[kingPosition][pieces[piece]] * pinned) + (~0ull * !pinned);

        //extract the piece's moves into the move arrays
        while (pieceMoves) {
            legalMovesTo[*numLegalMoves] = popLSB(&pieceMoves);
            legalMovesFrom[*numLegalMoves] = pieces[piece];
            //if promotion, add extra moves for promoting to a queen, rook, knight or bishop
            if (((m_eightByEight[legalMovesFrom[*numLegalMoves]] == PieceType::WhitePawn) && (legalMovesTo[*numLegalMoves] < 8))
                || ((m_eightByEight[legalMovesFrom[*numLegalMoves]] == PieceType::BlackPawn) && (legalMovesTo[*numLegalMoves] > 55))) {
                legalMovesFlags[*numLegalMoves] = PieceType::WhiteQueen;
                legalMovesFrom[*numLegalMoves + 1] = legalMovesFrom[*numLegalMoves];
                legalMovesTo[*numLegalMoves + 1] = legalMovesTo[*numLegalMoves];
                legalMovesFlags[*numLegalMoves + 1] = PieceType::WhiteKnight;
                legalMovesFrom[*numLegalMoves + 2] = legalMovesFrom[*numLegalMoves];
                legalMovesTo[*numLegalMoves + 2] = legalMovesTo[*numLegalMoves];
                legalMovesFlags[*numLegalMoves + 2] = PieceType::WhiteRook;
                legalMovesFrom[*numLegalMoves + 3] = legalMovesFrom[*numLegalMoves];
                legalMovesTo[*numLegalMoves + 3] = legalMovesTo[*numLegalMoves];
                legalMovesFlags[*numLegalMoves + 3] = PieceType::WhiteBishop;

                (*numLegalMoves) += 4;
            }
            else {
                legalMovesFlags[*numLegalMoves] = 0;
                (*numLegalMoves)++;
            }
        }
    }
}

void Board::getCaptureMoves(unsigned char* numLegalMoves, unsigned char* legalMovesFrom, unsigned char* legalMovesTo, unsigned char* legalMovesFlags) {
    updateAttackingSquares();
    updatePinnedPieces();

    char kingPosition = lsb(m_pieces[PieceType::WhiteKing + m_turn]);

    *numLegalMoves = 0;

    uint64_t pieceMoves;

    if (m_doubleCheck) {
        pieceMoves = getKingLegalMovesCapturesOnly(kingPosition);

        while (pieceMoves) {
            legalMovesTo[*numLegalMoves] = popLSB(&pieceMoves);
            legalMovesFrom[*numLegalMoves] = kingPosition;
            legalMovesFlags[*numLegalMoves] = 0;

            (*numLegalMoves)++;
        }

        return;
    }

    char pieces[16];
    char numPieces = 0;
    //unpack the bitboard of the white / black pieces and store them in an array
    uint64_t piecesBitboard = m_pieces[PieceType::White + m_turn];
    while (piecesBitboard) {
        pieces[numPieces] = popLSB(&piecesBitboard);
        numPieces++;
    }

    if (m_check) {
        //calculate the line on which pieces have to block to stop the check
        uint64_t blockOrCaptureCheckMask = m_alignMasks[kingPosition][m_checkingPiece] & m_alignMasks[m_checkingPiece][kingPosition];
        //erase the line if the checking piece is not a sliding piece (check can't be blocked)
        bool checkingPieceIsSliding = ((m_eightByEight[m_checkingPiece] == (PieceType::BlackQueen - m_turn))
            || (m_eightByEight[m_checkingPiece] == (PieceType::BlackRook - m_turn))
            || (m_eightByEight[m_checkingPiece] == (PieceType::BlackBishop - m_turn)));
        blockOrCaptureCheckMask *= checkingPieceIsSliding;
        //add the position of the checking piece, as it can be taken to stop check
        blockOrCaptureCheckMask |= 1ull << m_checkingPiece;

        for (char piece = 0; piece < numPieces; piece++) {
            switch (m_eightByEight[pieces[piece]]) {
            case PieceType::WhiteRook:
                pieceMoves = getRookLegalMovesCapturesOnly(pieces[piece]);
                break;
            case PieceType::BlackRook:
                pieceMoves = getRookLegalMovesCapturesOnly(pieces[piece]);
                break;
            case PieceType::WhiteBishop:
                pieceMoves = getBishopLegalMovesCapturesOnly(pieces[piece]);
                break;
            case PieceType::BlackBishop:
                pieceMoves = getBishopLegalMovesCapturesOnly(pieces[piece]);
                break;
            case PieceType::WhiteQueen:
                pieceMoves = getQueenLegalMovesCapturesOnly(pieces[piece]);
                break;
            case PieceType::BlackQueen:
                pieceMoves = getQueenLegalMovesCapturesOnly(pieces[piece]);
                break;
            case PieceType::WhitePawn:
                pieceMoves = getWhitePawnLegalMovesCapturesOnly(pieces[piece], kingPosition);
                break;
            case PieceType::BlackPawn:
                pieceMoves = getBlackPawnLegalMovesCapturesOnly(pieces[piece], kingPosition);
                break;
            case PieceType::WhiteKnight:
                pieceMoves = getKnightLegalMovesCapturesOnly(pieces[piece]);
                break;
            case PieceType::BlackKnight:
                pieceMoves = getKnightLegalMovesCapturesOnly(pieces[piece]);
                break;
            case PieceType::WhiteKing:
                pieceMoves = getKingLegalMovesCapturesOnly(pieces[piece]);
                break;
            case PieceType::BlackKing:
                pieceMoves = getKingLegalMovesCapturesOnly(pieces[piece]);
                break;
            }

            //restrict movement of pinned pieces
            bool pinned = (1ull << pieces[piece]) & (m_pinnedPieces);
            pieceMoves &= (m_alignMasks[kingPosition][pieces[piece]] * pinned) + (~0ull * !pinned);

            //restrict movement of pieces to enforce stopping the check
            bool isKing = pieces[piece] == kingPosition;
            pieceMoves &= (blockOrCaptureCheckMask * !isKing) + (~0ull * isKing);

            //also restrict movement of the king to stop it moving along the line of check
            //pieceMoves &= (~(m_alignMasks[m_checkingPiece][kingPosition] * isKing * checkingPieceIsSliding)) + (~0ull * !isKing);

            //extract the piece's moves into the move arrays
            while (pieceMoves) {
                legalMovesTo[*numLegalMoves] = popLSB(&pieceMoves);
                legalMovesFrom[*numLegalMoves] = pieces[piece];
                //if promotion, add extra moves for promoting to a queen, rook, knight or bishop
                if (((m_eightByEight[legalMovesFrom[*numLegalMoves]] == PieceType::WhitePawn) && (legalMovesTo[*numLegalMoves] < 8))
                    || ((m_eightByEight[legalMovesFrom[*numLegalMoves]] == PieceType::BlackPawn) && (legalMovesTo[*numLegalMoves] > 55))) {
                    legalMovesFlags[*numLegalMoves] = PieceType::WhiteQueen;
                    legalMovesFrom[*numLegalMoves + 1] = legalMovesFrom[*numLegalMoves];
                    legalMovesTo[*numLegalMoves + 1] = legalMovesTo[*numLegalMoves];
                    legalMovesFlags[*numLegalMoves + 1] = PieceType::WhiteKnight;
                    legalMovesFrom[*numLegalMoves + 2] = legalMovesFrom[*numLegalMoves];
                    legalMovesTo[*numLegalMoves + 2] = legalMovesTo[*numLegalMoves];
                    legalMovesFlags[*numLegalMoves + 2] = PieceType::WhiteRook;
                    legalMovesFrom[*numLegalMoves + 3] = legalMovesFrom[*numLegalMoves];
                    legalMovesTo[*numLegalMoves + 3] = legalMovesTo[*numLegalMoves];
                    legalMovesFlags[*numLegalMoves + 3] = PieceType::WhiteBishop;

                    (*numLegalMoves) += 4;
                }
                else {
                    legalMovesFlags[*numLegalMoves] = 0;
                    (*numLegalMoves)++;
                }
            }
        }

        return;
    }

    for (char piece = 0; piece < numPieces; piece++) {
        switch (m_eightByEight[pieces[piece]]) {
        case PieceType::WhiteRook:
            pieceMoves = getRookLegalMovesCapturesOnly(pieces[piece]);
            break;
        case PieceType::BlackRook:
            pieceMoves = getRookLegalMovesCapturesOnly(pieces[piece]);
            break;
        case PieceType::WhiteBishop:
            pieceMoves = getBishopLegalMovesCapturesOnly(pieces[piece]);
            break;
        case PieceType::BlackBishop:
            pieceMoves = getBishopLegalMovesCapturesOnly(pieces[piece]);
            break;
        case PieceType::WhiteQueen:
            pieceMoves = getQueenLegalMovesCapturesOnly(pieces[piece]);
            break;
        case PieceType::BlackQueen:
            pieceMoves = getQueenLegalMovesCapturesOnly(pieces[piece]);
            break;
        case PieceType::WhitePawn:
            pieceMoves = getWhitePawnLegalMovesCapturesOnly(pieces[piece], kingPosition);
            break;
        case PieceType::BlackPawn:
            pieceMoves = getBlackPawnLegalMovesCapturesOnly(pieces[piece], kingPosition);
            break;
        case PieceType::WhiteKnight:
            pieceMoves = getKnightLegalMovesCapturesOnly(pieces[piece]);
            break;
        case PieceType::BlackKnight:
            pieceMoves = getKnightLegalMovesCapturesOnly(pieces[piece]);
            break;
        case PieceType::WhiteKing:
            pieceMoves = getKingLegalMovesCapturesOnly(pieces[piece]);
            break;
        case PieceType::BlackKing:
            pieceMoves = getKingLegalMovesCapturesOnly(pieces[piece]);
            break;
        }

        //restrict movement of pinned pieces
        bool pinned = (1ull << pieces[piece]) & (m_pinnedPieces);
        pieceMoves &= (m_alignMasks[kingPosition][pieces[piece]] * pinned) + (~0ull * !pinned);

        //extract the piece's moves into the move arrays
        while (pieceMoves) {
            legalMovesTo[*numLegalMoves] = popLSB(&pieceMoves);
            legalMovesFrom[*numLegalMoves] = pieces[piece];
            //if promotion, add extra moves for promoting to a queen, rook, knight or bishop
            if (((m_eightByEight[legalMovesFrom[*numLegalMoves]] == PieceType::WhitePawn) && (legalMovesTo[*numLegalMoves] < 8))
                || ((m_eightByEight[legalMovesFrom[*numLegalMoves]] == PieceType::BlackPawn) && (legalMovesTo[*numLegalMoves] > 55))) {
                legalMovesFlags[*numLegalMoves] = PieceType::WhiteQueen;
                legalMovesFrom[*numLegalMoves + 1] = legalMovesFrom[*numLegalMoves];
                legalMovesTo[*numLegalMoves + 1] = legalMovesTo[*numLegalMoves];
                legalMovesFlags[*numLegalMoves + 1] = PieceType::WhiteKnight;
                legalMovesFrom[*numLegalMoves + 2] = legalMovesFrom[*numLegalMoves];
                legalMovesTo[*numLegalMoves + 2] = legalMovesTo[*numLegalMoves];
                legalMovesFlags[*numLegalMoves + 2] = PieceType::WhiteRook;
                legalMovesFrom[*numLegalMoves + 3] = legalMovesFrom[*numLegalMoves];
                legalMovesTo[*numLegalMoves + 3] = legalMovesTo[*numLegalMoves];
                legalMovesFlags[*numLegalMoves + 3] = PieceType::WhiteBishop;

                (*numLegalMoves) += 4;
            }
            else {
                legalMovesFlags[*numLegalMoves] = 0;
                (*numLegalMoves)++;
            }
        }
    }
}

uint64_t Board::getLegalMovesBitboardForSquare(char square, unsigned char* numLegalMoves, unsigned char* legalMovesFrom, unsigned char* legalMovesTo) {
    uint64_t movesBitboard = 0ull;
    for (unsigned char move = 0; move < *numLegalMoves; move++) {
        if (legalMovesFrom[move] == square) {
            movesBitboard |= 1ull << legalMovesTo[move];
        }
    }

    return movesBitboard;
}

void Board::updateAttackingSquares() {
    m_check = m_doubleCheck = false;

    char pieces[16];
    char numPieces = 0;
    //unpack the bitboard of the white / black pieces and store them in an array
    uint64_t piecesBitboard = m_pieces[PieceType::Black - m_turn];
    while (piecesBitboard) {
        pieces[numPieces] = popLSB(&piecesBitboard);
        numPieces++;
    }

    m_attackingSquares = 0ull;

    uint64_t pieceAttacks;

    bool isPieceCheckingKing;
    m_checkingPiece = 0;

    for (char piece = 0; piece < numPieces; piece++) {
        switch (m_eightByEight[pieces[piece]]) {
        case PieceType::WhiteRook:
            pieceAttacks = getRookAttacks(pieces[piece]);
            m_attackingSquares |= pieceAttacks;
            break;
        case PieceType::BlackRook:
            pieceAttacks = getRookAttacks(pieces[piece]);
            m_attackingSquares |= pieceAttacks;
            break;
        case PieceType::WhiteBishop:
            pieceAttacks = getBishopAttacks(pieces[piece]);
            m_attackingSquares |= pieceAttacks;
            break;
        case PieceType::BlackBishop:
            pieceAttacks = getBishopAttacks(pieces[piece]);
            m_attackingSquares |= pieceAttacks;
            break;
        case PieceType::WhiteQueen:
            pieceAttacks = getQueenAttacks(pieces[piece]);
            m_attackingSquares |= pieceAttacks;
            break;
        case PieceType::BlackQueen:
            pieceAttacks = getQueenAttacks(pieces[piece]);
            m_attackingSquares |= pieceAttacks;
            break;
        case PieceType::WhitePawn:
            pieceAttacks = getWhitePawnAttacks(pieces[piece]);
            m_attackingSquares |= pieceAttacks;
            m_pawnAttackingSquares = pieceAttacks;
            break;
        case PieceType::BlackPawn:
            pieceAttacks = getBlackPawnAttacks(pieces[piece]);
            m_attackingSquares |= pieceAttacks;
            m_pawnAttackingSquares = pieceAttacks;
            break;
        case PieceType::WhiteKnight:
            pieceAttacks = getKnightAttacks(pieces[piece]);
            m_attackingSquares |= pieceAttacks;
            break;
        case PieceType::BlackKnight:
            pieceAttacks = getKnightAttacks(pieces[piece]);
            m_attackingSquares |= pieceAttacks;
            break;
        case PieceType::WhiteKing:
            pieceAttacks = getKingAttacks(pieces[piece]);
            m_attackingSquares |= pieceAttacks;
            break;
        case PieceType::BlackKing:
            pieceAttacks = getKingAttacks(pieces[piece]);
            m_attackingSquares |= pieceAttacks;
            break;
        }

        isPieceCheckingKing = pieceAttacks & m_pieces[PieceType::WhiteKing + m_turn];
        m_doubleCheck = m_doubleCheck || (m_check && isPieceCheckingKing);
        m_check = m_check || isPieceCheckingKing;
        m_checkingPiece += pieces[piece] * isPieceCheckingKing;
    }
}

bool Board::isMovePromotion(unsigned char from, unsigned char to, unsigned char* numLegalMoves, unsigned char* legalMovesFrom, unsigned char* legalMovesTo, unsigned char* legalMovesFlags) {
    for (unsigned char move = 0; move < *numLegalMoves; move++) {
        if ((legalMovesFrom[move] == from) && (legalMovesTo[move] == to) && legalMovesFlags[move]) {
            return true;
        }
    }
    return false;
}

void Board::updatePinnedPieces() {
    char kingPos = lsb(m_pieces[PieceType::WhiteKing + m_turn]);
    char distanceFromTop = kingPos / 8;
    char distanceFromLeft = kingPos % 8;
    char distancesFromEdge[8];
    distancesFromEdge[0] = 7 - distanceFromLeft;
    distancesFromEdge[1] = 7 - distanceFromTop;
    distancesFromEdge[2] = distanceFromLeft;
    distancesFromEdge[3] = distanceFromTop;
    distancesFromEdge[4] = min(distancesFromEdge[0], distancesFromEdge[1]);
    distancesFromEdge[5] = min(distancesFromEdge[1], distancesFromEdge[2]);
    distancesFromEdge[6] = min(distancesFromEdge[2], distancesFromEdge[3]);
    distancesFromEdge[7] = min(distancesFromEdge[3], distancesFromEdge[0]);

    m_pinnedPieces = 0ull;
    for (char direction = 0; direction < 8; direction++) {
        uint64_t friendlyPieceAlongRay = 0ull;
        char position = kingPos;
        for (int i = 0; i < distancesFromEdge[direction]; i++) {
            position += constants::DIRECTION_OFFSETS[direction];
            if (m_eightByEight[position] < 12) {
                if (constants::PIECE_SIDES[m_eightByEight[position]] == m_turn) {
                    if (friendlyPieceAlongRay) {
                        //this is the second friendly piece in a row along the ray, so there's no pin
                        break;
                    }
                    else {
                        friendlyPieceAlongRay = 1ull << position;
                    }
                }
                else {
                    //calculate if the piece is able to attack in the direction of the king
                    bool xRayingKing = (m_eightByEight[position] == (PieceType::BlackQueen - m_turn))
                        || ((direction < 4) && (m_eightByEight[position] == (PieceType::BlackRook - m_turn)))
                        || ((direction > 3) && (m_eightByEight[position] == (PieceType::BlackBishop - m_turn)));
                    m_pinnedPieces |= friendlyPieceAlongRay * xRayingKing;
                    break;
                }
            }
        }
    }
}

//calculate the bitboards on whitch pinned pieces can move along
void Board::calculateAlignMasks() {
    for (char kingSquare = 0; kingSquare < 64; kingSquare++) {
        m_alignMasks[kingSquare] = new uint64_t[64];

        char distanceFromTop = kingSquare / 8;
        char distanceFromLeft = kingSquare % 8;
        char distancesFromEdge[8];
        distancesFromEdge[0] = 7 - distanceFromLeft;
        distancesFromEdge[1] = 7 - distanceFromTop;
        distancesFromEdge[2] = distanceFromLeft;
        distancesFromEdge[3] = distanceFromTop;
        distancesFromEdge[4] = min(distancesFromEdge[0], distancesFromEdge[1]);
        distancesFromEdge[5] = min(distancesFromEdge[1], distancesFromEdge[2]);
        distancesFromEdge[6] = min(distancesFromEdge[2], distancesFromEdge[3]);
        distancesFromEdge[7] = min(distancesFromEdge[3], distancesFromEdge[0]);

        for (char direction = 0; direction < 8; direction++) {
            uint64_t alignmentMask = 0ull;

            char position = kingSquare;
            for (int i = 0; i < distancesFromEdge[direction]; i++) {
                position += constants::DIRECTION_OFFSETS[direction];
                alignmentMask |= 1ull << position;
            }

            uint64_t piecesWithMask = alignmentMask;
            while (piecesWithMask) {
                m_alignMasks[kingSquare][popLSB(&piecesWithMask)] = alignmentMask;
            }
        }
    }
}

void Board::getUnMakeMoveState(unMakeMoveState* prevMoveState, char to) {
    prevMoveState->takenPieceType = m_eightByEight[to];
    prevMoveState->enPassantSquare = m_enPassantSquare;
    prevMoveState->enPassantBitboard = m_enPassantBitboard;
    prevMoveState->lastTakeOrPawnMove = m_lastTakeOrPawnMove;
    prevMoveState->last50MoveRule = m_50MoveRule;
    prevMoveState->castleRights[0] = m_castleRights[0];
    prevMoveState->castleRights[1] = m_castleRights[1];
    prevMoveState->castleRights[2] = m_castleRights[2];
    prevMoveState->castleRights[3] = m_castleRights[3];
}

unsigned long long Board::perft(int depth) {
    if (depth == 1) {
        unsigned char numLegalMoves;
        unsigned char legalMovesFrom[256];
        unsigned char legalMovesTo[256];
        unsigned char legalMovesFlags[256];
        getLegalMoves(&numLegalMoves, legalMovesFrom, legalMovesTo, legalMovesFlags);
        return numLegalMoves;
    }
    else {
        unsigned long long numPositions = 0;

        unsigned char numLegalMoves;
        unsigned char legalMovesFrom[256];
        unsigned char legalMovesTo[256];
        unsigned char legalMovesFlags[256];

        getLegalMoves(&numLegalMoves, legalMovesFrom, legalMovesTo, legalMovesFlags);

        for (int moveNum = 0; moveNum < numLegalMoves; moveNum++) {
            unMakeMoveState prevMoveState;
            getUnMakeMoveState(&prevMoveState, legalMovesTo[moveNum]);
            makeMove(legalMovesFrom[moveNum], legalMovesTo[moveNum], legalMovesFlags[moveNum]);
            numPositions += perft(depth - 1);
            unMakeMove(legalMovesFrom[moveNum], legalMovesTo[moveNum], legalMovesFlags[moveNum], &prevMoveState);
        }

        return numPositions;
    }
}

bool Board::inCheckAfterEnPassant(char friendlyPawnSquare, char kingPosition) {
    if (m_enPassantSquare < 64) {
        if ((kingPosition / 8) != (m_enPassantSquare / 8)) {
            return false;
        }
        uint64_t blockerBitboard = (~m_pieces[PieceType::All]);
        blockerBitboard ^= (1ull << m_enPassantSquare) | (1ull << friendlyPawnSquare);
        blockerBitboard &= m_rookMovementMasks[kingPosition];
        uint64_t key = (blockerBitboard * constants::ROOK_MAGICS[kingPosition]) >> constants::ROOK_SHIFTS[kingPosition];
        uint64_t kingRays = m_rookMovesLookup[kingPosition][key] & (~m_pieces[PieceType::White + m_turn]);
        return kingRays & (m_pieces[PieceType::BlackRook - m_turn] | m_pieces[PieceType::BlackQueen - m_turn]);
    }

    return false;
}

string Board::getSquareName(char squareNum) {
    char file = squareNum % 8;
    char rank = 7 - squareNum / 8;
    string value = "a1";
    value[0] = 'a' + file;
    value[1] = '1' + rank;
    return value;
}

string Board::getMoveName(char from, char to, char flags) {
    string result = getSquareName(from);
    result.append(getSquareName(to));

    if (flags) {
        switch (flags) {
        case 2:
            result.append("q");
            break;
        case 8:
            result.append("r");
            break;
        case 4:
            result.append("b");
            break;
        case 6:
            result.append("n");
        }
    }

    return result;
}

void Board::initZobristRandoms() {
    srand(353);

    for (int i = 0; i < 781; i++) {
        //generate random 64 bit unsigned integer
        uint64_t r30, s30, t4;
        r30 = RAND_MAX * rand() + rand();
        s30 = RAND_MAX * rand() + rand();
        t4 = rand() & 0xf;
        m_zobristRandoms[i] = (s30 << 34) + (r30 << 4) + t4;
    }
}

void Board::setZobristKey() {
    m_zobristKeys[m_ply] = 0ull;

    //include the positions for all the pieces
    uint64_t pieceBitboard;
    for (int typeOfPiece = 0; typeOfPiece < 12; typeOfPiece++) {
        pieceBitboard = m_pieces[typeOfPiece];
        while (pieceBitboard) {
            m_zobristKeys[m_ply] ^= m_zobristRandoms[typeOfPiece * 64 + popLSB(&pieceBitboard)];
        }
    }

    //include the side to move
    m_zobristKeys[m_ply] ^= m_zobristRandoms[768] * m_turn;

    //include castling rights
    m_zobristKeys[m_ply] ^= m_zobristRandoms[769] * m_castleRights[0];
    m_zobristKeys[m_ply] ^= m_zobristRandoms[770] * m_castleRights[1];
    m_zobristKeys[m_ply] ^= m_zobristRandoms[771] * m_castleRights[2];
    m_zobristKeys[m_ply] ^= m_zobristRandoms[772] * m_castleRights[3];

    //include en passant square
    m_zobristKeys[m_ply] ^= m_zobristRandoms[773 + m_enPassantSquare % 8] * (m_enPassantSquare < 64);
}