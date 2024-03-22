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
unique_ptr<ProgramNode> parser::program() {
    return make_unique<ProgramNode>(decl_list());
}

/**
 * Parses declaration-list -> declaration-list declaration | declaration
 *
 * Implemented as declaration-list -> declaration { declaration }
 */
vector<shared_ptr<DeclarationNode>> parser::decl_list() {
    vector<shared_ptr<DeclarationNode>> decls;

    do {
        decls.emplace_back(declaration());
    } while (m_current_token.type != END_OF_FILE);

    return decls;
}

/**
 * Parses declaration -> var-declaration | fun-declaration
 */
unique_ptr<DeclarationNode> parser::declaration() {
    switch (peek_token(2).type) {
    case SEMI:
    case LBRACK:
        return var_decl();
    case LPAREN:
        return fun_decl();
    default:
        type_spec();
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
unique_ptr<VariableDeclarationNode> parser::var_decl() {
    auto spec {type_spec()};
    value_type type {spec.first};
    location loc {spec.second};

    std::string id {match("variable declaration", ID).lexeme};

    unique_ptr<VariableDeclarationNode> new_node;

    if (m_current_token.type == LBRACK) {
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
std::pair<basic_type, location> parser::type_spec() {
    static const std::map<TokenType, basic_type> types {
        {VOID, basic_type::VOID}, {INT, basic_type::INT}};

    if (types.contains(m_current_token.type)) {
        std::pair<basic_type, location> ret {
            types.at(m_current_token.type), m_current_token.loc};
        get_token();
        return ret;
    }

    throw error("type specifier", "INT or VOID");
}

/**
 * Parses fun-declaration
 *          -> type-specifier ID LPAREN params RPAREN compound-stmt
 */
unique_ptr<FunctionDeclarationNode> parser::fun_decl() {
    auto spec {type_spec()};
    value_type type {spec.first};
    location loc {spec.second};

    std::string id {match("function declaration", ID).lexeme};

    match("function declaration", LPAREN);

    auto parameters {params()};

    match("function declaration", RPAREN);

    auto body {compound_stmt()};

    return make_unique<FunctionDeclarationNode>(
        type, id, parameters, std::move(body), loc);
}

/**
 * Parses params -> param-list | VOID
 */
vector<shared_ptr<ParameterNode>> parser::params() {
    if (m_current_token.type == VOID && peek_token(1).type == RPAREN) {
        match("parameters", VOID);
        // returns an empty vector
        return {};
    }

    return param_list();
}

/**
 * Parses param-list -> param-list COMMA param | param
 *
 * Implemented as param-list -> param { COMMA param }
 */
vector<shared_ptr<ParameterNode>> parser::param_list() {
    vector<shared_ptr<ParameterNode>> params;

    params.emplace_back(param());

    while (m_current_token.type == COMMA) {
        match("parameter list", COMMA);
        params.emplace_back(param());
    }

    return params;
}

/**
 * Parses param -> type-specifier ID | type-specifier ID LBRACK RBRACK
 *
 * Implemented as param -> type-specifier ID [ LBRACK RBRACK ]
 */
unique_ptr<ParameterNode> parser::param() {
    auto spec {type_spec()};
    value_type type {spec.first};
    location loc {spec.second};

    std::string id {match("parameter", ID).lexeme};

    if (m_current_token.type == LBRACK) {
        match("parameter", LBRACK);
        match("parameter", RBRACK);
        type.is_array = true;
    }

    return make_unique<ParameterNode>(type, id, loc);
}

/**
 * Parses compound-stmt -> LBRACE local-delarations statement-list RBRACE
 */
unique_ptr<CompoundStatementNode> parser::compound_stmt() {
    location loc {match("compound statement", LBRACE).loc};
    auto locals {local_decls()};
    auto statements {stmt_list()};
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
vector<shared_ptr<VariableDeclarationNode>> parser::local_decls() {
    vector<shared_ptr<VariableDeclarationNode>> decls;

    while (m_current_token.type == VOID || m_current_token.type == INT) {
        decls.emplace_back(var_decl());
    }

    return decls;
}

/**
 * Parses statement-list -> statement-list statement | empty
 *
 * Implemented as statement-list -> { statement }
 */
vector<unique_ptr<StatementNode>> parser::stmt_list() {
    vector<unique_ptr<StatementNode>> stmts;

    while (m_current_token.type != RBRACE) {
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
unique_ptr<StatementNode> parser::statement() {
    switch (m_current_token.type) {
    case IF:
        return if_statement();
    case WHILE:
        return while_statement();
    case RETURN:
        return return_stmt();
    case LBRACE:
        return compound_stmt();
    default:
        return expr_stmt();
    }
}

/**
 * Parses expression-stmt -> expression SEMI | SEMI
 *
 * Implemented as expression-stmt -> [ expression ] SEMI
 */
unique_ptr<ExpressionStatementNode> parser::expr_stmt() {
    if (m_current_token.type == SEMI) {
        location loc {match("expression statement", SEMI).loc};
        return make_unique<ExpressionStatementNode>(loc);
    }

    auto expr {expression()};
    location loc {expr->loc};
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
unique_ptr<IfStatementNode> parser::if_statement() {
    location loc {match("if statement", IF).loc};
    match("if statement", LPAREN);
    auto condition {expression()};
    match("if statement", RPAREN);
    auto then_stmt {statement()};

    if (m_current_token.type == ELSE) {
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
unique_ptr<WhileStatementNode> parser::while_statement() {
    location loc {match("while statement", WHILE).loc};
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
unique_ptr<ReturnStatementNode> parser::return_stmt() {
    location loc {match("return statement", RETURN).loc};

    if (m_current_token.type == SEMI) {
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
unique_ptr<expression_node> parser::expression() {
    if (m_current_token.type == ID && peek_token(1).type == ASSIGN) {
        return assignment_expr();
    }

    if (m_current_token.type == ID && peek_token(1).type == LBRACK) {
        size_t peek_idx {1};
        unsigned nest_level {0};
        do {
            switch (peek_token(peek_idx).type) {
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
                variable();
                break;
            default:
                break;
            }
            peek_idx++;
        } while (nest_level != 0);

        if (peek_token(peek_idx).type == ASSIGN) {
            return assignment_expr();
        }
    }

    return relational_expr();
}

/**
 * Parses assign-expression -> var ASSIGN expression
 */
unique_ptr<assignment_expression_node> parser::assignment_expr() {
    auto var {variable()};
    match("assignment expression", ASSIGN);
    auto expr {expression()};

    location loc {var->loc};

    return make_unique<assignment_expression_node>(
        std::move(var), std::move(expr), loc);
}

/**
 * Parses var -> ID | ID LBRACK expression RBRACK
 *
 * Implemented as var -> ID [ LBARCK expression RBRACK ]
 */
unique_ptr<variable_expression_node> parser::variable() {
    auto [_, id, __, loc] {match("variable", ID)};

    if (m_current_token.type == LBRACK) {
        match("variable", LBRACK);
        auto subscript {expression()};
        match("variable", RBRACK);

        return make_unique<subscript_expression_node>(
            id, std::move(subscript), loc);
    }

    return make_unique<variable_expression_node>(id, loc);
}

/**
 * Parses simple-expression -> additive-expression relop additive-expression
 *                           | additive-expression
 *
 * Implemented as simple-expression
 *                  -> additive-expression [ relop additive-expression ]
 */
unique_ptr<expression_node> parser::relational_expr() {
    auto lhs {add_expr()};
    location loc {lhs->loc};

    switch (m_current_token.type) {
    case LT:
    case LTE:
    case GT:
    case GTE:
    case EQ:
    case NEQ: {
        rel_op operation {relation_op()};
        auto rhs {add_expr()};
        return make_unique<relational_expression_node>(
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
rel_op parser::relation_op() {
    static const std::map<TokenType, rel_op> rel_ops {
        {LT, rel_op::LT},   {LTE, rel_op::LTE}, {GT, rel_op::GT},
        {GTE, rel_op::GTE}, {EQ, rel_op::EQ},   {NEQ, rel_op::NEQ}};

    if (rel_ops.contains(m_current_token.type)) {
        rel_op operation {rel_ops.at(m_current_token.type)};
        get_token();
        return operation;
    }

    throw error("relational operator", "LT, LTE, GT, GTE, EQ, or NEQ");
}

const std::map<TokenType, add_op> add_ops {
    {PLUS, add_op::PLUS},
    {MINUS, add_op::MINUS}};

/**
 * Parses additive-expression -> additive-expression addop term | term
 *
 * Implemented as additive-expression -> term { addop term }
 */
unique_ptr<expression_node> parser::add_expr() {
    auto root {term()};
    location loc {root->loc};

    while (add_ops.contains(m_current_token.type)) {
        auto operation {additive_op()};
        auto rhs {term()};
        root = make_unique<additive_expression_node>(
            operation, std::move(root), std::move(rhs), loc);
    }

    return root;
}

/**
 * Parses addop -> PLUS | MINUS
 */
add_op parser::additive_op() {
    if (add_ops.contains(m_current_token.type)) {
        add_op operation {add_ops.at(m_current_token.type)};
        get_token();
        return operation;
    }

    throw error("addition operator", "PLUS or MINUS");
}

const std::map<TokenType, mul_op> mul_ops {
    {TIMES, mul_op::TIMES},
    {DIVIDE, mul_op::DIVIDE}};

/**
 * Parses term -> term mulop factor | factor
 *
 * Implemented as term -> factor { mulop factor }
 */
unique_ptr<expression_node> parser::term() {
    auto root {factor()};
    location loc {root->loc};

    while (m_current_token.type == TIMES || m_current_token.type == DIVIDE) {
        auto operation {mult_op()};
        auto rhs {factor()};
        root = make_unique<multiplicative_expression_node>(
            operation, std::move(root), std::move(rhs), loc);
    }

    return root;
}

/**
 * Parses mulop -> TIMES | DIVIDE
 */
mul_op parser::mult_op() {
    if (mul_ops.contains(m_current_token.type)) {
        mul_op operation {mul_ops.at(m_current_token.type)};
        get_token();
        return operation;
    }

    throw error("multiplication operator", "TIMES or DIVIDE");
}

/**
 * Parses factor -> LPAREN expression RPAREN | var | call | NUM
 */
unique_ptr<expression_node> parser::factor() {
    switch (m_current_token.type) {
    case LPAREN: {
        match("factor", LPAREN);
        auto expr {expression()};
        match("factor", RPAREN);
        return expr;
    }
    case NUM: {
        auto [_, __, num, loc] {match("factor", NUM)};
        return make_unique<integer_literal_expression_node>(num, loc);
    }
    case ID:
        if (peek_token(1).type == LPAREN) {
            return fun_call();
        }

        return variable();
    default:
        throw error(
            "factor", "( expression ), variable, function call, or literal");
    }
}

/**
 * Parses call -> ID LPAREN args RPAREN
 */
unique_ptr<call_expression_node> parser::fun_call() {
    auto [_, id, __, loc] {match("function call", ID)};
    match("function call", LPAREN);
    auto args {fun_args()};
    match("function call", RPAREN);

    return make_unique<call_expression_node>(id, std::move(args), loc);
}

/**
 * Parses args -> arg-list | empty
 */
vector<unique_ptr<expression_node>> parser::fun_args() {
    if (m_current_token.type == RPAREN) {
        return {};
    }

    return args_list();
}

/**
 * Parses args-list -> args-list COMMA expression | expression
 *
 * Implemented as args-list -> expression { COMMA expression }
 */
vector<unique_ptr<expression_node>> parser::args_list() {
    vector<unique_ptr<expression_node>> args;

    args.emplace_back(expression());

    while (m_current_token.type == COMMA) {
        match("argument list", COMMA);
        args.emplace_back(expression());
    }

    return args;
}

/***********************************************************************/

parser::parser(lexer&& lexer)
    : m_lexer {std::move(lexer)}, m_current_token {Token {END_OF_FILE}} {}

unique_ptr<Node> parser::parse() {
    // Pull first token from lexer to start with good state
    get_token();

    return program();
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

Token const parser::match(
    const std::string_view construct,
    const TokenType expected_token) {
    if (m_current_token.type == expected_token) {
        Token matched_tok {m_current_token};
        get_token();
        return matched_tok;
    } else {
        throw parser_exception {
            construct, m_current_token, token_types.at(expected_token)};
    }
}

parser_exception parser::error(
    const std::string_view function,
    const std::string_view expected) {
    return parser_exception {function, m_current_token, expected};
}

/***********************************************************************/

parser_exception::parser_exception(
    const std::string_view construct,
    Token received_token,
    const std::string_view expected)
    : cminus_exception {received_token.loc}, m_received_token {received_token} {
    std::stringstream message_buffer;
    message_buffer << "Error while parsing " << std::quoted(construct) << '\n'
                   << "  Encountered: " << std::quoted(m_received_token.lexeme)
                   << " (line " << location.line_num << ", column "
                   << location.col_num << ")\n"
                   << "  Expected   : " << expected;
    m_error_message = message_buffer.str();
}

char const* parser_exception::what() const noexcept {
    return m_error_message.c_str();
}

/***********************************************************************/
