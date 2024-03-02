#ifndef PARSER_HPP
#define PARSER_HPP

/***********************************************************************/

#include "Lexer.hpp"

#include <optional>
#include <string_view>

/***********************************************************************/

class parser {
public:
    parser(lexer&& lexer);

    void parse();

private:
    void program();

    void decl_list();

    void declaration();

    void var_decl();

    void type_spec();

    void fun_decl();

    void params();

    void param_list();

    void param();

    void compound_stmt();

    void local_decls();

    void stmt_list();

    void statement();

    void expr_stmt();

    void if_statement();

    void while_statement();

    void return_stmt();

    void expression();

    void variable();

    void simple_expr();

    void relation_op();

    void add_expr();

    void add_op();

    void term();

    void mul_op();

    void factor();

private:
    Token& get_token();

    Token& peek_token();

    void match(const std::string_view function, const TokenType expected_token);

private:
    lexer m_lexer;
    Token m_current_token;
    std::optional<Token> m_peeked_token;
};

/***********************************************************************/

#endif
