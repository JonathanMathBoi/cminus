#include "Parser.hpp"
#include "AST.hpp"
#include "Lexer.hpp"
#include "MiscUtils.hpp"

#include <cstddef>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string_view>
#include <utility>
#include <vector>

using std::make_unique;
using std::shared_ptr;
using std::unique_ptr;
using std::vector;

/***********************************************************************/

/**
 * Parses program -> decleration-list
 */
unique_ptr<ProgramNode> Parser::program() {
    return make_unique<ProgramNode>(declarationList());
}

/**
 * Parses declaration-list -> declaration-list declaration | declaration
 *
 * Implemented as declaration-list -> declaration { declaration }
 */
vector<shared_ptr<DeclarationNode>> Parser::declarationList() {
    vector<shared_ptr<DeclarationNode>> decls;

    do {
        decls.emplace_back(declaration());
    } while (m_currentToken.type != END_OF_FILE);

    return decls;
}

/**
 * Parses declaration -> var-declaration | fun-declaration
 */
unique_ptr<DeclarationNode> Parser::declaration() {
    switch (peekToken(2).type) {
    case SEMI:
    case LBRACK:
        return variableDeclaration();
    case LPAREN:
        return functionDeclaration();
    default:
        typeSpec();
        match("declaration", ID);
        throw error("declaration", "';', '[', or '('");
    }
}

/**
 * Parses var-declaration -> type-specifier ID SEMI
 *                         | type-specifier ID LBRACK NUM RBRACK SEMI
 *
 * Implemented as var-declaration
 *                  -> type-specifier ID [ LBRACK NUM RBRACK ] SEMI
 */
unique_ptr<VariableDeclarationNode> Parser::variableDeclaration() {
    auto spec {typeSpec()};
    ValueType type {spec.first};
    Location loc {spec.second};

    std::string id {match("variable declaration", ID).lexeme};

    unique_ptr<VariableDeclarationNode> new_node;

    if (m_currentToken.type == LBRACK) {
        type.is_array = true;

        match("variable declaration", LBRACK);
        int size {match("variable declaration", NUM).number};
        match("variable declaration", RBRACK);

        new_node = make_unique<ArrayDeclarationNode>(type, id, size, loc);
    } else {
        new_node = make_unique<VariableDeclarationNode>(type, id, loc);
    }

    match("variable declaration", SEMI);

    return new_node;
}

/**
 * Parses type-specifier -> INT | VOID
 */
std::pair<TypeSpecifier, Location> Parser::typeSpec() {
    static const std::map<TokenType, TypeSpecifier> types {
        {VOID, TypeSpecifier::VOID}, {INT, TypeSpecifier::INT}};

    if (types.contains(m_currentToken.type)) {
        std::pair<TypeSpecifier, Location> ret {
            types.at(m_currentToken.type), m_currentToken.loc};
        getToken();
        return ret;
    }

    throw error("type specifier", "INT or VOID");
}

/**
 * Parses fun-declaration
 *          -> type-specifier ID LPAREN params RPAREN compound-stmt
 */
unique_ptr<FunctionDeclarationNode> Parser::functionDeclaration() {
    auto spec {typeSpec()};
    ValueType type {spec.first};
    Location loc {spec.second};

    std::string id {match("function declaration", ID).lexeme};

    match("function declaration", LPAREN);

    auto parameters {functionParameters()};

    match("function declaration", RPAREN);

    auto body {compoundStatement()};
    body->is_function_body = true;

    return make_unique<FunctionDeclarationNode>(
        type, id, parameters, std::move(body), loc);
}

/**
 * Parses params -> param-list | VOID
 */
vector<shared_ptr<ParameterNode>> Parser::functionParameters() {
    if (m_currentToken.type == VOID && peekToken(1).type == RPAREN) {
        match("parameters", VOID);
        // returns an empty vector
        return {};
    }

    return parameterList();
}

/**
 * Parses param-list -> param-list COMMA param | param
 *
 * Implemented as param-list -> param { COMMA param }
 */
vector<shared_ptr<ParameterNode>> Parser::parameterList() {
    vector<shared_ptr<ParameterNode>> params;

    params.emplace_back(parameter());

    while (m_currentToken.type == COMMA) {
        match("parameter list", COMMA);
        params.emplace_back(parameter());
    }

    return params;
}

/**
 * Parses param -> type-specifier ID | type-specifier ID LBRACK RBRACK
 *
 * Implemented as param -> type-specifier ID [ LBRACK RBRACK ]
 */
unique_ptr<ParameterNode> Parser::parameter() {
    auto spec {typeSpec()};
    ValueType type {spec.first};
    Location loc {spec.second};

    std::string id {match("parameter", ID).lexeme};

    if (m_currentToken.type == LBRACK) {
        match("parameter", LBRACK);
        match("parameter", RBRACK);
        type.is_array = true;
    }

    return make_unique<ParameterNode>(type, id, loc);
}

/**
 * Parses compound-stmt -> LBRACE local-delarations statement-list RBRACE
 */
unique_ptr<CompoundStatementNode> Parser::compoundStatement() {
    Location loc {match("compound statement", LBRACE).loc};
    auto locals {localDeclarations()};
    auto statements {statementList()};
    match("compound statement", RBRACE);
    return make_unique<CompoundStatementNode>(
        locals, std::move(statements), loc);
}

/**
 * Parses local-declarations -> local-delarations var-delcaration
 *                            | empty
 *
 * Implemented as local-declarations -> { var-declaration }
 */
vector<shared_ptr<VariableDeclarationNode>> Parser::localDeclarations() {
    vector<shared_ptr<VariableDeclarationNode>> decls;

    while (m_currentToken.type == VOID || m_currentToken.type == INT) {
        decls.emplace_back(variableDeclaration());
    }

    return decls;
}

/**
 * Parses statement-list -> statement-list statement | empty
 *
 * Implemented as statement-list -> { statement }
 */
vector<unique_ptr<StatementNode>> Parser::statementList() {
    vector<unique_ptr<StatementNode>> stmts;

    while (m_currentToken.type != RBRACE) {
        stmts.emplace_back(statement());
    }

    return stmts;
}

/**
 * Parses statement -> expression-stmt
 *                   | compound-stmt
 *                   | selection-stmt
 *                   | iteration-stmt
 *                   | return-stmt
 */
unique_ptr<StatementNode> Parser::statement() {
    switch (m_currentToken.type) {
    case IF:
        return ifStatement();
    case WHILE:
        return whileStatement();
    case RETURN:
        return returnStatement();
    case LBRACE:
        return compoundStatement();
    default:
        return expressionStatement();
    }
}

/**
 * Parses expression-stmt -> expression SEMI | SEMI
 *
 * Implemented as expression-stmt -> [ expression ] SEMI
 */
unique_ptr<ExpressionStatementNode> Parser::expressionStatement() {
    if (m_currentToken.type == SEMI) {
        Location loc {match("expression statement", SEMI).loc};
        return make_unique<ExpressionStatementNode>(loc);
    }

    auto expr {expression()};
    Location loc {expr->loc};
    match("expression statement", SEMI);

    return make_unique<ExpressionStatementNode>(std::move(expr), loc);
}

/**
 * Parses selection-stmt -> IF LPAREN expression RPAREN statement
 *                        | IF LPAREN expression RPAREN statement ELSE statement
 *
 * Implemented as selection-stmt
 *                  -> IF LPAREN expression RPAREN [ ELSE statement]
 */
unique_ptr<IfStatementNode> Parser::ifStatement() {
    Location loc {match("if statement", IF).loc};
    match("if statement", LPAREN);
    auto condition {expression()};
    match("if statement", RPAREN);
    auto then_stmt {statement()};

    if (m_currentToken.type == ELSE) {
        match("if statement", ELSE);
        auto else_stmt {statement()};
        return make_unique<IfStatementNode>(
            std::move(condition), std::move(then_stmt), std::move(else_stmt),
            loc);
    }

    return make_unique<IfStatementNode>(
        std::move(condition), std::move(then_stmt), loc);
}

/**
 * Parses iteration-stmt -> WHILE LPAREN expression RPAREN statement
 */
unique_ptr<WhileStatementNode> Parser::whileStatement() {
    Location loc {match("while statement", WHILE).loc};
    match("while statement", LPAREN);
    auto condition {expression()};
    match("while statement", RPAREN);
    auto body {statement()};

    return make_unique<WhileStatementNode>(
        std::move(condition), std::move(body), loc);
}

/**
 * Parses return-stmt -> RETURN SEMI | RETURN expression SEMI
 *
 * Implemented as return-stmt -> RETURN [ expression ] SEMI
 */
unique_ptr<ReturnStatementNode> Parser::returnStatement() {
    Location loc {match("return statement", RETURN).loc};

    if (m_currentToken.type == SEMI) {
        match("return statement", SEMI);
        return make_unique<ReturnStatementNode>(loc);
    }

    auto expr {expression()};
    match("return expression", SEMI);

    return make_unique<ReturnStatementNode>(std::move(expr), loc);
}

/**
 * Parses expression -> var ASSIGN expression | simple_expression
 *
 * Implemented as expression -> assign-expression | simple-expression
 *
 * Ad-hoc solution used to check for assignment expression with an indexed array
 * as the variable.
 */
unique_ptr<ExpressionNode> Parser::expression() {
    if (m_currentToken.type == ID && peekToken(1).type == ASSIGN) {
        return assignmentExpression();
    }

    if (m_currentToken.type == ID && peekToken(1).type == LBRACK) {
        size_t peek_idx {1};
        unsigned nest_level {0};
        do {
            switch (peekToken(peek_idx).type) {
            case LBRACK:
                nest_level++;
                break;
            case RBRACK:
                nest_level--;
                break;
            case END_OF_FILE:
                // Corner-case: ID LBRACK EOF
                // If ID LBRACK EOF is hit, call variable which will recognize
                // the bad variable
                variableExpression();
                break;
            default:
                break;
            }
            peek_idx++;
        } while (nest_level != 0);

        if (peekToken(peek_idx).type == ASSIGN) {
            return assignmentExpression();
        }
    }

    return relationalExpression();
}

/**
 * Parses assign-expression -> var ASSIGN expression
 */
unique_ptr<AssignmentExpressionNode> Parser::assignmentExpression() {
    auto var {variableExpression()};
    match("assignment expression", ASSIGN);
    auto expr {expression()};

    Location loc {var->loc};

    return make_unique<AssignmentExpressionNode>(
        std::move(var), std::move(expr), loc);
}

/**
 * Parses var -> ID | ID LBRACK expression RBRACK
 *
 * Implemented as var -> ID [ LBARCK expression RBRACK ]
 */
unique_ptr<VariableExpressionNode> Parser::variableExpression() {
    auto [_, id, __, loc] {match("variable", ID)};

    if (m_currentToken.type == LBRACK) {
        match("variable", LBRACK);
        auto subscript {expression()};
        match("variable", RBRACK);

        return make_unique<SubscriptExpressionNode>(
            id, std::move(subscript), loc);
    }

    return make_unique<VariableExpressionNode>(id, loc);
}

/**
 * Parses simple-expression -> additive-expression relop additive-expression
 *                           | additive-expression
 *
 * Implemented as simple-expression
 *                  -> additive-expression [ relop additive-expression ]
 */
unique_ptr<ExpressionNode> Parser::relationalExpression() {
    auto lhs {additiveExpression()};
    Location loc {lhs->loc};

    switch (m_currentToken.type) {
    case LT:
    case LTE:
    case GT:
    case GTE:
    case EQ:
    case NEQ: {
        RelationalOp operation {relationOperation()};
        auto rhs {additiveExpression()};
        return make_unique<RelationalExpressionNode>(
            operation, std::move(lhs), std::move(rhs), loc);
    }
    default:
        break;
    }

    return lhs;
}

/**
 * Parses relop -> LTE | LT | GT | GTE | EQ | NEQ
 */
RelationalOp Parser::relationOperation() {
    static const std::map<TokenType, RelationalOp> rel_ops {
        {LT, RelationalOp::LT}, {LTE, RelationalOp::LTE},
        {GT, RelationalOp::GT}, {GTE, RelationalOp::GTE},
        {EQ, RelationalOp::EQ}, {NEQ, RelationalOp::NEQ}};

    if (rel_ops.contains(m_currentToken.type)) {
        RelationalOp operation {rel_ops.at(m_currentToken.type)};
        getToken();
        return operation;
    }

    throw error("relational operator", "LT, LTE, GT, GTE, EQ, or NEQ");
}

const std::map<TokenType, AdditiveOp> add_ops {
    {PLUS, AdditiveOp::PLUS},
    {MINUS, AdditiveOp::MINUS}};

/**
 * Parses additive-expression -> additive-expression addop term | term
 *
 * Implemented as additive-expression -> term { addop term }
 */
unique_ptr<ExpressionNode> Parser::additiveExpression() {
    auto root {term()};
    Location loc {root->loc};

    while (add_ops.contains(m_currentToken.type)) {
        auto operation {additiveOperation()};
        auto rhs {term()};
        root = make_unique<AdditiveExpressionNode>(
            operation, std::move(root), std::move(rhs), loc);
    }

    return root;
}

/**
 * Parses addop -> PLUS | MINUS
 */
AdditiveOp Parser::additiveOperation() {
    if (add_ops.contains(m_currentToken.type)) {
        AdditiveOp operation {add_ops.at(m_currentToken.type)};
        getToken();
        return operation;
    }

    throw error("addition operator", "PLUS or MINUS");
}

const std::map<TokenType, MultiplicativeOp> mul_ops {
    {TIMES, MultiplicativeOp::TIMES},
    {DIVIDE, MultiplicativeOp::DIVIDE}};

/**
 * Parses term -> term mulop factor | factor
 *
 * Implemented as term -> factor { mulop factor }
 */
unique_ptr<ExpressionNode> Parser::term() {
    auto root {factor()};
    Location loc {root->loc};

    while (m_currentToken.type == TIMES || m_currentToken.type == DIVIDE) {
        auto operation {multiplicativeOperation()};
        auto rhs {factor()};
        root = make_unique<MultiplicativeExpressionNode>(
            operation, std::move(root), std::move(rhs), loc);
    }

    return root;
}

/**
 * Parses mulop -> TIMES | DIVIDE
 */
MultiplicativeOp Parser::multiplicativeOperation() {
    if (mul_ops.contains(m_currentToken.type)) {
        MultiplicativeOp operation {mul_ops.at(m_currentToken.type)};
        getToken();
        return operation;
    }

    throw error("multiplication operator", "TIMES or DIVIDE");
}

/**
 * Parses factor -> LPAREN expression RPAREN | var | call | NUM
 */
unique_ptr<ExpressionNode> Parser::factor() {
    switch (m_currentToken.type) {
    case LPAREN: {
        match("factor", LPAREN);
        auto expr {expression()};
        match("factor", RPAREN);
        return expr;
    }
    case NUM: {
        auto [_, __, num, loc] {match("factor", NUM)};
        return make_unique<IntegerLiteralExpressionNode>(num, loc);
    }
    case ID:
        if (peekToken(1).type == LPAREN) {
            return functionCall();
        }

        return variableExpression();
    default:
        throw error(
            "factor", "( expression ), variable, function call, or literal");
    }
}

/**
 * Parses call -> ID LPAREN args RPAREN
 */
unique_ptr<CallExpressionNode> Parser::functionCall() {
    auto [_, id, __, loc] {match("function call", ID)};
    match("function call", LPAREN);
    auto args {functionArguments()};
    match("function call", RPAREN);

    return make_unique<CallExpressionNode>(id, std::move(args), loc);
}

/**
 * Parses args -> arg-list | empty
 */
vector<unique_ptr<ExpressionNode>> Parser::functionArguments() {
    if (m_currentToken.type == RPAREN) {
        return {};
    }

    return argumentList();
}

/**
 * Parses args-list -> args-list COMMA expression | expression
 *
 * Implemented as args-list -> expression { COMMA expression }
 */
vector<unique_ptr<ExpressionNode>> Parser::argumentList() {
    vector<unique_ptr<ExpressionNode>> args;

    args.emplace_back(expression());

    while (m_currentToken.type == COMMA) {
        match("argument list", COMMA);
        args.emplace_back(expression());
    }

    return args;
}

/***********************************************************************/

Parser::Parser(Lexer&& lexer)
    : m_lexer {std::move(lexer)}, m_currentToken {Token {END_OF_FILE}} {}

unique_ptr<Node> Parser::parse() {
    // Pull first token from lexer to start with good state
    getToken();

    return program();
}

Token const& Parser::getToken() {
    if (!m_peekedTokens.empty()) {
        m_currentToken = m_peekedTokens.front();
        m_peekedTokens.pop_front();
    } else {
        m_currentToken = m_lexer.getToken();
    }

    return m_currentToken;
}

Token const& Parser::peekToken(size_t index) {
    if (index == 0) {
        return m_currentToken;
    }

    index--;

    while (index >= m_peekedTokens.size()) {
        m_peekedTokens.push_back(m_lexer.getToken());
    }

    return m_peekedTokens[index];
}

Token const Parser::match(
    const std::string_view construct,
    const TokenType expected_token) {
    if (m_currentToken.type == expected_token) {
        Token matched_tok {m_currentToken};
        getToken();
        return matched_tok;
    } else {
        throw ParserException {
            construct, m_currentToken, token_types.at(expected_token)};
    }
}

ParserException Parser::error(
    const std::string_view function,
    const std::string_view expected) {
    return ParserException {function, m_currentToken, expected};
}

/***********************************************************************/

ParserException::ParserException(
    const std::string_view construct,
    Token received_token,
    const std::string_view expected)
    : CMinusException {received_token.loc}, m_receivedToken {received_token} {
    std::stringstream message_buffer;
    message_buffer << "Error while parsing " << std::quoted(construct) << '\n'
                   << "  Encountered: " << std::quoted(m_receivedToken.lexeme)
                   << " (line " << location.line_num << ", column "
                   << location.col_num << ")\n"
                   << "  Expected   : " << expected;
    m_errorMessage = message_buffer.str();
}

char const* ParserException::what() const noexcept {
    return m_errorMessage.c_str();
}

/***********************************************************************/
