#include <algorithm>
#include <assert.h>
#include <cstdio>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <signal.h>
#include <sstream>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
#include "parser.h"

namespace cmd {
	int child_stderr_fd, redirect_stderr_fd;
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

	void handleExit(int status) {
		if (WIFEXITED(status) && WEXITSTATUS(status) == 1) {
			char err[512];
			fgets(err, 512, fdopen(child_stderr_fd, "r"));
			std::string err_s(err);
			err_s = "ERROR: " + err_s;

			write(cmd::redirect_stderr_fd, err_s.c_str(), err_s.length());
		}
	}
	
	void handler(int sig) {
		int status;
		pid_t pid = waitpid(-1, &status, WNOHANG);
		if (pid > 0) {
			cmd::handleExit(status);
		}
	}

	bool read(std::string& cmd, bool silent_prompt) {
		if (!silent_prompt) std::cout << "Shell: ";
		std::getline(std::cin, cmd);
		return !std::cin.eof();
	}

	void run(program_cmd_t cmd);
}

void perrorToRedirectFd(const char* str) {
	std::ostringstream err_ss;
	err_ss << str << ": " << strerror(errno) << std::endl;
	std::string err_str = err_ss.str();

	write(cmd::redirect_stderr_fd, err_str.c_str(), err_str.length());
}

void adjustForRedirect(program_cmd_t cmd) {
	if (cmd.does_redirect_files[0]) {
		int rd_fd = open(cmd.redirect_files[0].c_str(), O_RDONLY);
		if (rd_fd == -1) {
			perrorToRedirectFd("ERROR");
			exit(errno);
		}
		if (dup2(rd_fd, STDIN_FILENO)) {
			perrorToRedirectFd("ERROR");
		}
		close(rd_fd);
	}

	if (cmd.does_redirect_files[1]) {
		mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
		int wrt_fd = open(cmd.redirect_files[1].c_str(), O_WRONLY|O_CREAT|O_TRUNC, mode);
		if (wrt_fd == -1) {
			perrorToRedirectFd("ERROR");
			exit(errno);
		}
		if (dup2(wrt_fd, STDOUT_FILENO) == -1) {
			perrorToRedirectFd("ERROR");
		}
		close(wrt_fd);
	}
}

void cmd::run(program_cmd_t cmd) {
	auto args = convertArgsForExec(cmd.args);
	adjustForRedirect(cmd);
	if (execvp(cmd.program_exec.c_str(), args) == -1) {
		perrorToRedirectFd("ERROR");
		exit(errno);
	}
	free(args);
	exit(EXIT_SUCCESS);
}

/* Pipe hierarchy is laid out from last cmd -> first. */
void pipeAndExecRec(std::vector<program_cmd_t>& parsed_cmd, int pos) {
	assert(pos >= 0);
	auto cmd = parsed_cmd[pos];
	
	if (pos == 0) {
		cmd::run(cmd);
	}

	int pipefd[2];
	if (pipe(pipefd) == -1) {
		perrorToRedirectFd("ERROR");
		exit(EXIT_FAILURE);
	}

	pid_t pid = fork();

	if (pid == 0) {
		close(pipefd[0]);
		dup2(pipefd[1], STDOUT_FILENO);
		close(pipefd[1]);
		pipeAndExecRec(parsed_cmd, pos - 1);
	} else {
		close(pipefd[1]);
		dup2(pipefd[0], STDIN_FILENO);
		close(pipefd[0]);

		int status;
		wait(&status);
		cmd::handleExit(status);
		cmd::run(cmd);
	}
}

int main(int argc, char* argv[]) {
	bool silent_prompt = false;
	if (argc == 2 && strcmp(argv[1], "-n") == 0) {
		silent_prompt = true;
	}
	std::string cmd;
	int stderr_pipefd[2] = { 0, 0 };
	if (pipe(stderr_pipefd) == -1) {
		perrorToRedirectFd("ERROR");
		exit(EXIT_FAILURE);
	}
	cmd::child_stderr_fd = stderr_pipefd[0];
	signal(SIGCHLD, cmd::handler);
	cmd::redirect_stderr_fd = dup(STDERR_FILENO);
	if (cmd::redirect_stderr_fd == -1) {
		perrorToRedirectFd("ERROR");
		exit(EXIT_FAILURE);
	}
	dup2(stderr_pipefd[1], STDERR_FILENO);
	close(stderr_pipefd[1]);

	while (cmd::read(cmd, silent_prompt)) {
		if (cmd == "") continue;
		bool background = (cmd.back() == '&');
		if (background) {
			cmd = cmd.substr(0, cmd.length() - 1);
		}

		auto parsed_cmd = Parser::getParsedCmd(cmd);
		// Parser::printParsedCmd(parsed_cmd);

		if (parsed_cmd.size() == 1 && parsed_cmd.front().program_exec == "cd") {
			/* Special case of cd. Uses chdir syscall to actually change working directory */
			std::string cd_path = parsed_cmd.front().args[1];
			if (chdir(cd_path.c_str()) == -1) {
				perrorToRedirectFd("ERROR");
			}
			continue;	
		}

		pid_t pid = fork();
		if (pid == 0) {
			pipeAndExecRec(parsed_cmd, parsed_cmd.size() - 1);
		} else {
			if (!background) {
				int status;
				wait(&status);
				cmd::handleExit(status);
			}
		}
	}
}
