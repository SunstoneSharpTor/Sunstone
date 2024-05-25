#pragma once

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