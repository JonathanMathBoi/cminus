#ifndef PARSER_HPP
#define PARSER_HPP

/***********************************************************************/

#include "AST.hpp"
#include "Lexer.hpp"
#include "MiscUtils.hpp"

#include <deque>
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

    void assignment_expr();

    void variable();

    void simple_expr();

    void relation_op();

    void add_expr();

    void add_op();

    void term();

    void mul_op();

    void factor();

    void fun_call();

    void fun_args();

    void args_list();

private:
    /**
     * Move the current token one forward and returns a reference to the new
     * current token.
     */
    Token const& get_token();

    /**
     * Peeks the token `index` after the current token.
     *
     * Note: peek_token(0) returns the current token.
     */
    Token const& peek_token(size_t index);

    /// Matches on a TokenType
    ///
    /// Checks to see if the current token matches the expected. If it matches,
    /// the token is consumed and current is moved forward. Otherwise a
    /// parse_exception is thrown.
    ///
    /// \param construct the name of the construct being parsed
    /// \param expected_token the TokenType to be matched against
    ///
    /// \returns the successfully matched Token
    ///
    /// \throws parser_exception if the current token does not match the
    ///                          expected type
    Token const match(
        const std::string_view construct,
        const TokenType expected_token);

    /**
     * Throws an error indicating the function which encountered an error and
     * what token it had expected.
     */
    void error(
        const std::string_view function,
        const std::string_view expected);

private:
    lexer m_lexer;
    Token m_current_token;
    std::deque<Token> m_peeked_tokens;
};

/***********************************************************************/

class parser_exception : public cminus_exception {
public:
    parser_exception(
        const std::string_view construct,
        Token received_token,
        const std::string_view expected);

    virtual char const* what() const noexcept;

private:
    Token m_received_token;
    std::string m_error_message;
};

/***********************************************************************/

#endif
