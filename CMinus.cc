#include "AST.hpp"
#include "Lexer.hpp"
#include "MiscUtils.hpp"
#include "Parser.hpp"
#include "PrintVisitor.hpp"

#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <memory>
#include <string_view>

/***********************************************************************/

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "No source file provided" << std::endl;
        return 1;
    }

    const std::string_view input_file {argv[1]};

    // ifstream needs a full std::string or raw char const*
    std::ifstream source {input_file.data()};

    Lexer lexer {std::move(source)};

    Parser parser {std::move(lexer)};

    std::unique_ptr<Node> ast;

    try {
        ast = parser.parse();
        std::cout << "Valid!" << std::endl;
    } catch (CMinusException const& exception) {
        std::cout << exception.what() << std::endl;
        return -1;
    }

    std::filesystem::path old_path {input_file};
    std::filesystem::path new_path {old_path.parent_path() / old_path.stem()};
    new_path += ".ast";

    std::ofstream ast_file {new_path, std::ios::trunc};

    PrintVisitor printer {ast_file};

    ast->accept(printer);
}
