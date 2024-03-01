#include "Lexer.hpp"

#include <format>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

/***********************************************************************/

const std::map<TokenType, std::string> token_types {
    {END_OF_FILE, "END_OF_FILE"},
    {ERROR, "ERROR"},
    {IF, "IF"},
    {ELSE, "ELSE"},
    {INT, "INT"},
    {VOID, "VOID"},
    {RETURN, "RETURN"},
    {WHILE, "WHILE"},
    {PLUS, "PLUS"},
    {MINUS, "MINUS"},
    {TIMES, "TIMES"},
    {DIVIDE, "DIVIDE"},
    {LT, "LT"},
    {LTE, "LTE"},
    {GT, "GT"},
    {GTE, "GTE"},
    {EQ, "EQ"},
    {NEQ, "NEQ"},
    {ASSIGN, "ASSIGN"},
    {INCREMENT, "INCREMENT"},
    {DECREMENT, "DECREMENT"},
    {SEMI, "SEMI"},
    {COMMA, "COMMA"},
    {LPAREN, "LPAREN"},
    {RPAREN, "RPAREN"},
    {LBRACK, "LBRACK"},
    {RBRACK, "RBRACK"},
    {LBRACE, "LBRACE"},
    {RBRACE, "RBRACE"},
    {ID, "ID"},
    {NUM, "NUM"}
};

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

    std::cout << "TOKEN               LEXEME              VALUE\n"
              << "=====               ======              =====" << std::endl;

    Token token {END_OF_FILE};
    do {
        token = lexer.get_token ();
        print_token(token, lexer.get_line_num(), lexer.get_column_num());
    } while (token.type != END_OF_FILE);
}



