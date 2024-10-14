#pragma once

#include <cstdint>
#include <string>

#ifndef CONSTANTS_H
#define CONSTANTS_H

namespace constants {
    constexpr char PIECE_LETTERS[12] = { 'K', 'k', 'Q', 'q', 'B', 'b', 'N', 'n', 'R', 'r', 'P', 'p' };
    constexpr bool PIECE_SIDES[13] = { 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0 };

    constexpr char DIRECTION_OFFSETS[8] = { 1, 8, -1, -8, 9, 7, -9, -7 };

    constexpr int PIECE_VALUES[13] = { 0, 0, 1220, -1220, 397, -397, 375, -375, 613, -613, 100, -100, 0 };

    constexpr int MAX_DEPTH = { 1000 };

    constexpr int PIECE_SQUARE_TABLES_EARLY_GAME[64 * 12] = {  // White King
		                                                          -30,-40,-40,-50,-50,-40,-40,-30,
                                                              -30,-40,-40,-50,-50,-40,-40,-30,
                                                              -30,-40,-40,-50,-50,-40,-40,-30,
                                                              -30,-40,-40,-50,-50,-40,-40,-30,
                                                              -20,-30,-30,-40,-40,-30,-30,-20,
                                                              -10,-20,-20,-20,-20,-20,-20,-10,
                                                               20, 20,  0,  0,  0,  0, 20, 20,
                                                               20, 30, 10,  0,  0, 10, 30, 20,
                                                               // Black King
                                                              -20,-30,-10,  0,  0,-10,-30,-20,
                                                              -20,-20,  0,  0,  0,  0,-20,-20,
                                                               10, 20, 20, 20, 20, 20, 20, 10,
                                                               20, 30, 30, 40, 40, 30, 30, 20,
                                                               30, 40, 40, 50, 50, 40, 40, 30,
                                                               30, 40, 40, 50, 50, 40, 40, 30,
                                                               30, 40, 40, 50, 50, 40, 40, 30,
                                                               30, 40, 40, 50, 50, 40, 40, 30,
                                                               // White Queen
                                                              -20,-10,-10, -5, -5,-10,-10,-20,
                                                              -10,  0,  0,  0,  0,  0,  0,-10,
                                                              -10,  0,  5,  5,  5,  5,  0,-10,
                                                               -5,  0,  5,  5,  5,  5,  0, -5,
                                                                0,  0,  5,  5,  5,  5,  0, -5,
                                                              -10,  5,  5,  5,  5,  5,  0,-10,
                                                              -10,  0,  5,  0,  0,  0,  0,-10,
                                                              -20,-10,-10, -5, -5,-10,-10,-20,
                                                               // Black Queen
                                                               20, 10, 10,  5,  5, 10, 10, 20,
                                                               10,  0, -5,  0,  0,  0,  0, 10,
                                                               10, -5, -5, -5, -5, -5,  0, 10,
                                                                0,  0, -5, -5, -5, -5,  0,  5,
                                                                5,  0, -5, -5, -5, -5, -0,  5,
                                                               10, -0, -5, -5, -5, -5, -0, 10,
                                                               10, -0, -0, -0, -0, -0, -0, 10,
                                                               20, 10, 10,  5,  5, 10, 10, 20,
															                                 //White Bishop
                                                              -20,-10,-10,-10,-10,-10,-10,-20,
                                                              -10,  0,  0,  0,  0,  0,  0,-10,
                                                              -10,  0,  5, 10, 10,  5,  0,-10,
                                                              -10,  5,  5, 10, 10,  5,  5,-10,
                                                              -10,  0, 10, 10, 10, 10,  0,-10,
                                                              -10, 10, 10, 10, 10, 10, 10,-10,
                                                              -10,  5,  0,  0,  0,  0,  5,-10,
                                                              -20,-10,-10,-10,-10,-10,-10,-20,
                                                               // Black Bishop
                                                               20, 10, 10, 10, 10, 10, 10, 20,
                                                               10, -5,  0,  0,  0,  0, -5, 10,
                                                               10,-10,-10,-10,-10,-10,-10, 10,
                                                               10,  0,-10,-10,-10,-10,  0, 10,
                                                               10, -5, -5,-10,-10, -5, -5, 10,
                                                               10,  0, -5,-10,-10, -5,  0, 10,
                                                               10,  0,  0,  0,  0,  0,  0, 10,
                                                               20, 10, 10, 10, 10, 10, 10, 20,
                                                               // White Knight
                                                              -50,-40,-30,-30,-30,-30,-40,-50,
                                                              -40,-20,  0,  0,  0,  0,-20,-40,
                                                              -30,  0, 10, 15, 15, 10,  0,-30,
                                                              -30,  5, 15, 20, 20, 15,  5,-30,
                                                              -30,  0, 15, 20, 20, 15,  0,-30,
                                                              -30,  5, 10, 15, 15, 10,  5,-30,
                                                              -40,-20,  0,  5,  5,  0,-20,-40,
                                                              -50,-40,-30,-30,-30,-30,-40,-50,
                                                               // Black Knight
                                                               50, 40, 30, 30, 30, 30, 40, 50,
                                                               40, 20,  0, -5, -5,  0, 20, 40,
                                                               30, -5,-10,-15,-15,-10, -5, 30,
                                                               30,  0,-15,-20,-20,-15,  0, 30,
                                                               30, -5,-15,-20,-20,-15, -5, 30,
                                                               30,  0,-10,-15,-15,-10,  0, 30,
                                                               40, 20,  0,  0,  0,  0, 20, 40,
                                                               50, 40, 30, 30, 30, 30, 40, 50,
                                                               // White Rook
                                                                0,  0,  0,  0,  0,  0,  0,  0,
                                                                5, 10, 10, 10, 10, 10, 10,  5,
                                                               -5,  0,  0,  0,  0,  0,  0, -5,
                                                               -5,  0,  0,  0,  0,  0,  0, -5,
                                                               -5,  0,  0,  0,  0,  0,  0, -5,
                                                               -5,  0,  0,  0,  0,  0,  0, -5,
                                                               -5,  0,  0,  0,  0,  0,  0, -5,
                                                              -20,  0,  0,  5,  5,  0,  0,-20,
                                                               // Black Rook
                                                               20,  0,  0, -5, -5,  0,  0, 20,
                                                                5,  0,  0,  0,  0,  0,  0,  5,
                                                                5,  0,  0,  0,  0,  0,  0,  5,
                                                                5,  0,  0,  0,  0,  0,  0,  5,
                                                                5,  0,  0,  0,  0,  0,  0,  5,
                                                                5,  0,  0,  0,  0,  0,  0,  5,
                                                               -5,-10,-10,-10,-10,-10,-10, -5,
                                                                0,  0,  0,  0,  0,  0,  0,  0,
                                                               // White Pawn
                                                                0,  0,  0,  0,  0,  0,  0,  0,
                                                               50, 50, 50, 50, 50, 50, 50, 50,
                                                               10, 10, 20, 30, 30, 20, 10, 10,
                                                                5,  5, 10, 25, 25, 10,  5,  5,
                                                                0,  0,  0, 20, 20,  0,  0,  0,
                                                                5, -5,-10,  0,  0,-10, -5,  5,
                                                                5, 10, 10,-20,-20, 10, 10,  5,
                                                                0,  0,  0,  0,  0,  0,  0,  0,
                                                               // Black Pawn
                                                                0,  0,  0,  0,  0,  0,  0,  0,
                                                               -5,-10,-10, 20, 20,-10,-10, -5,
                                                               -5,  5, 10,  0,  0, 10,  5, -5,
                                                                0,  0,  0,-20,-20,  0,  0,  0,
                                                               -5, -5,-10,-25,-25,-10, -5, -5,
                                                              -10,-10,-20,-30,-30,-20,-10,-10,
                                                              -50,-50,-50,-50,-50,-50,-50,-50,
                                                                0,  0,  0,  0,  0,  0,  0,  0 };

    constexpr int PIECE_SQUARE_TABLES_END_GAME[64 * 12] = {  // White King
		                                                        -20,-10,-10,-10,-10,-10,-10,-20,
                                                             -5,  0,  5,  5,  5,  5,  0, -5,
                                                            -10, -5, 20, 30, 30, 20, -5,-10,
                                                            -15,-10, 35, 45, 45, 35,-10,-15,
                                                            -20,-15, 30, 40, 40, 30,-15,-20,
                                                            -25,-20, 20, 25, 25, 20,-20,-25,
                                                            -30,-25,  0,  0,  0,  0,-25,-30,
                                                            -50,-30,-30,-30,-30,-30,-30,-50,
                                                             // Black King
                                                             50, 30, 30, 30, 30, 30, 30, 50,
                                                             30, 25,  0,  0,  0,  0, 25, 30,
                                                             25, 20,-20,-25,-25,-20, 20, 25,
                                                             20, 15,-30,-40,-40,-30, 15, 20,
                                                             15, 10,-35,-45,-45,-35, 10, 15,
                                                             10,  5,-20,-30,-30,-20,  5, 10,
                                                              5,  0, -5, -5, -5, -5,  0,  5,
                                                             20, 10, 10, 10, 10, 10, 10, 20,
                                                             // White Queen
                                                            -20,-10,-10, -5, -5,-10,-10,-20,
                                                            -10,  0,  0,  0,  0,  0,  0,-10,
                                                            -10,  0,  5,  5,  5,  5,  0,-10,
                                                             -5,  0,  5,  5,  5,  5,  0, -5,
                                                              0,  0,  5,  5,  5,  5,  0, -5,
                                                            -10,  5,  5,  5,  5,  5,  0,-10,
                                                            -10,  0,  5,  0,  0,  0,  0,-10,
                                                            -20,-10,-10, -5, -5,-10,-10,-20,
                                                             // Black Queen
                                                             20, 10, 10,  5,  5, 10, 10, 20,
                                                             10,  0, -5,  0,  0,  0,  0, 10,
                                                             10, -5, -5, -5, -5, -5,  0, 10,
                                                              0,  0, -5, -5, -5, -5,  0,  5,
                                                              5,  0, -5, -5, -5, -5, -0,  5,
                                                             10, -0, -5, -5, -5, -5, -0, 10,
                                                             10, -0, -0, -0, -0, -0, -0, 10,
                                                             20, 10, 10,  5,  5, 10, 10, 20,
                                                             // White Bishop
                                                            -20,-10,-10,-10,-10,-10,-10,-20,
                                                            -10,  0,  0,  0,  0,  0,  0,-10,
                                                            -10,  0,  5, 10, 10,  5,  0,-10,
                                                            -10,  5,  5, 10, 10,  5,  5,-10,
                                                            -10,  0, 10, 10, 10, 10,  0,-10,
                                                            -10, 10, 10, 10, 10, 10, 10,-10,
                                                            -10,  5,  0,  0,  0,  0,  5,-10,
                                                            -20,-10,-10,-10,-10,-10,-10,-20,
                                                             // Black Bishop
                                                             20, 10, 10, 10, 10, 10, 10, 20,
                                                             10, -5,  0,  0,  0,  0, -5, 10,
                                                             10,-10,-10,-10,-10,-10,-10, 10,
                                                             10,  0,-10,-10,-10,-10,  0, 10,
                                                             10, -5, -5,-10,-10, -5, -5, 10,
                                                             10,  0, -5,-10,-10, -5,  0, 10,
                                                             10,  0,  0,  0,  0,  0,  0, 10,
                                                             20, 10, 10, 10, 10, 10, 10, 20,
                                                             // White Knight
                                                            -50,-40,-30,-30,-30,-30,-40,-50,
                                                            -40,-20,  0,  0,  0,  0,-20,-40,
                                                            -30,  0, 10, 15, 15, 10,  0,-30,
                                                            -30,  5, 15, 20, 20, 15,  5,-30,
                                                            -30,  0, 15, 20, 20, 15,  0,-30,
                                                            -30,  5, 10, 15, 15, 10,  5,-30,
                                                            -40,-20,  0,  5,  5,  0,-20,-40,
                                                            -50,-40,-30,-30,-30,-30,-40,-50,
                                                             // Black Knight
                                                             50, 40, 30, 30, 30, 30, 40, 50,
                                                             40, 20,  0, -5, -5,  0, 20, 40,
                                                             30, -5,-10,-15,-15,-10, -5, 30,
                                                             30,  0,-15,-20,-20,-15,  0, 30,
                                                             30, -5,-15,-20,-20,-15, -5, 30,
                                                             30,  0,-10,-15,-15,-10,  0, 30,
                                                             40, 20,  0,  0,  0,  0, 20, 40,
                                                             50, 40, 30, 30, 30, 30, 40, 50,
															                               // White Rook
                                                              0,  0,  0,  0,  0,  0,  0,  0,
                                                              5, 10, 10, 10, 10, 10, 10,  5,
                                                             -5,  0,  0,  0,  0,  0,  0, -5,
                                                             -5,  0,  0,  0,  0,  0,  0, -5,
                                                             -5,  0,  0,  0,  0,  0,  0, -5,
                                                             -5,  0,  0,  0,  0,  0,  0, -5,
                                                             -5,  0,  0,  0,  0,  0,  0, -5,
                                                              0,  0,  0,  5,  5,  0,  0,  0,
													                              		 // Black Rook
                                                              0,  0,  0, -5, -5,  0,  0,  0,
                                                              5,  0,  0,  0,  0,  0,  0,  5,
                                                              5,  0,  0,  0,  0,  0,  0,  5,
                                                              5,  0,  0,  0,  0,  0,  0,  5,
                                                              5,  0,  0,  0,  0,  0,  0,  5,
                                                              5,  0,  0,  0,  0,  0,  0,  5,
                                                             -5,-10,-10,-10,-10,-10,-10, -5,
                                                              0,  0,  0,  0,  0,  0,  0,  0,
                                                             // White Pawn
                                                              0,  0,  0,  0,  0,  0,  0,  0,
                                                             70, 60, 60, 60, 60, 60, 60, 70,
                                                             40, 40, 40, 40, 40, 40, 40, 40,
                                                             30, 30, 30, 30, 30, 30, 30, 30,
                                                             20, 20, 20, 20, 20, 20, 20, 20,
                                                             10, 10, 10, 10, 10, 10, 10, 10,
                                                              0,  0,  0,  0,  0,  0,  0,  0,
                                                              0,  0,  0,  0,  0,  0,  0,  0,
                                                             // Black Pawn
                                                              0,  0,  0,  0,  0,  0,  0,  0,
                                                              0,  0,  0,  0,  0,  0,  0,  0,
                                                            -10,-10,-10,-10,-10,-10,-10,-10,
                                                            -20,-20,-20,-20,-20,-20,-20,-20,
                                                            -30,-30,-30,-30,-30,-30,-30,-30,
                                                            -40,-40,-40,-40,-40,-40,-40,-40,
                                                            -70,-60,-60,-60,-60,-60,-60,-70,
                                                              0,  0,  0,  0,  0,  0,  0,  0 };

    constexpr uint64_t ROOK_MAGICS[64] = { 9259400989436281217u, 18014673655848964u, 144132823215906944u, 612498347570233348u, 2522022422773761168u, 72097176494604296u, 72058693692293124u, 72057732560650246u, 9224638676399570944u, 72198882355847168u, 281544776220928u, 9237586626283048960u, 4616471127393632272u, 1729523002988822656u, 18296973065060353u, 4629981892995844352u, 35734165652096u, 2393088131735560u, 4616472193079972160u, 5770788120569858u, 5068748872755232u, 721702390117435904u, 6126479058936098u, 1207263776153668u, 35736275402760u, 4506093929635856u, 281556585291776u, 189714727009533984u, 72066392278696064u, 281487861874696u, 290763655237468228u, 2341873206392029444u, 35459258384512u, 4904595984807363648u, 576495940987273296u, 6994661969525485568u, 13572373689140224u, 2379027091787093248u, 4505833614353200u, 9223372947421405313u, 612489825811480578u, 287112154972224u, 4503737074745362u, 5836770738924748812u, 281578060447762u, 4398080098432u, 1166511695993241616u, 577868133650464786u, 2316257894737651968u, 6053436035663134848u, 4647759354559250944u, 578994645575403776u, 288274358768763008u, 9225065286911262848u, 288232579617784832u, 550896861696u, 378865633007345794u, 578994165606519427u, 590025431453953u, 105605343752193u, 18577486171866242u, 18296509141889539u, 8800698519564u, 2414106011477737730u };
    constexpr int ROOK_SHIFTS[64] = { 52, 53, 53, 53, 53, 53, 53, 52, 53, 54, 54, 54, 54, 54, 54, 53, 53, 54, 54, 54, 54, 54, 54, 53, 53, 54, 54, 54, 54, 54, 54, 53, 53, 54, 54, 54, 54, 54, 54, 53, 53, 54, 54, 54, 54, 54, 54, 53, 53, 54, 54, 54, 54, 54, 54, 53, 52, 53, 53, 53, 53, 53, 53, 52 };
    constexpr uint64_t BISHOP_MAGICS[64] = { 10493393737567183232u, 1155472375882515472u, 9299938221821788737u, 585619694904410120u, 726218777666019328u, 282712195989504u, 18304704124026880u, 3495094579376425024u, 616500584987525632u, 1152939165520823104u, 9223939560982052864u, 27048280332435466u, 36038764564316160u, 6922052452907024396u, 4787277939622032u, 216174983855018056u, 4538870176678208u, 2310347727730901248u, 9224502618343342088u, 562954793779219u, 282594088321026u, 10556033315702800u, 13836183956582041696u, 2170770209126877697u, 1158058423200581648u, 2310348808133038336u, 128353723385774629u, 37163493037711424u, 9241531571024977928u, 4906678395408089345u, 1225271577332433932u, 563501353435265u, 1229557611713528320u, 113768675739767312u, 9295500275589054992u, 2254000989143169u, 2254153455911168u, 58547349743600129u, 577595807004492416u, 9223515050709370112u, 82191934584655872u, 1154619154993517888u, 18157369414386432u, 144150656050004864u, 144141606424151040u, 1206966126081212448u, 4789476966532096u, 94875905020660224u, 703125110383446025u, 1374398294540288u, 5766861526185099536u, 10379671896232034308u, 594478586824753164u, 598983285992917248u, 4508002170437634u, 9243678952575877120u, 1297111464910259200u, 576460898469283908u, 2026901483391356935u, 290554760921353216u, 13651542016133632u, 5332262029943111970u, 9223407509258764424u, 9082045636558976u };
    constexpr int BISHOP_SHIFTS[64] = { 58, 59, 59, 59, 59, 59, 59, 58, 59, 59, 59, 59, 59, 59, 59, 59, 59, 59, 57, 57, 57, 57, 59, 59, 59, 59, 57, 55, 55, 57, 59, 59, 59, 59, 57, 55, 55, 57, 59, 59, 59, 59, 57, 57, 57, 57, 59, 59, 59, 59, 59, 59, 59, 59, 59, 59, 58, 59, 59, 59, 59, 59, 59, 58 };
}
#endif