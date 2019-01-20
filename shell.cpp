#include <algorithm>
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
#include "parser.h"

bool readCmd(std::string& cmd) {
	std::cout << "Shell: ";
	std::getline(std::cin, cmd);
	return !std::cin.eof();
}

char** const convertArgsForExec(const std::vector<std::string>& cmd_args) {
	std::vector<char *> cmd_buf(cmd_args.size());
	std::transform(cmd_args.begin(), cmd_args.end(), cmd_buf.begin(), [](const std::string& arg) {
		return const_cast<char*>(arg.c_str());
	});

	cmd_buf.push_back(nullptr);
	char** args = new char*[cmd_buf.size()];
	std::copy(cmd_buf.begin(), cmd_buf.end(), args);
	return args;
}

int main() {
	std::string cmd;
	while (readCmd(cmd)) {
		bool background = (cmd.back() == '&');
		if (background) {
			cmd = cmd.substr(0, cmd.length() - 1);
		}

		auto parsed_cmd = Parser::getParsedCmd(cmd);
		// Parser::printParsedCmd(parsed_cmd);

		program_cmd_t cmd = parsed_cmd[0];

		pid_t pid = fork();
		if (pid == 0) {
			auto args = convertArgsForExec(cmd.args);
			execvp(cmd.program_exec.c_str(), args);
			free(args);
			exit(0);
		} else {
			int status;
			wait(&status);
			std::cout << " STATUS: " << status << std::endl;
		}
	}
}