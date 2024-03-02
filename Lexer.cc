#include "Lexer.hpp"
#include "Exception.hpp"

#include <cctype>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <algorithm>

lexer::lexer (std::ifstream&& source_file)
    : m_sourceFile {std::move (source_file)}
    , m_lineNum {1}
    , m_colNum {0}
{}

int lexer::get_line_num () const
{
    return m_lineNum;
}

int lexer::get_column_num () const
{
    return m_colNum;
}

Token lexer::get_token ()
{
    char c {get_char ()};

    switch (c)
    {
    /* Special Chars */
    case EOF:
        return Token {END_OF_FILE};

    /* Punctuators */
    case ';':
        return Token {SEMI, ";"};
    case ',':
        return Token {COMMA, ","};
    case '(':
        return Token {LPAREN, "("};
    case ')':
        return Token {RPAREN, ")"};
    case '[':
        return Token {LBRACK, "["};
    case ']':
        return Token {RBRACK, "]"};
    case '{':
        return Token {LBRACE, "{"};
    case '}':
        return Token {RBRACE, "}"};
    
    /* Simple Operators */
    case '+':
        return next_or_else('+', '+', INCREMENT, PLUS);
    case '-':
        return next_or_else('-', '-', DECREMENT, MINUS);
    case '*':
        return Token {TIMES, "*"};

    /* Divison and Comments */
    case '/':
        if (peek_char () == '*') {
            get_char ();
            eat_comment ();
            return get_token ();
        }
        return Token {DIVIDE, "/"};

    /* Equals and Assign */
    case '=':
       return next_or_else('=', '=', EQ, ASSIGN);
    case '!':
       return next_or_else('!', '=', NEQ, ERROR);

    /* Relational Operators */
    case '<':
       return next_or_else('<', '=', LTE, LT);
    case '>':
       return next_or_else('>', '=', GTE, GT);

    default:
       if (!is_alphanum(c)) {
           std::string lexeme {c};
           return Token {ERROR, lexeme};
       }

       // Move cursor back for literal, keyword, and id handling
       unget_char (c);

       if (is_digit(c)) {
           return lex_literal ();
       }

       return lex_keyword_id ();
    }
}

Token lexer::lex_literal ()
{
    std::string lexeme {};
    do {
        lexeme += get_char ();
    } while (peek_char () == '_' || is_digit (peek_char ()));

    if (is_alpha (peek_char ())) {
        do {
            lexeme += get_char ();
        } while (is_alphanum (peek_char ()));

        return Token {ERROR, lexeme};
    }

    // Strip out '_'s
    std::string num {lexeme};
    num.erase(std::remove(num.begin(), num.end(), '_'), num.end());
    int value = std::stoi (num);

    return Token {NUM, lexeme, value};
}

Token lexer::lex_keyword_id ()
{
    std::string lexeme {};
    do {
        lexeme += get_char ();
    } while (is_alphanum (peek_char ()) || peek_char () == '_');

    if (keywords.contains (lexeme)) {
        return Token {keywords.at (lexeme), lexeme};
    }

    return Token {ID, lexeme};
}

Token
lexer::next_or_else (
    char cur,
    char look_for,
    TokenType found,
    TokenType not_found
)
{
    std::string lexeme {cur};
    if (peek_char () == look_for) {
        lexeme += get_char ();
        return Token {found, lexeme};
    }
    return Token {not_found, lexeme};
}

void lexer::eat_comment ()
{
    char c {get_char ()};
    while (c != EOF) {
        if (c == '*' && peek_char () == '/') {
            get_char ();
            return;
        }

        c = get_char ();
    }

    // If EOF is hit, put it back for getToken to hit
    unget_char (c);
}

char lexer::get_char ()
{
    char c {static_cast<char> (m_sourceFile.get ())};
    switch (c) {
    case '\n':
        m_lineNum++;
        m_colNum = 0;
        return get_char ();
    case '\t':
        m_colNum += 4 - (m_colNum % 4);
        return get_char ();
    case '\r':
        return get_char ();
    case ' ':
        m_colNum++;
        return get_char ();
    default:
        m_colNum++;
        return c;
    }
}

char lexer::peek_char ()
{
    return m_sourceFile.peek ();
}

void lexer::unget_char (char c)
{
    m_sourceFile.putback (c);
    m_colNum--;
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

lexer_exception::lexer_exception(Token bad_token, int line_num, int col_num)
    : cminus_exception {line_num, col_num}, m_bad_token {bad_token}
{
    std::stringstream message_buffer;
    message_buffer << "Error while lexing\n"
        << "  Encountered: "
        << std::quoted (m_bad_token.lexeme)
        << " (line " << m_line_num << ", column " << m_col_num << ")";
    m_error_message = message_buffer.str();
}

char const* lexer_exception::what() const noexcept {
    return m_error_message.c_str();
}

