#include <iostream>
#include <vector>
#include "parser.h"

bool readCmd(std::string& cmd) {
	std::cout << "Shell: ";
	std::getline(std::cin, cmd);
	return !std::cin.eof();
}

int main() {
	std::string cmd;
	while (readCmd(cmd)) {
		bool background = (cmd.back() == '&');
		if (background) {
			cmd = cmd.substr(0, cmd.length() - 1);
		}

		auto parsed_cmd = Parser::getParsedCmd(cmd);
		Parser::printParsedCmd(parsed_cmd);
	}
}