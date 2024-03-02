#include "Parser.hpp"
#include "Lexer.hpp"
#include "Exception.hpp"

#include <iomanip>
#include <sstream>
#include <string_view>

/***********************************************************************/

parser::parser(lexer&& lexer)
    : m_lexer {std::move (lexer)}, m_current_token {Token {END_OF_FILE}}
{}

void parser::parse() {
    // Pull first token from lexer to start with good state
    get_token();

    program();
}

Token const& parser::get_token() {
    if (m_peeked_token) {
        m_current_token = m_peeked_token.value();
        m_peeked_token.reset();
    } else {
        m_peeked_token = m_lexer.get_token();
    }

    return m_current_token;
}

Token const& parser::peek_token() {
    if (!m_peeked_token) {
        m_peeked_token = m_lexer.get_token();
    }

    return m_peeked_token.value();
}

void parser::match(
    const std::string_view function,
    const TokenType expected_token
)
{
    if (m_current_token.type == expected_token) {
        get_token();
    } else {
        throw parser_exception {
            function,
            m_current_token,
            // Todo: Quote this somehow
            token_types.at(m_current_token.type),
            m_lexer.get_line_num(),
            m_lexer.get_column_num()
        };
    }
}

/***********************************************************************/

parser_exception::parser_exception(
    const std::string_view construct,
    Token received_token,
    const std::string_view expected,
    int line_num,
    int col_num
)
    : cminus_exception {line_num, col_num}, m_received_token {received_token}
{
    std::stringstream message_buffer;
    message_buffer << "Error while parsing " << std::quoted(construct) << '\n'
        << "  Encountered: " << std::quoted(m_received_token.lexeme)
        << " (line " << m_line_num << ", column " << m_col_num << ")\n"
        << "  Expected   : " << expected;
    m_error_message = message_buffer.str();
}

char const* parser_exception::what() const noexcept {
    return m_error_message.c_str();
}

/***********************************************************************/

