#include "Parser.hpp"
#include "Lexer.hpp"
#include "Exception.hpp"

#include <iomanip>
#include <sstream>
#include <string_view>

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

