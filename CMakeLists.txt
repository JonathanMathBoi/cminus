cmake_minimum_required(VERSION 3.28)
project(CMinus)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED COMPONENTS program_options)

add_executable(CMinus
    src/CMinus.cpp
    src/MiscUtils.hpp
    src/lexer/Lexer.cpp
    src/lexer/Lexer.hpp
    src/parser/Parser.cpp
    src/parser/Parser.hpp
    src/ast/AST.cpp
    src/ast/AST.hpp
    src/ast/PrintVisitor.cpp
    src/ast/PrintVisitor.hpp
    src/symbol/SymbolTable.cpp
    src/symbol/SymbolTable.hpp
    src/symbol/SymbolVisitor.cpp
    src/symbol/SymbolVisitor.hpp
    src/semantics/SemanticVisitor.hpp
    src/semantics/SemanticVisitor.cpp
)

target_link_libraries(CMinus PRIVATE Boost::program_options)

target_compile_options(CMinus PRIVATE -O1 -g)
