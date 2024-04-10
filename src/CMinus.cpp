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

#include <boost/program_options.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/positional_options.hpp>
#include <boost/program_options/value_semantic.hpp>
#include <boost/program_options/variables_map.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <memory>
#include <ostream>
#include <string_view>

namespace po = boost::program_options;

/***********************************************************************/

po::variables_map getCmdArgs(int argc, char* argv[]);

std::unique_ptr<Node> getAST(std::string source);

void linkSymbols(Node& ast);

/***********************************************************************/

int main(int argc, char* argv[]) {
    po::variables_map vm {getCmdArgs(argc, argv)};

    std::unique_ptr<Node> ast {getAST(vm["input-file"].as<std::string>())};

    linkSymbols(*ast);

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

    std::filesystem::path old_path {vm["input-file"].as<std::string>()};
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

/***********************************************************************/

po::variables_map getCmdArgs(int argc, char* argv[]) {
    // Define command line options
    po::options_description desc("Options");
    desc.add_options()("help,h", "Produce help message")(
        "optimize,O", po::value<int>()->default_value(0),
        "Optimization level (0, 1, 2, or 3)")(
        "input-file", po::value<std::string>(), "Input file");

    po::positional_options_description p;
    p.add("input-file", -1);

    // Parse the command line
    po::variables_map vm;
    po::store(
        po::command_line_parser(argc, argv).options(desc).positional(p).run(),
        vm);
    po::notify(vm);

    // Handle help option
    if (vm.count("help")) {
        std::cout << desc << std::endl;
        std::exit(1);
    }

    // Check if input file is provided
    if (!vm.count("input-file")) {
        std::cerr << "Error: Input file not specified!" << std::endl;
        std::exit(1);
    }

    return vm;
}

/***********************************************************************/

std::unique_ptr<Node> getAST(std::string source) {
    // If an input file is given, use the file, else use stdin
    Lexer lexer {source == "-" ? Lexer {} : Lexer {std::ifstream {source}}};
    Parser parser {std::move(lexer)};

    try {
        return parser.parse();
    } catch (CMinusException const& exception) {
        std::cout << exception.what() << std::endl;
        std::exit(1);
    }
}

/***********************************************************************/

void linkSymbols(Node& ast) {
    SymbolVisitor symbolVisitor;
    try {
        ast.accept(symbolVisitor);
    } catch (CMinusException const& exception) {
        std::cout << exception.what() << std::endl;
        std::exit(1);
    }
}

/***********************************************************************/
