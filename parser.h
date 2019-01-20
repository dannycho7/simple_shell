#include <vector>

#ifndef PARSER_H
#define PARSER_H

struct program_cmd_t {
	std::string program_exec;
	std::vector<std::string> args;
	bool does_redirect_files[2] = { false, false }; // 0 => input, 1 => output
	std::string redirect_files[2];
};

namespace Parser {
	std::vector<program_cmd_t> getParsedCmd(const std::string cmd);	
	void printParsedCmd(std::vector<program_cmd_t> parsed_cmd);
}

#endif