#include "Exception.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"

#include <fstream>
#include <iostream>

/***********************************************************************/

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "No source file provided" << std::endl;
        return 1;
    }

    std::ifstream source {argv[1]};

    lexer lexer {std::move(source)};

    parser parser {std::move(lexer)};

    try {
        parser.parse();
        std::cout << "Valid!" << std::endl;
    } catch (cminus_exception const& exception) {
        std::cout << exception.what() << std::endl;
    }
}
