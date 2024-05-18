#include <iostream>

#include "engine.h"

using namespace std;

int main() {
	Engine engine;
	string command;

	while (command != "quit") {
		getline(cin, command);
		engine.receiveCommand(command);
	}
}