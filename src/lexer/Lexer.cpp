#include "Lexer.hpp"
#include "../MiscUtils.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>

Lexer::Lexer(std::ifstream&& source_file)
    : m_sourceFile {std::move(source_file)}, m_lineNum {1}, m_colNum {0} {}

int Lexer::get_line_num() const {
    return m_lineNum;
}

int Lexer::get_column_num() const {
    return m_colNum;
}

Token Lexer::makeToken(TokenType type, std::string lexeme, int number) const {
    return Token {
        type, lexeme, number, Location {m_tokenLineNum, m_tokenColNum}};
}

Token Lexer::getToken() {
    char c {getChar()};
    m_tokenLineNum = m_lineNum;
    m_tokenColNum = m_colNum;

    switch (c) {
    /* Special Chars */
    case EOF:
        return makeToken(END_OF_FILE);

    /* Punctuators */
    case ';':
        return makeToken(SEMI, ";");
    case ',':
        return makeToken(COMMA, ",");
    case '(':
        return makeToken(LPAREN, "(");
    case ')':
        return makeToken(RPAREN, ")");
    case '[':
        return makeToken(LBRACK, "[");
    case ']':
        return makeToken(RBRACK, "]");
    case '{':
        return makeToken(LBRACE, "{");
    case '}':
        return makeToken(RBRACE, "}");

    /* Simple Operators */
    case '+':
        return nextOrElse('+', '+', INCREMENT, PLUS);
    case '-':
        return nextOrElse('-', '-', DECREMENT, MINUS);
    case '*':
        return makeToken(TIMES, "*");

    /* Divison and Comments */
    case '/':
        if (peekChar() == '*') {
            getChar();
            eatComment();
            return getToken();
        }
        return makeToken(DIVIDE, "/");

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
            throw LexerException {makeToken(ERROR, lexeme)};
        }

        // Move cursor back for literal, keyword, and id handling
        ungetChar(c);

        if (is_digit(c)) {
            return lexLiteral();
        }

        return lexKeywordID();
    }
}

Token Lexer::lexLiteral() {
    std::string lexeme {};
    do {
        lexeme += getChar();
    } while (peekChar() == '_' || is_digit(peekChar()));

    if (is_alpha(peekChar())) {
        do {
            lexeme += getChar();
        } while (is_alphanum(peekChar()));

        throw LexerException {makeToken(ERROR, lexeme)};
    }

    // Strip out '_'s
    std::string num {lexeme};
    num.erase(std::remove(num.begin(), num.end(), '_'), num.end());
    int value = std::stoi(num);

    return makeToken(NUM, lexeme, value);
}

Token Lexer::lexKeywordID() {
    std::string lexeme {};
    do {
        lexeme += getChar();
    } while (is_alphanum(peekChar()) || peekChar() == '_');

    if (keywords.contains(lexeme)) {
        return makeToken(keywords.at(lexeme), lexeme);
    }

    return makeToken(ID, lexeme);
}

Token Lexer::nextOrElse(
    char cur,
    char look_for,
    TokenType found,
    TokenType not_found) {
    std::string lexeme {cur};
    if (peekChar() == look_for) {
        lexeme += getChar();
        return makeToken(found, lexeme);
    }
    return makeToken(not_found, lexeme);
}

void Lexer::eatComment() {
    char c {getChar()};
    while (c != EOF) {
        if (c == '*' && peekChar() == '/') {
            getChar();
            return;
        }

        c = getChar();
    }

    // If EOF is hit, put it back for getToken to hit
    ungetChar(c);
}

char Lexer::getChar() {
    char c {static_cast<char>(m_sourceFile.get())};
    switch (c) {
    case '\n':
        m_lineNum++;
        m_colNum = 0;
        return getChar();
    case '\t':
        m_colNum += 4 - (m_colNum % 4);
        return getChar();
    case '\r':
        return getChar();
    case ' ':
        m_colNum++;
        return getChar();
    default:
        m_colNum++;
        return c;
    }
}

char Lexer::peekChar() {
    return m_sourceFile.peek();
}

void Lexer::ungetChar(char c) {
    m_sourceFile.putback(c);
    m_colNum--;
}

bool is_alphanum(char c) {
    return std::isalnum(static_cast<unsigned char>(c));
}

bool is_alpha(char c) {
    return std::isalpha(static_cast<unsigned char>(c));
}

bool is_digit(char c) {
    return std::isdigit(static_cast<unsigned char>(c));
}

LexerException::LexerException(Token bad_token)
    : CMinusException {bad_token.loc}, m_badToken {bad_token} {
    std::stringstream message_buffer;
    message_buffer << "Error while lexing\n"
                   << "  Encountered: " << std::quoted(m_badToken.lexeme)
                   << " (line " << location.line_num << ", column "
                   << location.col_num << ")";
    m_errorMessage = message_buffer.str();
}

char const* LexerException::what() const noexcept {
    return m_errorMessage.c_str();
}
