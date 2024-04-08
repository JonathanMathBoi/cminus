#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>
#include "MiscUtils.hpp"
#include "ast/AST.hpp"
#include "ast/PrintVisitor.hpp"
#include "codegen/CodegenVisitor.hpp"
#include "lexer/Lexer.hpp"
#include "parser/Parser.hpp"
#include "semantics/SemanticVisitor.hpp"
#include "symbol/SymbolVisitor.hpp"

#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <memory>
#include <ostream>
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
        SymbolVisitor symbol_visitor;
        ast->accept(symbol_visitor);
    } catch (CMinusException const& exception) {
        std::cout << exception.what() << std::endl;
        return -1;
    }

    SemanticVisitor semantic_visitor;
    ast->accept(semantic_visitor);
    if (!semantic_visitor.isValid()) {
        for (auto& error : semantic_visitor.errors()) {
            std::cout << error.message() << '\n';
        }
        std::cout << std::flush;
        return -1;
    }

    std::cout << "Valid!\n";

    std::filesystem::path old_path {input_file};
    std::filesystem::path new_path {old_path.parent_path() / old_path.stem()};
    new_path += ".ast";

    std::ofstream ast_file {new_path, std::ios::trunc};

    PrintVisitor printer {ast_file};

    ast->accept(printer);

    std::cout << "AST saved to " << new_path << std::endl;

    auto context {std::make_shared<llvm::LLVMContext>()};
    auto module {std::make_shared<llvm::Module>("cmprogram", *context)};

    CodegenVisitor codegen {context, module};
    ast->accept(codegen);
    module->print(llvm::errs(), /*AAW=*/nullptr);
}
