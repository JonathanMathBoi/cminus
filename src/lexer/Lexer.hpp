#ifndef LEXER_H
#define LEXER_H

/***********************************************************************/

#include <exception>
#include <fstream>
#include <map>
#include <string>
#include <string_view>

#include "../MiscUtils.hpp"

/***********************************************************************/

enum TokenType {
    // Bookkeeping
    END_OF_FILE,
    ERROR,

    // Keywords
    IF,
    ELSE,
    INT,
    FLOAT,
    BOOL,
    VOID,
    RETURN,
    WHILE,
    TRUE,
    FALSE,

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
    INT_LITERAL,
    FLOAT_LITERAL
};

const std::map<TokenType, std::string> token_types {
    {END_OF_FILE, "END_OF_FILE"},
    {ERROR, "ERROR"},
    {IF, "IF"},
    {ELSE, "ELSE"},
    {INT, "INT"},
    {FLOAT, "FLOAT"},
    {BOOL, "BOOL"},
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
    {INT_LITERAL, "INT_LITERAL"},
    {FLOAT_LITERAL, "FLOAT_LITERAL"}};

/***********************************************************************/

/// Token Class
///
/// A lexed token form a source file.
class Token {
public:
    /// Constructs a general Token
    ///
    /// \param type the type of the token
    /// \param lexeme the lexeme of the token
    /// \param loc the location of the token in source code
    Token(TokenType type, std::string lexeme, Location loc);
    /// Constructs an int literal Token
    ///
    /// \param value the int value of the token
    /// \param lexeme the lexeme of the token
    /// \param loc the location of the token in source code
    Token(int value, std::string lexeme, Location loc);
    /// Constructs a float literal Token
    ///
    /// \param value the float value of the token
    /// \param lexeme the lexeme of the token
    /// \param loc the location of the token in source code
    Token(float value, std::string lexeme, Location loc);

public:
    /// Gets the type of the token
    TokenType type() const;
    /// Gets the lexeme of the token
    const std::string_view lexeme() const;
    /// Gets the location of the token
    Location location() const;
    /// Gets the integer value of the token
    ///
    /// \throws BadValueAccess if the token is not an int literal
    int intValue() const;
    /// Gets the float value of the token
    ///
    /// \throws BadValueAccess if the token is not a float literal
    float floatValue() const;

private:
    TokenType m_type;
    std::string m_lexeme;
    Location m_location;
    union {
        int intLiteral;
        float floatLiteral;
    } m_value;
};

class BadTokenValueAccess : public std::exception {};

/***********************************************************************/

class Lexer {
public:
    Lexer(std::ifstream&& source_file);

    Token getToken();

    [[deprecated("Use Token::line_num instead.")]] int get_line_num() const;

    [[deprecated("Use Token::col_num instead.")]] int get_column_num() const;

private:
    char getChar();

    char peekChar();

    /**
     * Takes a char and places it back at the front of the input stream. Also
     * accounts for adjusting column count back.
     */
    void ungetChar(char c);

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
    nextOrElse(char cur, char look_for, TokenType found, TokenType not_found);
    Token lexLiteral();
    Token lexKeywordID();

    void eatComment();
    /// Creates a new Token
    ///
    /// Creates a token with the current token location of the lexer
    ///
    /// \param type the type of the new token
    /// \param lexeme the lexeme of the new token
    Token makeToken(TokenType type, std::string lexeme = "") const;
    /// Creates a new int literal Token
    ///
    /// \param value the value of the integer literal
    /// \param lexeme the lexeme of the literal
    Token makeToken(int value, std::string lexeme) const;
    /// Creates a new float literal Token
    ///
    /// \param value the value of the float literal
    /// \param lexeme the lexeme of the float literal
    Token makeToken(float value, std::string lexeme) const;

private:
    std::ifstream m_sourceFile;
    int m_lineNum;
    int m_colNum;
    // Additional data members if necessary
    // ...
    /* The line where the currently lexed token is. */
    int m_tokenLineNum;
    /* The column where the currently lexed token starts. */
    int m_tokenColNum;
};

/***********************************************************************/

class LexerException : public CMinusException {
public:
    LexerException(Token bad_token);

    virtual char const* what() const noexcept;

private:
    Token m_badToken;
    std::string m_errorMessage;
};

/***********************************************************************/

bool is_alphanum(char c);

bool is_alpha(char c);

bool is_digit(char c);

/***********************************************************************/

#endif
