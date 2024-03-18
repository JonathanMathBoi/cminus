#ifndef PARSER_HPP
#define PARSER_HPP

/***********************************************************************/

#include "AST.hpp"
#include "Lexer.hpp"
#include "MiscUtils.hpp"

#include <deque>
#include <memory>
#include <string_view>
#include <vector>

using std::shared_ptr;
using std::unique_ptr;
using std::vector;

/***********************************************************************/

class parser_exception;

/***********************************************************************/

class parser {
public:
    parser(lexer&& lexer);

    unique_ptr<node> parse();

private:
    unique_ptr<program_node> program();
    vector<shared_ptr<declaration_node>> decl_list();
    unique_ptr<declaration_node> declaration();
    /// Parses a type specifier
    ///
    /// \returns the type specified and its location in the code
    std::pair<basic_type, location> type_spec();
    unique_ptr<variable_declaration_node> var_decl();
    unique_ptr<function_declaration_node> fun_decl();
    vector<shared_ptr<param_node>> params();
    vector<shared_ptr<param_node>> param_list();
    unique_ptr<param_node> param();
    unique_ptr<compound_statement_node> compound_stmt();
    vector<shared_ptr<variable_declaration_node>> local_decls();
    vector<unique_ptr<statement_node>> stmt_list();
    unique_ptr<statement_node> statement();
    unique_ptr<expression_statement_node> expr_stmt();
    unique_ptr<if_statement_node> if_statement();
    unique_ptr<while_statement_node> while_statement();
    unique_ptr<return_statement_node> return_stmt();
    unique_ptr<expression_node> expression();
    unique_ptr<assignment_expression_node> assignment_expr();
    unique_ptr<variable_expression_node> variable();
    unique_ptr<expression_node> relational_expr();
    rel_op relation_op();
    unique_ptr<expression_node> add_expr();
    add_op additive_op();

    unique_ptr<expression_node> term();

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
    parser_exception error(
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
