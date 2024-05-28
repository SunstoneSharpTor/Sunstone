#include <iostream>
#include <sstream>
#include <fstream>
#include <chrono>
#include <thread>

#include "engine.h"
#include "constants.h"

using namespace std;

void Engine::receiveCommand(string command) {
	// ofstream myfile;
	// myfile.open("log.txt", ios::app);
	// string logLine = command;
	// myfile << logLine << "\n";
	// myfile.close();

	stringstream stream(command);
	string word;
	stream >> word;

	if (word == "go") {
		int time = 0;
		string timeWord = m_board.getTurn() ? "btime" : "wtime";
		//find the time left
		while (stream >> word) {
			if (word == timeWord) {
				stream >> word;
				time = stoi(word);
			}
		}

		//calculate the best move
		unsigned char from, to, flags;
		int currentDepth, eval;
		bool cancelSearch = false;
		iterativeDeepeningSearch(time, &currentDepth, &cancelSearch, &eval, &from, &to, &flags);

		//send bestmove command
		string bestMove = "bestmove ";
		bestMove.append(m_board.getMoveName(from, to, flags));
		bestMove.append("\n");
		cout << bestMove;
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
					m_search.getNodeCount();
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
		cout << "id name Sunstone 1.12\n";
		cout << "id author Bertie Cartwright\n\n";
		cout << "uciok\n";
	}
}

void Engine::iterativeDeepeningSearch(int time, int* currentDepth, bool* cancelSearch, int* eval, unsigned char* bestMoveFrom, unsigned char* bestMoveTo, unsigned char* bestMoveFlags) {
	auto startTime = chrono::high_resolution_clock::now();
	int timeSearched = 0;
	int targetTime = time == 0 ? 10000 : time / 80;

	m_search.resetNodeCount();

	*eval = 0;
	*currentDepth = 0;
	int bestMoveNum = 0;

	// Check for only 1 legal move
	if (m_search.checkForSingleLegalMove(bestMoveFrom, bestMoveTo, bestMoveFlags) && (time != 0)) {
		timeSearched = chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - startTime).count();
		printInfo(timeSearched, *currentDepth, 0);

		return;
	}

	while (!(*cancelSearch)) {
		if ((timeSearched > targetTime) || ((std::abs(*eval) >= std::numeric_limits<int>::max() / 2 - constants::MAX_DEPTH) && (time != 0))) {
			return;
		}

		(*currentDepth)++;

		std::thread worker(&Engine::work, this, cancelSearch, bestMoveFrom, bestMoveTo, bestMoveFlags, *currentDepth, &bestMoveNum, eval);
		worker.join();
		timeSearched = chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - startTime).count();
		printInfo(timeSearched, *currentDepth, *eval);
	}
}

void Engine::work(bool* cancelSearch, unsigned char* from, unsigned char* to, unsigned char* flags, int depth, int* bestMoveNum, int* eval) {
	m_search.rootSearch(cancelSearch, from, to, flags, depth, bestMoveNum, eval);
}

void Engine::printInfo(int timeSearched, int currentDepth, int eval) {
	string info = "info";
	info.append(" depth ");
	info.append(to_string(currentDepth));
	info.append(" seldepth ");
	info.append(to_string(currentDepth));
	info.append(" multipv 1");
	if (eval >= std::numeric_limits<int>::max() / 2 - constants::MAX_DEPTH) {
		info.append(" score mate ");
		info.append(to_string((std::numeric_limits<int>::max() / 2 - eval + 1) / 2));
	}
	else if (eval <= -std::numeric_limits<int>::max() / 2+ constants::MAX_DEPTH) {
		info.append(" score mate ");
		info.append(to_string((std::numeric_limits<int>::max() / 2 + eval - 1) / -2));
	}
	else {
		info.append(" score cp ");
		info.append(to_string(eval));
	}
	info.append(" nodes ");
	info.append(to_string(m_search.getNodeCount()));
	info.append(" nps ");
	if (timeSearched > 0) {
		info.append(to_string(m_search.getNodeCount() * 1000 / timeSearched));
	}
	else {
		info.append(to_string(m_search.getNodeCount() * 1000));
	}
	info.append(" time ");
	info.append(to_string(timeSearched));

	info.append("\n");
	cout << info;
}