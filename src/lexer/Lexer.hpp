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

/// \class Token
/// \brief A lexed token form a source file
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
    /// \brief Gets the integer value of the token
    /// \throws BadValueAccess if the token is not an int literal
    int intValue() const;
    /// \brief Gets the float value of the token
    /// \throws BadValueAccess if the token is not a float literal
    float floatValue() const;

private:
    /// The type of the token
    TokenType m_type;
    /// The lexeme of the token
    std::string m_lexeme;
    /// The location of the token in source code
    Location m_location;
    /// The value of a literal token
    union {
        int intLiteral;
        float floatLiteral;
    } m_value;
};

/// \class BadTokenValueAccess
class BadTokenValueAccess : public std::exception {};

/***********************************************************************/

/// \class Lexer
/// \brief The lexer for C-
///
/// The lexer takes a source file and reads through it, generating
/// [Tokens](#Token) as getToken() is called.
class Lexer {
public:
    /// \brief Constructs a Lexer from a source file
    ///
    /// \param source_file the source file to be lexed
    Lexer(std::ifstream source_file);

    /// Gets the next token from the source file
    Token getToken();
    /// \brief Gets the current line number of the lexer
    ///
    /// \returns the current line number of the lexer
    ///
    /// \deprecated This function has been deprecated as it gives the line
    /// number at the end of the most recently lexed token. Please use
    /// Token::location() instead.
    [[deprecated("Use Token::location() instead.")]] int get_line_num() const;
    /// \brief Gets the current column number of the lexer
    ///
    /// \returns the current column number of the lexer
    ///
    /// \deprecated This function has been deprecated as it gives the column
    /// number at the end of the most recently lexed token. Please use
    /// Token::location() instead.
    [[deprecated("Use Token::location() instead.")]] int get_column_num() const;

private:
    /// \brief Gets the next char from the source file
    ///
    /// \returns the next char in the source file
    char getChar();
    /// \brief Peeks one char ahead in the source file
    ///
    /// \returns the char after the current char in the source file
    char peekChar();
    /// \brief Returns a char to the source file
    ///
    /// Takes a char and places it back at the front of the input stream. Also
    /// accounts for adjusting column count back.
    ///
    /// \param c the char to unget
    void ungetChar(char c);
    /// \brief Generate conditional Token based on next char
    ///
    /// Takes the current char and checks if the next char matches a given char.
    /// If the next char matches \p look_for, a Token of type \p found is
    /// returned, else the next char is put back, and a token of type \p
    /// notFound is returned.
    ///
    /// \param cur the current char
    /// \param lookFor the char to match against
    /// \param found the TokenType to be used if the match is successful
    /// \param notFound the TokenType to be used if the match is not successful
    ///
    /// \returns the matched Token
    Token
    nextOrElse(char cur, char look_for, TokenType found, TokenType not_found);
    /// \brief Lexes a type literal token
    ///
    /// Lexes a type literal token. Currently either an int literal, or a float
    /// literal.
    ///
    /// \returns the lexed literal Token
    Token lexLiteral();
    /// \brief Lexed a keyword or identifier token
    ///
    /// \returns the lexed Token
    Token lexKeywordID();
    /// Eats a comment
    void eatComment();
    /// \brief Creates a new Token
    ///
    /// Creates a token with the current token location of the lexer
    ///
    /// \param type the type of the new token
    /// \param lexeme the lexeme of the new token
    Token makeToken(TokenType type, std::string lexeme = "") const;
    /// \brief Creates a new int literal Token
    ///
    /// \param value the value of the integer literal
    /// \param lexeme the lexeme of the literal
    Token makeToken(int value, std::string lexeme) const;
    /// \brief Creates a new float literal Token
    ///
    /// \param value the value of the float literal
    /// \param lexeme the lexeme of the float literal
    Token makeToken(float value, std::string lexeme) const;

private:
    /// The source file to lex from
    std::ifstream m_sourceFile;
    /// The current line number of the lexer
    int m_lineNum;
    /// The current column number of the lexer
    int m_colNum;
    /// The line number of the start of the current token
    int m_tokenLineNum;
    /// The column number of the start of the current token
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
