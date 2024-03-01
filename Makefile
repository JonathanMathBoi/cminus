CXX = clang++
CXXFLAGS = -Wall -O0 -g -std=c++2b

CMinus : CMinus.cc Lexer.hpp Lexer.o
	$(CXX) $(CXXFLAGS) -o CMinus CMinus.cc Lexer.o

Lexer.o : Lexer.cc Lexer.hpp

.PHONY: clean

clean:
	rm *.o CMinus

