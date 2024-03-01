#include "Lexer.hpp"

#include <cctype>
#include <fstream>
#include <string>
#include <algorithm>

Lexer::Lexer (std::ifstream&& sourceFile)
    : m_sourceFile (std::move (sourceFile))
    , m_lineNum (1)
    , m_colNum (0)
{}

int Lexer::getLineNum () const
{
    return m_lineNum;
}

int Lexer::getColumnNum () const
{
    return m_colNum;
}

Token Lexer::getToken ()
{
    eatWhitespace ();

    char c {getChar ()};
    m_colNum++;

    // Declared here as used in mutiple switch cases
    char next;

    switch (c)
    {
    /* Special Chars */
    case EOF:
        return Token (END_OF_FILE);

    /* Punctuators */
    case ';':
        return Token (SEMI, ";");
    case ',':
        return Token (COMMA, ",");
    case '(':
        return Token (LPAREN, "(");
    case ')':
        return Token (RPAREN, ")");
    case '[':
        return Token (LBRACK, "[");
    case ']':
        return Token (RBRACK, "]");
    case '{':
        return Token (LBRACE, "{");
    case '}':
        return Token (RBRACE, "}");
    
    /* Simple Operators */
    case '+':
        return nextOrElse('+', '+', INCREMENT, PLUS);
    case '-':
        return nextOrElse('-', '-', DECREMENT, MINUS);
    case '*':
        return Token (TIMES, "*");

    /* Divison and Comments */
    case '/':
       next = getChar ();
       if (next == '*') {
           m_colNum++;
           eatComment ();
           return getToken ();
       }
       m_sourceFile.putback(next);
       return Token (DIVIDE, "/");

    /* Equals and Assign */
    case '=':
       return nextOrElse('=', '=', EQ, ASSIGN);
    case '!':
       return nextOrElse('!', '=', NEQ, ERROR);

    /* Relational Operators */
    case '<':
       return nextOrElse('<', '=', LTE, LT);
    case '>':
       return nextOrElse('>', '=', GTE, GT);

    default:
       if (!is_alphanum(c)) {
           std::string lexeme {c};
           return Token (ERROR, lexeme);
       }

       // Move cursor back for literal, keyword, and id handling
       m_sourceFile.putback(c);
       m_colNum--;

       if (is_digit(c)) {
           return eatLiteral ();
       }

       return eatKeywordOrId ();
    }
}

Token Lexer::eatLiteral ()
{
    std::string lexeme {};
    char c {getChar ()};
    do {
        m_colNum++;
        lexeme += c;
        c = getChar ();
    } while (c == '_' || is_digit(c));

    if (is_alpha(c)) {
        do {
            m_colNum++;
            lexeme += c;
            c = getChar ();
        } while (is_alphanum(c));

        m_sourceFile.putback(c);

        return Token (ERROR, lexeme);
    }

    m_sourceFile.putback(c);

    // Strip out '_'s
    std::string num {lexeme};
    num.erase(std::remove(num.begin(), num.end(), '_'), num.end());
    int value = std::stoi (num);

    return Token (NUM, lexeme, value);
}

Token Lexer::eatKeywordOrId ()
{
    std::string lexeme {};
    char c {getChar ()};
    do {
        m_colNum++;
        lexeme += c;
        c = getChar ();
    } while (is_alphanum(c));

    m_sourceFile.putback(c);

    if (keywords.contains (lexeme)) {
        return Token (keywords.at (lexeme), lexeme);
    }

    return Token (ID, lexeme);
}

Token
Lexer::nextOrElse (char cur, char lookFor, TokenType found, TokenType notFound)
{
    std::string lexeme {cur};
    char next {getChar ()};
    if (next == lookFor) {
        m_colNum++;
        lexeme += lookFor;
        return Token (found, lexeme);
    }
    m_sourceFile.putback(next);
    return Token (notFound, lexeme);
}

void Lexer::eatComment ()
{
    char c {getChar ()};
    while (c != EOF) {
        char next;

        switch (c) {
        case '*':
            next = getChar ();
            if (next == '/') {
                m_colNum++;
                return;
            }
            m_sourceFile.putback(next);
            break;
        case '\n':
            m_lineNum++;
            m_colNum = 0;
            break;
        case '\r':
            break;
        case '\t':
            m_colNum += 4;
            break;
        default:
            m_colNum++;
            break;
        }
        c = getChar ();
    }

    // If EOF is hit, put it back for getToken to hit
    m_sourceFile.putback(c);
}

void Lexer::eatWhitespace ()
{
    char c {getChar ()};
    while (c == ' ' || c == '\t' || c == '\r' || c == '\n')
    {
        switch (c) {
        case '\n':
            m_lineNum++;
            m_colNum = 0;
            break;
        case '\t':
            m_colNum += 4;
            break;
        case '\r':
            break;
        case ' ':
            m_colNum++;
            break;
        }
        c = getChar ();
    }
    m_sourceFile.putback(c);
}

char Lexer::getChar ()
{
    return m_sourceFile.get();
}

bool is_alphanum(char c)
{
    return std::isalnum (static_cast<unsigned char> (c));
}

bool is_alpha(char c)
{
    return std::isalpha (static_cast<unsigned char> (c));
}

bool is_digit(char c)
{
    return std::isdigit (static_cast<unsigned char> (c));
}

