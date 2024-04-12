#ifndef PARSER_HPP
#define PARSER_HPP

/***********************************************************************/

#include "../MiscUtils.hpp"
#include "../ast/AST.hpp"
#include "../lexer/Lexer.hpp"

#include <deque>
#include <memory>
#include <string_view>
#include <vector>

/***********************************************************************/

class ParserException;

/***********************************************************************/

class Parser {
public:
    Parser(Lexer&& lexer);

    std::unique_ptr<Node> parse();

private:
    std::unique_ptr<ProgramNode> program();
    std::vector<std::shared_ptr<Declaration>> declarationList();
    std::unique_ptr<Declaration> declaration();
    /// Parses a type specifier
    ///
    /// \returns the type specified and its location in the code
    std::pair<Type, Location> typeSpec();
    /// \brief Parses a variable declaration
    ///
    /// \returns a declaration with Variable kind
    std::unique_ptr<Declaration> variableDeclaration();
    /// \brief Parses a function declaration
    ///
    /// \returns a declaration with Function kind
    std::unique_ptr<Declaration> functionDeclaration();
    /// \brief Parses parameters to a function
    ///
    /// \returns a vector of parameter declarations
    std::vector<std::shared_ptr<Declaration>> functionParameters();
    /// \brief Parses a list of parameters
    ///
    /// \returns a vector of parameter declarations
    std::vector<std::shared_ptr<Declaration>> parameterList();
    /// \brief Parses a function parameter
    ///
    /// \returns a declaration with Parameter kind
    std::unique_ptr<Declaration> parameter();
    std::unique_ptr<CompoundStatementNode> compoundStatement();
    /// \brief Parses a list of variable declarations
    ///
    /// \returns a vector of declarations with Variable or Array kind
    std::vector<std::shared_ptr<Declaration>> localDeclarations();
    std::vector<std::unique_ptr<StatementNode>> statementList();
    std::unique_ptr<StatementNode> statement();
    std::unique_ptr<ExpressionStatementNode> expressionStatement();
    std::unique_ptr<IfStatementNode> ifStatement();
    std::unique_ptr<WhileStatementNode> whileStatement();
    std::unique_ptr<ReturnStatementNode> returnStatement();
    std::unique_ptr<ExpressionNode> expression();
    std::unique_ptr<AssignmentExpressionNode> assignmentExpression();
    std::unique_ptr<VariableExpressionNode> variableExpression();
    std::unique_ptr<ExpressionNode> relationalExpression();
    RelationalOp relationOperation();
    std::unique_ptr<ExpressionNode> additiveExpression();
    AdditiveOp additiveOperation();
    std::unique_ptr<ExpressionNode> term();
    MultiplicativeOp multiplicativeOperation();
    std::unique_ptr<ExpressionNode> factor();
    std::unique_ptr<CallExpressionNode> functionCall();
    std::vector<std::unique_ptr<ExpressionNode>> functionArguments();
    std::vector<std::unique_ptr<ExpressionNode>> argumentList();

private:
    /**
     * Move the current token one forward and returns a reference to the new
     * current token.
     */
    Token const& getToken();

    /**
     * Peeks the token `index` after the current token.
     *
     * Note: peek_token(0) returns the current token.
     */
    Token const& peekToken(size_t index);

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
    ParserException error(
        const std::string_view function,
        const std::string_view expected);

private:
    Lexer m_lexer;
    Token m_currentToken;
    std::deque<Token> m_peekedTokens;
};

/***********************************************************************/

class ParserException : public CMinusException {
public:
    ParserException(
        const std::string_view construct,
        Token received_token,
        const std::string_view expected);

    virtual char const* what() const noexcept;

private:
    Token m_receivedToken;
    std::string m_errorMessage;
};

/***********************************************************************/

#endif
