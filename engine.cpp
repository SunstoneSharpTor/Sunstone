#include <iostream>
#include <sstream>
#include <fstream>
#include <chrono>

#include "engine.h"

using namespace std;

void Engine::receiveCommand(string command) {

	ofstream myfile;
	myfile.open("log.txt", ios::app);
	string logLine = command;
	myfile << logLine << "\n";
	myfile.close();

	stringstream stream(command);
	string word;
	stream >> word;

	if (word == "go") {
		//calculate the best move
		unsigned char from, to, flags;
		int currentDepth, eval;
		bool cancelSearch = false;
		m_board.numPositions = 0;
		auto tp = chrono::high_resolution_clock::now();
		iterativeDeepeningSearch(&currentDepth, &cancelSearch, &eval, &from, &to, &flags);
		int timeSearched = chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - tp).count();

		//send info command
		string info = "info";
		info.append(" depth ");
		info.append(to_string(currentDepth - 1));
		info.append(" seldepth ");
		info.append(to_string(currentDepth - 1));
		info.append(" multipv 1");
		if (eval > 900000000) {
			info.append(" score mate ");
			info.append(to_string((1000000000 - eval + 1) / 2));
		}
		else if (eval < -900000000) {
			info.append(" score mate ");
			info.append(to_string((1000000000 + eval + 1) / -2));
		}
		else {
			info.append(" score cp ");
			info.append(to_string(eval));
		}
		info.append(" nodes ");
		info.append(to_string(m_board.numPositions));
		info.append(" nps ");
		if (timeSearched > 0) {
			info.append(to_string(m_board.numPositions * 1000 / timeSearched));
		}
		else {
			info.append(to_string(m_board.numPositions * 1000));
		}
		info.append(" time ");
		info.append(to_string(timeSearched));

		info.append("\n");
		cout << info;



		//send bestmove command
		string bestMove = "bestmove ";
		bestMove.append(m_board.getMoveName(from, to, flags));
		bestMove.append("\n");
		cout << bestMove;

		/*myfile.open("log.txt", ios::app);
		string logLine = bestMove;
		myfile << logLine << "\n";
		myfile.close();*/
	}

	if (word == "position") {
		//load the initial position
		stream >> word;
		if (word == "startpos") {
			m_board.loadFromFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
		}
		else if (word == "fen") {
			string fen = "";
			for (int i = 0; i < 6; i++) {
				stream >> word;
				fen.append(word);
				fen.append(" ");
			}

			m_board.loadFromFen(fen);
		}

		//play all the moves that have been played since the initial position
		stream >> word;
		if (word == "moves") {
			while (stream >> word) {
				char from = m_board.getSquareNumFromString(word.substr(0, 2));
				char to = m_board.getSquareNumFromString(word.substr(2, 2));
				char flags;
				if (word.size() > 4) {
					switch (word[4]) {
					case 'q':
						flags = 2;
						break;
					case 'r':
						flags = 8;
						break;
					case 'b':
						flags = 4;
						break;
					case 'n':
						flags = 6;
						break;
					default:
						flags = 0;
						break;
					}
				}
				else {
					flags = 0;
				}

				//TEMP
				/*if ((m_board.m_ply % 2 == 1)) {
					//calculate the best move
					unsigned char Ffrom, Fto, Fflags;
					int currentDepth, eval;
					bool cancelSearch = false;
					m_board.numPositions = 0;
					auto tp = chrono::high_resolution_clock::now();
					iterativeDeepeningSearch(&currentDepth, &cancelSearch, &eval, &Ffrom, &Fto, &Fflags);
					int timeSearched = chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - tp).count();
				}*/


				m_board.makeMove(from, to, flags);
			}
		}
	}

	if (word == "isready") {
		cout << "readyok\n";
	}

	if (word == "ucinewgame") {
		//i think it should reset things like the transposition table here
	}

	if (word == "uci") {
		cout << "id name Sunstone 1.9\n";
		cout << "id author Bertie Cartwright\n\n";
		cout << "uciok\n";
	}
}


void Engine::iterativeDeepeningSearch(int* currentDepth, bool* cancelSearch, int* eval, unsigned char* bestMoveFrom, unsigned char* bestMoveTo, unsigned char* bestMoveFlags) {
	*currentDepth = 1;
	int bestMoveNum = 0;

	auto startTime = chrono::high_resolution_clock::now();

	unsigned char currentBestMoveFrom, currentBestMoveTo, currentBestMoveFlags;
	currentBestMoveFrom = currentBestMoveTo = currentBestMoveFlags = 0;

	while (!(*cancelSearch)) {
		*bestMoveFrom = currentBestMoveFrom;
		*bestMoveTo = currentBestMoveTo;
		*bestMoveFlags = currentBestMoveFlags;
		(*currentDepth)++;

		int timeSearched = chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - startTime).count();
		if (timeSearched > 50) {
			return;
		}

		*eval = m_board.findBestMove(cancelSearch, &currentBestMoveFrom, &currentBestMoveTo, &currentBestMoveFlags, *currentDepth, &bestMoveNum);
	}
}