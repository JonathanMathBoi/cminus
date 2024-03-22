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

/***********************************************************************/

class parser_exception;

/***********************************************************************/

class parser {
public:
    parser(lexer&& lexer);

    std::unique_ptr<Node> parse();

private:
    std::unique_ptr<ProgramNode> program();
    std::vector<std::shared_ptr<declaration_node>> decl_list();
    std::unique_ptr<declaration_node> declaration();
    /// Parses a type specifier
    ///
    /// \returns the type specified and its location in the code
    std::pair<basic_type, location> type_spec();
    std::unique_ptr<variable_declaration_node> var_decl();
    std::unique_ptr<function_declaration_node> fun_decl();
    std::vector<std::shared_ptr<param_node>> params();
    std::vector<std::shared_ptr<param_node>> param_list();
    std::unique_ptr<param_node> param();
    std::unique_ptr<compound_statement_node> compound_stmt();
    std::vector<std::shared_ptr<variable_declaration_node>> local_decls();
    std::vector<std::unique_ptr<statement_node>> stmt_list();
    std::unique_ptr<statement_node> statement();
    std::unique_ptr<expression_statement_node> expr_stmt();
    std::unique_ptr<if_statement_node> if_statement();
    std::unique_ptr<while_statement_node> while_statement();
    std::unique_ptr<return_statement_node> return_stmt();
    std::unique_ptr<expression_node> expression();
    std::unique_ptr<assignment_expression_node> assignment_expr();
    std::unique_ptr<variable_expression_node> variable();
    std::unique_ptr<expression_node> relational_expr();
    rel_op relation_op();
    std::unique_ptr<expression_node> add_expr();
    add_op additive_op();
    std::unique_ptr<expression_node> term();
    mul_op mult_op();
    std::unique_ptr<expression_node> factor();
    std::unique_ptr<call_expression_node> fun_call();
    std::vector<std::unique_ptr<expression_node>> fun_args();
    std::vector<std::unique_ptr<expression_node>> args_list();

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
