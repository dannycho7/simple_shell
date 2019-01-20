#include <iostream>
#include <sstream>
#include <vector>
#include "parser.h"

namespace Parser {
	void printParsedCmd(std::vector<program_cmd_t> parsed_cmd) {
		for (auto it = parsed_cmd.begin(); it != parsed_cmd.end(); it++) {
			std::cout << "Program Exec: " << it->program_exec << std::endl;
			std::cout << "Args: ";
			for (auto args_it = it->args.begin(); args_it != it->args.end(); args_it++) {
				std::cout << "|" << *args_it << "|";
			}
			std::cout << std::endl;
			std::cout << "does_redirect_files: " << it->does_redirect_files[0] << it->does_redirect_files[1] << std::endl;
			std::cout << "redirect_files: " << it->redirect_files[0] << "|" << it->redirect_files[1] << std::endl;
		}
	}

	std::string getNextToken(std::string program_cmd, int& start_pos) {
		int token_start_i = program_cmd.find_first_not_of(" ", start_pos);
		if (program_cmd[token_start_i] == '<' || program_cmd[token_start_i] == '>') {
			start_pos = token_start_i;
			return program_cmd.substr(token_start_i, 1);
		} else {
			int token_end_i = program_cmd.find_first_of(">< ", token_start_i);
			start_pos = token_end_i;

			return program_cmd.substr(token_start_i, token_end_i - token_start_i + 1);
		}
	}

	program_cmd_t getParsedProgramCmd(const std::string program_cmd) {
		int program_exec_i_start = program_cmd.find_first_not_of(" ");
		int program_exec_i_end = program_cmd.find_first_of(">< ", program_exec_i_start);
		program_cmd_t parsed_program_cmd;
		std::string program_exec = program_cmd.substr(program_exec_i_start, program_exec_i_end - program_exec_i_start);
		parsed_program_cmd.program_exec = program_exec;
		parsed_program_cmd.args.push_back(program_exec);

		int current_i = program_cmd.find_first_not_of(" ", program_exec_i_end);
		while (current_i != std::string::npos) {
			std::string token = getNextToken(program_cmd, current_i);

			if (token == "<") {
				parsed_program_cmd.does_redirect_files[0] = true;
				parsed_program_cmd.redirect_files[0] = getNextToken(program_cmd, ++current_i);
			} else if (token == ">") {
				parsed_program_cmd.does_redirect_files[1] = true;
				parsed_program_cmd.redirect_files[1] = getNextToken(program_cmd, ++current_i);
			} else {
				parsed_program_cmd.args.push_back(token);
			}
			current_i = program_cmd.find_first_not_of(" ", current_i);
		}

		return parsed_program_cmd;
	}

	std::vector<program_cmd_t> getParsedCmd(const std::string cmd) {
		std::stringstream cmd_ss(cmd);
		std::string program_cmd;
		std::vector<program_cmd_t> parsed_cmd;

		while (std::getline(cmd_ss, program_cmd, '|')) {
			parsed_cmd.push_back(getParsedProgramCmd(program_cmd));
		}

		return parsed_cmd;
	}
}