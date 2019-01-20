#include <algorithm>
#include <assert.h>
#include <fcntl.h>
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
#include "parser.h"

void adjustForRedirect(program_cmd_t cmd) {
	if (cmd.does_redirect_files[0]) {
		int rd_fd = open(cmd.redirect_files[0].c_str(), O_RDONLY);
		int dup_res = dup2(rd_fd, STDIN_FILENO);
		close(rd_fd);
	}

	if (cmd.does_redirect_files[1]) {
		mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
		int wrt_fd = open(cmd.redirect_files[1].c_str(), O_WRONLY|O_CREAT|O_TRUNC, mode);
		int dup_res = dup2(wrt_fd, STDOUT_FILENO);
		close(wrt_fd);
	}
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

/* Pipe hierarchy is laid out from last cmd -> first. */
void pipeAndExecRec(std::vector<program_cmd_t>& parsed_cmd, int pos) {
	assert(pos >= 0);
	auto cmd = parsed_cmd[pos];
	auto args = convertArgsForExec(cmd.args);
	adjustForRedirect(cmd);

	if (pos == 0) {
		execvp(cmd.program_exec.c_str(), args);
		free(args);
		exit(EXIT_SUCCESS);
	}

	int pipefd[2];
	if (pipe(pipefd) == -1) {
		perror("pipe");
		exit(EXIT_FAILURE);
	}

	pid_t pid = fork();

	if (pid == 0) {
		close(pipefd[0]);
		int dup_res = dup2(pipefd[1], STDOUT_FILENO);
		close(pipefd[1]);
		pipeAndExecRec(parsed_cmd, pos - 1);
	} else {
		close(pipefd[1]);
		int dup_res = dup2(pipefd[0], STDIN_FILENO);
		close(pipefd[0]);

		int status;
		wait(&status);
		std::cout << " STATUS FROM POS " << pos << ": " << status << std::endl;

		execvp(cmd.program_exec.c_str(), args);
		free(args);
		exit(EXIT_SUCCESS);
	}
}

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
		// Parser::printParsedCmd(parsed_cmd);

		pid_t pid = fork();
		if (pid == 0) {
			pipeAndExecRec(parsed_cmd, parsed_cmd.size() - 1);
		} else {
			int status;
			wait(&status);
			std::cout << " STATUS: " << status << std::endl;
		}
	}
}