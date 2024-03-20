#ifndef LEXER_H
#define LEXER_H

/***********************************************************************/

#include <fstream>
#include <map>
#include <string>

#include "MiscUtils.hpp"

/***********************************************************************/

enum TokenType {
    // Bookkeeping
    END_OF_FILE,
    ERROR,

    // Keywords
    IF,
    ELSE,
    INT,
    VOID,
    RETURN,
    WHILE,

    // Operators
    PLUS,
    MINUS,
    TIMES,
    DIVIDE,
    LT,
    LTE,
    GT,
    GTE,
    EQ,
    NEQ,
    ASSIGN,
    INCREMENT,
    DECREMENT,

    // Punctuators
    SEMI,
    COMMA,
    LPAREN,
    RPAREN,
    LBRACK,
    RBRACK,
    LBRACE,
    RBRACE,

    // Identifiers and integer literals
    ID,
    NUM
};

const std::map<TokenType, std::string> token_types {
    {END_OF_FILE, "END_OF_FILE"},
    {ERROR, "ERROR"},
    {IF, "IF"},
    {ELSE, "ELSE"},
    {INT, "INT"},
    {VOID, "VOID"},
    {RETURN, "RETURN"},
    {WHILE, "WHILE"},
    {PLUS, "PLUS"},
    {MINUS, "MINUS"},
    {TIMES, "TIMES"},
    {DIVIDE, "DIVIDE"},
    {LT, "LT"},
    {LTE, "LTE"},
    {GT, "GT"},
    {GTE, "GTE"},
    {EQ, "EQ"},
    {NEQ, "NEQ"},
    {ASSIGN, "ASSIGN"},
    {INCREMENT, "INCREMENT"},
    {DECREMENT, "DECREMENT"},
    {SEMI, "SEMI"},
    {COMMA, "COMMA"},
    {LPAREN, "LPAREN"},
    {RPAREN, "RPAREN"},
    {LBRACK, "LBRACK"},
    {RBRACK, "RBRACK"},
    {LBRACE, "LBRACE"},
    {RBRACE, "RBRACE"},
    {ID, "ID"},
    {NUM, "NUM"}};

/***********************************************************************/

/// Token Struct
///
/// A lexed token from a source file.
///
/// Consists of its TokenType, its lexeme, an optional number (used for NUM
/// tokens), and its location in the code.
struct Token {
    Token(
        TokenType pType,
        std::string pLexeme = "",
        int pNumber = 0,
        location loc = location {-1, -1})
        : type {pType}, lexeme {pLexeme}, number {pNumber}, loc {loc} {}

    TokenType type;
    std::string lexeme;
    int number;
    location loc;
};

/***********************************************************************/

const std::map<std::string, TokenType> keywords {
    {"if", IF},     {"else", ELSE},     {"int", INT},
    {"void", VOID}, {"return", RETURN}, {"while", WHILE}};

/***********************************************************************/

class lexer {
public:
    lexer(std::ifstream&& source_file);

    Token get_token();

    [[deprecated("Use Token::line_num instead.")]] int get_line_num() const;

    [[deprecated("Use Token::col_num instead.")]] int get_column_num() const;

private:
    char get_char();

    char peek_char();

    /**
     * Takes a char and places it back at the front of the input stream. Also
     * accounts for adjusting column count back.
     */
    void unget_char(char c);

    /**
     * Takes the current char and checks if the next char matches a given char.
     * If the next char matches, a Token of type found is returned, else the
     * next char is put back, and a token of type notFound is returned.
     *
     * @param cur the current char
     * @param lookFor the char of interest for next
     * @param found the TokenType if this is the long token
     * @param notFound the TokenType if this is the short token
     *
     * @returns the matched token
     */
    Token
    next_or_else(char cur, char look_for, TokenType found, TokenType not_found);

    Token lex_literal();

    Token lex_keyword_id();

    void eat_comment();

    /**
     * Creates a token with the currently lexed token's line number and column
     * number.
     */
    Token make_token(TokenType type, std::string lexeme = "", int number = 0)
        const;

    // Additional helper methods
    // ...

private:
    std::ifstream m_sourceFile;
    int m_lineNum;
    int m_colNum;
    // Additional data members if necessary
    // ...
    /* The line where the currently lexed token is. */
    int m_token_line;
    /* The column where the currently lexed token starts. */
    int m_token_col;
};

/***********************************************************************/

class lexer_exception : public cminus_exception {
public:
    lexer_exception(Token bad_token);

    virtual char const* what() const noexcept;

private:
    Token m_bad_token;
    std::string m_error_message;
};

/***********************************************************************/

bool is_alphanum(char c);

bool is_alpha(char c);

bool is_digit(char c);

/***********************************************************************/

#endif
