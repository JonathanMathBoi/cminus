#include "Parser.hpp"
#include "Lexer.hpp"
#include "Exception.hpp"

#include <iomanip>
#include <sstream>
#include <string_view>

/***********************************************************************/

void parser::program() {
    decl_list();
}

void parser::decl_list() {
    do {
        declaration();
    } while(m_current_token.type != END_OF_FILE);
}

void parser::declaration() {
    switch (peek_token(2).type) {
    case SEMI:
    case LBRACK:
        var_decl();
        break;
    case LPAREN:
        fun_decl();
        break;
    default:
        type_spec();
        match("declaration", ID);
        error("declaration", "';', '[', or '('");
        break;
    }
}

void parser::var_decl() {
    type_spec();
    match("variable declaration", ID);

    if (m_current_token.type == LBRACK) {
        match("variable declaration", LBRACK);
        match("variable declaration", NUM);
        match("variable declaration", RBRACK);
    }

    match("variable declaration", SEMI);
}

void parser::type_spec() {
    switch (m_current_token.type) {
    case INT:
    case VOID:
        get_token();
        break;
    default:
        error("type specifier", "INT or VOID");
        break;
    }
}

void parser::fun_decl() {
    type_spec();
    match("function declaration", ID);
    match("function declaration", LPAREN);
    params();
    match("function declaration", RPAREN);
    compound_stmt();
}

void parser::params() {
    if (m_current_token.type == VOID && peek_token(1).type == RPAREN) {
        match("parameters", VOID);
        return;
    }

    param_list();
}

void parser::param_list() {
    param();

    while (m_current_token.type == COMMA) {
        match("parameter list", COMMA);
        param();
    }
}

void parser::param() {
    type_spec();
    match("parameter", ID);

    if (m_current_token.type == LBRACK) {
        match("parameter", LBRACK);
        match("parameter", RBRACE);
    }
}

void parser::compound_stmt() {
    match("compound statement", LBRACE);
    local_decls();
    stmt_list();
    match("compound statement", RBRACE);
}

void parser::local_decls() {
    while (m_current_token.type == VOID || m_current_token.type == INT) {
        var_decl();
    }
}

void parser::stmt_list() {
    while (m_current_token.type != RBRACE) {
        statement();
    }
}

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
    if (!m_peeked_tokens.empty()) {
        m_current_token = m_peeked_tokens.front();
        m_peeked_tokens.pop_front();
    } else {
        m_current_token = m_lexer.get_token();
    }

    return m_current_token;
}

Token const& parser::peek_token(size_t index) {
    if (index == 0) {
        return m_current_token;
    }

    index--;

    while (index >= m_peeked_tokens.size()) {
        m_peeked_tokens.push_back(m_lexer.get_token());
    }

    return m_peeked_tokens[index];
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

void parser::error(
    const std::string_view function,
    const std::string_view expected
)
{
    throw parser_exception {
        function,
        m_current_token,
        expected,
        m_lexer.get_line_num(),
        m_lexer.get_column_num()
    };
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

