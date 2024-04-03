cmake_minimum_required(VERSION 3.28)
project(CMinus)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

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

target_compile_options(CMinus PRIVATE -O1 -g)

llvm_config(CMinus USE_SHARED support core irreader)

target_link_libraries(CMinus ${llvm_libs})

