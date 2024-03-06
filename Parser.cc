#include "Parser.hpp"
#include "Lexer.hpp"
#include "Exception.hpp"

#include <cstddef>
#include <iomanip>
#include <sstream>
#include <string_view>

/***********************************************************************/

/**
 * Parses program -> decleration-list
 */
void parser::program() {
    decl_list();
}

/**
 * Parses declaration-list -> declaration-list declaration | declaration
 *
 * Implemented as declaration-list -> declaration { declaration }
 */
void parser::decl_list() {
    do {
        declaration();
    } while(m_current_token.type != END_OF_FILE);
}

/**
 * Parses declaration -> var-declaration | fun-declaration
 */
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

/**
 * Parses var-declaration -> type-specifier ID SEMI
 *                         | type-specifier ID LBRACK NUM RBRACK SEMI
 * 
 * Implemented as var-declaration
 *                  -> type-specifier ID [ LBRACK NUM RBRACK ] SEMI
 */
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

/**
 * Parses type-specifier -> INT | VOID
 */
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

/**
 * Parses fun-declaration
 *          -> type-specifier ID LPAREN params RPAREN compound-stmt
 */
void parser::fun_decl() {
    type_spec();
    match("function declaration", ID);
    match("function declaration", LPAREN);
    params();
    match("function declaration", RPAREN);
    compound_stmt();
}

/**
 * Parses params -> param-list | VOID
 */
void parser::params() {
    if (m_current_token.type == VOID && peek_token(1).type == RPAREN) {
        match("parameters", VOID);
        return;
    }

    param_list();
}

/**
 * Parses param-list -> param-list COMMA param | param
 *
 * Implemented as param-list -> param { COMMA param }
 */
void parser::param_list() {
    param();

    while (m_current_token.type == COMMA) {
        match("parameter list", COMMA);
        param();
    }
}

/**
 * Parses param -> type-specifier ID | type-specifier ID LBRACK RBRACK
 *
 * Implemented as param -> type-specifier ID [ LBRACK RBRACK ]
 */
void parser::param() {
    type_spec();
    match("parameter", ID);

    if (m_current_token.type == LBRACK) {
        match("parameter", LBRACK);
        match("parameter", RBRACK);
    }
}

/**
 * Parses compound-stmt -> LBRACE local-delarations statement-list RBRACE
 */
void parser::compound_stmt() {
    match("compound statement", LBRACE);
    local_decls();
    stmt_list();
    match("compound statement", RBRACE);
}

/**
 * Parses local-declarations -> local-delarations var-delcaration
 *                            | empty
 * 
 * Implemented as local-declarations -> { var-declaration }
 */
void parser::local_decls() {
    while (m_current_token.type == VOID || m_current_token.type == INT) {
        var_decl();
    }
}

/**
 * Parses statement-list -> statement-list statement | empty
 *
 * Implemented as statement-list -> { statement }
 */
void parser::stmt_list() {
    while (m_current_token.type != RBRACE) {
        statement();
    }
}

/**
 * Parses statement -> expression-stmt
 *                   | compound-stmt
 *                   | selection-stmt
 *                   | iteration-stmt
 *                   | return-stmt
 */
void parser::statement() {
    switch (m_current_token.type) {
    case IF:
        if_statement();
        break;
    case WHILE:
        while_statement();
        break;
    case RETURN:
        return_stmt();
        break;
    case LBRACE:
        compound_stmt();
        break;
    default:
        expr_stmt();
        break;
    }
}

/**
 * Parses expression-stmt -> expression SEMI | SEMI
 *
 * Implemented as expression-stmt -> [ expression ] SEMI
 */
void parser::expr_stmt() {
    if (m_current_token.type == SEMI) {
        match("expression statement", SEMI);
        return;
    }

    expression();
    match("expression statement", SEMI); 
}

/**
 * Parses selection-stmt -> IF LPAREN expression RPAREN statement
 *                        | IF LPAREN expression RPAREN statement ELSE statement
 *
 * Implemented as selection-stmt
 *                  -> IF LPAREN expression RPAREN [ ELSE statement]
 */
void parser::if_statement() {
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
void parser::while_statement() {
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
void parser::return_stmt() {
    match("return statement", RETURN);
    
    if (m_current_token.type == SEMI) {
        match("return statement", SEMI);
        return;
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
void parser::expression() {
    if (m_current_token.type == ID && peek_token(1).type == ASSIGN) {
        assignment_expr();
        return;
    } else if (m_current_token.type == ID && peek_token(1).type == LBRACK) {
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
            return;
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
        error("relational operator", "LT, LTE, GT, GTE, EQ, or NEQ");
        break;
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
        error("addition operator", "PLUS or MINUS");
        break;
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
        error("multiplication operator", "TIMES or DIVIDE");
        break;
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
        error("factor", "( expression ), variable, function call, or literal");
        break;
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
            token_types.at(expected_token),
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

