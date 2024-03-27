CXX = clang++
CXXFLAGS = -Wall -O0 -g -std=c++23

CMinus : CMinus.cc Lexer.o AST.o Parser.o PrintVisitor.o
	$(CXX) $(CXXFLAGS) $^ -o $@

Lexer.o : Lexer.cc Lexer.hpp

Parser.o : Parser.cc Parser.hpp Lexer.hpp AST.hpp

AST.o : AST.cc AST.hpp

PrintVisitor.o : PrintVisitor.cc PrintVisitor.hpp

SymbolTable.o : SymbolTable.cc SymbolTable.hpp AST.hpp

SymbolVisitor.o : SymbolVisitor.cc SymbolVisitor.hpp AST.hpp SymbolTable.hpp

.PHONY: clean

clean:
	rm *.o CMinus

