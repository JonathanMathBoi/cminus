#ifndef LEXER_H
#define LEXER_H

/***********************************************************************/

#include <fstream>
#include <map>
#include <string>

/***********************************************************************/

enum TokenType
{
  // Bookkeeping
  END_OF_FILE, ERROR,

  // Keywords
  IF, ELSE, INT, VOID, RETURN, WHILE,

  // Operators
  PLUS, MINUS, TIMES, DIVIDE,
  LT, LTE, GT, GTE, EQ, NEQ,
  ASSIGN, INCREMENT, DECREMENT, 

  // Punctuators
  SEMI, COMMA, LPAREN, RPAREN, LBRACK, RBRACK, LBRACE, RBRACE,

  // Identifiers and integer literals
  ID, NUM 
};

/***********************************************************************/

struct Token
{
  Token (TokenType pType,
	 std::string pLexeme = "",
	 int pNumber = 0)
      : type (pType), lexeme (pLexeme), number (pNumber)
  {  }

  TokenType   type;
  std::string lexeme;
  int         number;
};

/***********************************************************************/

const std::map<std::string, TokenType> keywords
{
    {"if", IF},
    {"else", ELSE},
    {"int", INT},
    {"void", VOID},
    {"return", RETURN},
    {"while", WHILE}
};

/***********************************************************************/

class Lexer
{
public:
  Lexer (std::ifstream&& sourceFile);

  Token
  getToken ();

  int
  getLineNum () const;

  int
  getColumnNum () const;
  
private:
  char
  getChar ();

  /**
   * Takes the current char and checks if the next char matches a given char.
   * If the next char matches, a Token of type found is returned, else the next
   * char is put back, and a token of type notFound is returned.
   *
   * @param cur the current char
   * @param lookFor the char of interest for next
   * @param found the TokenType if this is the long token
   * @param notFound the TokenType if this is the short token
   *
   * @returns the matched token
   */
  Token
  nextOrElse (char cur, char lookFor, TokenType found, TokenType notFound);

  void
  eatWhitespace ();

  void
  eatComment ();

  Token
  eatLiteral ();

  Token
  eatKeywordOrId ();
  
  // Additional helper methods
  // ...

private:
  std::ifstream m_sourceFile;
  int   m_lineNum;
  int   m_colNum;
  // Additional data members if necessary
  // ...
};

/***********************************************************************/

bool
is_alphanum (char c);

bool
is_alpha (char c);

bool
is_digit (char c);

/***********************************************************************/

#endif
