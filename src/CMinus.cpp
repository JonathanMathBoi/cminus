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

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Passes/OptimizationLevel.h>
#include <llvm/Support/CodeGen.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Host.h>

#include <llvm/Analysis/CGSCCPassManager.h>
#include <llvm/Analysis/LoopAnalysisManager.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Passes/PassBuilder.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <memory>
#include <ostream>
#include <string_view>
#include <system_error>

namespace po = boost::program_options;

using llvm::CGSCCAnalysisManager;
using llvm::FunctionAnalysisManager;
using llvm::LoopAnalysisManager;
using llvm::ModuleAnalysisManager;
using llvm::ModulePassManager;
using llvm::OptimizationLevel;
using llvm::PassBuilder;

/***********************************************************************/

po::variables_map getCmdArgs(int argc, char* argv[]);
std::unique_ptr<Node> getAST(std::string source);
void linkSymbols(Node& ast);
void checkSemantics(Node& ast);
void printAST(Node& ast, std::filesystem::path outfile);
std::filesystem::path getOutputFile(po::variables_map& options);
void codegen(
    Node& ast,
    std::shared_ptr<llvm::LLVMContext> context,
    std::shared_ptr<llvm::Module> module);
void optimize(std::shared_ptr<llvm::Module> module, unsigned opt_level);
int assembleLink(
    std::shared_ptr<llvm::Module> module,
    std::filesystem::path outfile);

/***********************************************************************/

int main(int argc, char* argv[]) {
    po::variables_map vm {getCmdArgs(argc, argv)};
    auto infile {vm["input-file"].as<std::string>()};
    std::unique_ptr<Node> ast {getAST(infile)};

    linkSymbols(*ast);
    checkSemantics(*ast);

    auto outfile {getOutputFile(vm)};

    if (vm.count("emit-ast")) {
        printAST(*ast, outfile);
        return 0;
    }

    auto context {std::make_shared<llvm::LLVMContext>()};
    auto module {std::make_shared<llvm::Module>(infile, *context)};

    codegen(*ast, context, module);

    optimize(module, vm["optimize"].as<unsigned>());

    if (vm.count("emit-llvm")) {
        std::error_code ec;
        llvm::raw_fd_ostream output {outfile.string(), ec};
        module->print(output, /*AAW=*/nullptr);
        return 0;
    }

    return assembleLink(module, outfile);
}

/***********************************************************************/

po::variables_map getCmdArgs(int argc, char* argv[]) {
    // Define command line options
    po::options_description desc("Options");
    desc.add_options()("help,h", "Produce help message")(
        "optimize,O", po::value<unsigned>()->default_value(0),
        "Optimization level (0, 1, 2, or 3)")(
        "input-file", po::value<std::string>(), "Input file")(
        "output-file,o", po::value<std::string>(), "Output file")(
        "emit-ast", "Emit AST to the output")(
        "emit-llvm", "Emit LLVM assembly to the output");

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

    unsigned long emit_types {vm.count("emit-ast") + vm.count("emit-llvm")};
    if (emit_types > 1) {
        std::cout << "Error: can not use both --emit-ast and --emit-llvm"
                  << std::endl;
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

void checkSemantics(Node& ast) {
    SemanticVisitor semanticVisitor;
    ast.accept(semanticVisitor);
    if (!semanticVisitor.isValid()) {
        for (auto& error : semanticVisitor.errors()) {
            std::cout << error.message() << '\n';
        }
        std::cout << std::flush;
        std::exit(1);
    }
}

/***********************************************************************/

std::filesystem::path getOutputFile(po::variables_map& options) {
    if (options.count("output-file")) {
        return std::filesystem::path {options["output-file"].as<std::string>()};
    }

    std::filesystem::path old_path {options["input-file"].as<std::string>()};
    std::filesystem::path new_path {old_path.parent_path() / old_path.stem()};

    if (options["input-file"].as<std::string>() == "-") {
        new_path = std::filesystem::current_path() / "a.out";
    }

    if (options.count("emit-ast")) {
        new_path += ".ast";
    } else if (options.count("emit-llvm")) {
        new_path += ".ll";
    }

    return new_path;
}

/***********************************************************************/

void printAST(Node& ast, std::filesystem::path outfile) {
    std::ofstream output {outfile, std::ios::trunc};
    PrintVisitor printer {output};
    ast.accept(printer);
}

/***********************************************************************/

void codegen(
    Node& ast,
    std::shared_ptr<llvm::LLVMContext> context,
    std::shared_ptr<llvm::Module> module) {
    std::string target_triple {llvm::sys::getDefaultTargetTriple()};

    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();

    std::string error;
    auto target {llvm::TargetRegistry::lookupTarget(target_triple, error)};

    if (!target) {
        std::cerr << error;
        std::exit(1);
    }

    std::string cpu {llvm::sys::getHostCPUName()};
    std::string features {""};

    llvm::TargetOptions opts;
    auto target_machine {target->createTargetMachine(
        target_triple, cpu, features, opts, llvm::Reloc::PIC_)};

    module->setDataLayout(target_machine->createDataLayout());
    module->setTargetTriple(target_triple);

    CodegenVisitor codegen {context, module};
    ast.accept(codegen);
}

/***********************************************************************/

void optimize(std::shared_ptr<llvm::Module> module, unsigned opt_level) {
    LoopAnalysisManager LAM;
    FunctionAnalysisManager FAM;
    CGSCCAnalysisManager CGAM;
    ModuleAnalysisManager MAM;

    PassBuilder PB;

    PB.registerModuleAnalyses(MAM);
    PB.registerCGSCCAnalyses(CGAM);
    PB.registerFunctionAnalyses(FAM);
    PB.registerLoopAnalyses(LAM);
    PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

    OptimizationLevel level;
    switch (opt_level) {
    case 0:
        level = OptimizationLevel::O0;
        break;
    case 1:
        level = OptimizationLevel::O1;
        break;
    case 2:
        level = OptimizationLevel::O2;
        break;
    default:
        level = OptimizationLevel::O3;
        break;
    }

    ModulePassManager MPM {PB.buildPerModuleDefaultPipeline(level)};

    MPM.run(*module, MAM);
}

/***********************************************************************/

int assembleLink(
    std::shared_ptr<llvm::Module> module,
    std::filesystem::path outfile) {
    std::string temp_file {std::tmpnam(nullptr)};
    temp_file += ".ll";

    {
        std::error_code ec;
        llvm::raw_fd_ostream llvm_ir {temp_file, ec};
        module->print(llvm_ir, /*AAW=*/nullptr);
    }

    std::string command {"clang -o "};
    command += outfile.string();
    command += " ";
    command += temp_file;
    return std::system(command.c_str());
}

/***********************************************************************/
