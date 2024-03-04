#include "Exception.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"

#include <format>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

/***********************************************************************/

std::string
token_value (Token tok)
{
    switch (tok.type) {
    case ID:
        return tok.lexeme;
    case NUM:
        return std::to_string (tok.number);
    default:
        return "";
    }
}

std::string
quote (std::string str)
{
    std::stringstream ss;
    ss << std::quoted (str);
    std::string quoted;
    ss >> quoted;
    return quoted;
}

void
print_token (Token tok, int line_num, int col_num)
{
    if (tok.type == ERROR) {
        std::cout << std::format (
                "{:20s}{:20s}Line: {:d}; Column: {:d}",
                token_types.at(tok.type),
                quote (tok.lexeme),
                line_num,
                col_num
            ) << std::endl;
    } else {
        std::cout << std::format(
                "{:20s}{:20s}{:s}", 
                token_types.at (tok.type),
                quote (tok.lexeme),
                token_value (tok)
            ) << std::endl;
    }
}

/***********************************************************************/

int main (int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "No source file provided" << std::endl;
        return 1;
    }

    std::ifstream source {argv[1]};

    lexer lexer {std::move (source)};

    parser parser {std::move(lexer)};

    try {
        parser.parse();
        std::cout << "Valid!" << std::endl;
    } catch (cminus_exception const& exception) {
        std::cout << exception.what() << std::endl;
    }
}



