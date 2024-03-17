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

/***********************************************************************/

/**
 * Parses program -> decleration-list
 */
std::unique_ptr<program_node> parser::program() {
    return std::make_unique<program_node>(decl_list());
}

/**
 * Parses declaration-list -> declaration-list declaration | declaration
 *
 * Implemented as declaration-list -> declaration { declaration }
 */
std::vector<std::shared_ptr<declaration_node>> parser::decl_list() {
    std::vector<std::shared_ptr<declaration_node>> decls;

    do {
        decls.emplace_back(declaration());
    } while (m_current_token.type != END_OF_FILE);

    return decls;
}

/**
 * Parses declaration -> var-declaration | fun-declaration
 */
std::unique_ptr<declaration_node> parser::declaration() {
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
std::unique_ptr<variable_declaration_node> parser::var_decl() {
    auto spec {type_spec()};
    value_type type {spec.first};
    location loc {spec.second};

    std::string id {match("variable declaration", ID).lexeme};

    std::unique_ptr<variable_declaration_node> new_node;

    if (m_current_token.type == LBRACK) {
        type.is_array = true;

        match("variable declaration", LBRACK);
        int size {match("variable declaration", NUM).number};
        match("variable declaration", RBRACK);

        new_node =
            std::make_unique<array_declaration_node>(type, id, size, loc);
    } else {
        new_node = std::make_unique<variable_declaration_node>(type, id, loc);
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
std::unique_ptr<function_declaration_node> parser::fun_decl() {
    auto spec {type_spec()};
    value_type type {spec.first};
    location loc {spec.second};

    std::string id {match("function declaration", ID).lexeme};

    match("function declaration", LPAREN);

    auto parameters {params()};

    match("function declaration", RPAREN);

    auto body {compound_stmt()};

    return std::make_unique<function_declaration_node>(
        type, id, parameters, std::move(body), loc);
}

/**
 * Parses params -> param-list | VOID
 */
std::vector<std::shared_ptr<param_node>> parser::params() {
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
std::vector<std::shared_ptr<param_node>> parser::param_list() {
    std::vector<std::shared_ptr<param_node>> params;

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
std::unique_ptr<param_node> parser::param() {
    auto spec {type_spec()};
    value_type type {spec.first};
    location loc {spec.second};

    std::string id {match("parameter", ID).lexeme};

    if (m_current_token.type == LBRACK) {
        match("parameter", LBRACK);
        match("parameter", RBRACK);
        type.is_array = true;
    }

    return std::make_unique<param_node>(type, id, loc);
}

/**
 * Parses compound-stmt -> LBRACE local-delarations statement-list RBRACE
 */
std::unique_ptr<compound_statement_node> parser::compound_stmt() {
    location loc {match("compound statement", LBRACE).loc};
    auto locals {local_decls()};
    auto statements {stmt_list()};
    match("compound statement", RBRACE);
    return std::make_unique<compound_statement_node>(
        locals, std::move(statements), loc);
}

/**
 * Parses local-declarations -> local-delarations var-delcaration
 *                            | empty
 *
 * Implemented as local-declarations -> { var-declaration }
 */
std::vector<std::shared_ptr<variable_declaration_node>> parser::local_decls() {
    std::vector<std::shared_ptr<variable_declaration_node>> decls;

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
std::vector<std::unique_ptr<statement_node>> parser::stmt_list() {
    std::vector<std::unique_ptr<statement_node>> stmts;

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
std::unique_ptr<statement_node> parser::statement() {
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
std::unique_ptr<expression_statement_node> parser::expr_stmt() {
    if (m_current_token.type == SEMI) {
        location loc {match("expression statement", SEMI).loc};
        return std::make_unique<expression_statement_node>(loc);
    }

    auto expr {expression()};
    location loc {expr->loc};
    match("expression statement", SEMI);

    return std::make_unique<expression_statement_node>(std::move(expr), loc);
}

/**
 * Parses selection-stmt -> IF LPAREN expression RPAREN statement
 *                        | IF LPAREN expression RPAREN statement ELSE statement
 *
 * Implemented as selection-stmt
 *                  -> IF LPAREN expression RPAREN [ ELSE statement]
 */
std::unique_ptr<if_statement_node> parser::if_statement() {
    match("if statement", IF);
    match("if statement", LPAREN);
    expression();
    match("if statement", RPAREN);
    statement();

    if (m_current_token.type == ELSE) {
        match("if statement", ELSE);
        statement();
    }
}

/**
 * Parses iteration-stmt -> WHILE LPAREN expression RPAREN statement
 */
std::unique_ptr<while_statement_node> parser::while_statement() {
    match("while statement", WHILE);
    match("while statement", LPAREN);
    expression();
    match("while statement", RPAREN);
    statement();
}

/**
 * Parses return-stmt -> RETURN SEMI | RETURN expression SEMI
 *
 * Implemented as return-stmt -> RETURN [ expression ] SEMI
 */
std::unique_ptr<return_statement_node> parser::return_stmt() {
    match("return statement", RETURN);

    if (m_current_token.type == SEMI) {
        match("return statement", SEMI);
        // TODO: build real node
        return nullptr;
    }

    expression();
    match("return expression", SEMI);
}

/**
 * Parses expression -> var ASSIGN expression | simple_expression
 *
 * Implemented as expression -> assign-expression | simple-expression
 *
 * Ad-hoc solution used to check for assignment expression with an indexed array
 * as the variable.
 */
std::unique_ptr<expression_node> parser::expression() {
    if (m_current_token.type == ID && peek_token(1).type == ASSIGN) {
        assignment_expr();
        // TODO: Build real node.
        return nullptr;
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
            default:
                break;
            }
            peek_idx++;
        } while (nest_level != 0);

        if (peek_token(peek_idx).type == ASSIGN) {
            assignment_expr();
            // TODO: Build real node.
            return nullptr;
        }
    }

    simple_expr();
}

/**
 * Parses assign-expression -> var ASSIGN expression
 */
void parser::assignment_expr() {
    variable();
    match("assignment expression", ASSIGN);
    expression();
}

/**
 * Parses var -> ID | ID LBRACK expression RBRACK
 *
 * Implemented as var -> ID [ LBARCK expression RBRACK ]
 */
void parser::variable() {
    match("variable", ID);

    if (m_current_token.type == LBRACK) {
        match("variable", LBRACK);
        expression();
        match("variable", RBRACK);
    }
}

/**
 * Parses simple-expression -> additive-expression relop additive-expression
 *                           | additive-expression
 *
 * Implemented as simple-expression
 *                  -> additive-expression [ relop additive-expression ]
 */
void parser::simple_expr() {
    add_expr();

    switch (m_current_token.type) {
    case LT:
    case LTE:
    case GT:
    case GTE:
    case EQ:
    case NEQ:
        relation_op();
        add_expr();
        break;
    default:
        break;
    }
}

/**
 * Parses relop -> LTE | LT | GT | GTE | EQ | NEQ
 */
void parser::relation_op() {
    switch (m_current_token.type) {
    case LT:
    case LTE:
    case GT:
    case GTE:
    case EQ:
    case NEQ:
        get_token();
        break;
    default:
        throw error("relational operator", "LT, LTE, GT, GTE, EQ, or NEQ");
    }
}

/**
 * Parses additive-expression -> additive-expression addop term | term
 *
 * Implemented as additive-expression -> term { addop term }
 */
void parser::add_expr() {
    term();

    while (m_current_token.type == PLUS || m_current_token.type == MINUS) {
        add_op();
        term();
    }
}

/**
 * Parses addop -> PLUS | MINUS
 */
void parser::add_op() {
    switch (m_current_token.type) {
    case PLUS:
    case MINUS:
        get_token();
        break;
    default:
        throw error("addition operator", "PLUS or MINUS");
    }
}

/**
 * Parses term -> term mulop factor | factor
 *
 * Implemented as term -> factor { mulop factor }
 */
void parser::term() {
    factor();

    while (m_current_token.type == TIMES || m_current_token.type == DIVIDE) {
        mul_op();
        factor();
    }
}

/**
 * Parses mulop -> TIMES | DIVIDE
 */
void parser::mul_op() {
    switch (m_current_token.type) {
    case TIMES:
    case DIVIDE:
        get_token();
        break;
    default:
        throw error("multiplication operator", "TIMES or DIVIDE");
    }
}

/**
 * Parses factor -> LPAREN expression RPAREN | var | call | NUM
 */
void parser::factor() {
    switch (m_current_token.type) {
    case LPAREN:
        match("factor", LPAREN);
        expression();
        match("factor", RPAREN);
        break;
    case NUM:
        match("factor", NUM);
        break;
    case ID:
        if (peek_token(1).type == LPAREN) {
            fun_call();
            return;
        }

        variable();
        break;
    default:
        throw error(
            "factor", "( expression ), variable, function call, or literal");
    }
}

/**
 * Parses call -> ID LPAREN RPAREN
 */
void parser::fun_call() {
    match("function call", ID);
    match("function call", LPAREN);
    fun_args();
    match("function call", RPAREN);
}

/**
 * Parses args -> arg-list | empty
 */
void parser::fun_args() {
    if (m_current_token.type == RPAREN) {
        return;
    }

    args_list();
}

/**
 * Parses args-list -> args-list COMMA expression | expression
 *
 * Implemented as args-list -> expression { COMMA expression }
 */
void parser::args_list() {
    expression();

    while (m_current_token.type == COMMA) {
        match("argument list", COMMA);
        expression();
    }
}

/***********************************************************************/

parser::parser(lexer&& lexer)
    : m_lexer {std::move(lexer)}, m_current_token {Token {END_OF_FILE}} {}

std::unique_ptr<node> parser::parse() {
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
