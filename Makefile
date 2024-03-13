CXX = clang++
CXXFLAGS = -Wall -O0 -g -std=c++2b

CMinus : CMinus.cc Lexer.hpp Lexer.o Parser.hpp Parser.o
	$(CXX) $(CXXFLAGS) -o CMinus CMinus.cc Lexer.o Parser.o

Lexer.o : Lexer.cc Lexer.hpp

Parser.o : Parser.cc Parser.hpp Lexer.hpp

.PHONY: clean

clean:
	rm *.o CMinus

