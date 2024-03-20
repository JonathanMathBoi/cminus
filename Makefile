CXX = clang++
CXXFLAGS = -Wall -O0 -g -std=c++23

CMinus : CMinus.cc Lexer.hpp Lexer.o AST.hpp AST.o Parser.hpp Parser.o
	$(CXX) $(CXXFLAGS) -o CMinus CMinus.cc Lexer.o Parser.o AST.o

Lexer.o : Lexer.cc Lexer.hpp

Parser.o : Parser.cc Parser.hpp Lexer.hpp AST.hpp

AST.o : AST.cc AST.hpp

.PHONY: clean

clean:
	rm *.o CMinus

