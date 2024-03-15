#ifndef AST_HPP
#define AST_HPP

/***********************************************************************/

#include <memory>
#include <optional>
#include <string>
#include <vector>

/***********************************************************************/
// Foward Class Declarations

// Visitor
class visitor;

// Abstract Base Node
struct node;

// Program Root Node
struct program_node;

// Declaration Nodes
struct declaration_node;
struct function_declaration_node;
struct param_node;
struct variable_declaration_node;
struct array_declaration_node;

// Statement Nodes
struct statement_node;
struct compound_statement_node;
struct if_statement_node;
struct while_statement_node;
// Not parsing for statements yet
// struct for_statement_node;
struct return_statement_node;
struct expression_statement_node;

// Expression Nodes
struct expression_node;
struct assignment_expression_node;
struct variable_expression_node;
struct subscript_expression_node;
struct call_expression_node;
struct additive_expression_node;
struct multiplicative_expression_node;
struct relational_expression_node;
// Not parsing increment and decrement yet
// struct unary_expression_node;
struct integer_literal_expression_node;

/***********************************************************************/
// Enums and bookkeeping

struct value_type {
    enum type {
        VOID, INT /*, FLOAT */
    } type;
    bool is_array;
};

/***********************************************************************/
// Abstract Classes

class visitor {
public:
    virtual void visit(program_node& node) = 0;

    virtual void visit(declaration_node& node) = 0;
    virtual void visit(function_declaration_node& node) = 0;
    virtual void visit(variable_declaration_node& node) = 0;
    virtual void visit(array_declaration_node& node) = 0;
    virtual void visit(param_node& node) = 0;

    virtual void visit(statement_node& node) = 0;
    virtual void visit(compound_statement_node& node) = 0;
    virtual void visit(if_statement_node& node) = 0;
    virtual void visit(while_statement_node& node) = 0;
    // Not parsing for statement yet
    // virtual void visit(for_statement_node& node) = 0;
    virtual void visit(return_statement_node& node) = 0;
    virtual void visit(expression_statement_node& node) = 0;

    virtual void visit(expression_node& node) = 0;
    virtual void visit(assignment_expression_node& node) = 0;
    virtual void visit(variable_expression_node& node) = 0;
    virtual void visit(subscript_expression_node& node) = 0;
    virtual void visit(call_expression_node& node) = 0;
    virtual void visit(additive_expression_node& node) = 0;
    virtual void visit(multiplicative_expression_node& node) = 0;
    virtual void visit(relational_expression_node& node) = 0;
    // Not parsing increment and decrement yet
    // virtual void visit(unary_expression_node& node) = 0;
    virtual void visit(integer_literal_expression_node& node) = 0;
};

struct node {
    virtual ~node();

    virtual void accept(visitor& visitor) = 0;
};

/***********************************************************************/
// Program Root Node

struct program_node : node {
    program_node(std::vector<std::unique_ptr<declaration_node>> declarations);

    virtual ~program_node() = default;

    virtual void accept(visitor& visitor);

    std::vector<std::unique_ptr<declaration_node>> declarations;
};

/***********************************************************************/
// Declaration Nodes

struct declaration_node : node {
    declaration_node(value_type type, std::string identifier);

    virtual ~declaration_node() = default;

    virtual void accept(visitor& visitor) = 0;

    value_type value_type;
    std::string identifier;

    int nest_level;
};

struct function_declaration_node : declaration_node {
    function_declaration_node(
        value_type type,
        std::string identifier,
        std::vector<std::unique_ptr<param_node>> params,
        std::unique_ptr<compound_statement_node> body
    );

    virtual ~function_declaration_node() = default;

    virtual void accept(visitor& visitor);

    std::vector<std::unique_ptr<param_node>> parameters;
    std::unique_ptr<compound_statement_node> function_body;
};

struct variable_declaration_node : declaration_node {
    variable_declaration_node(value_type type, std::string identifier);

    virtual ~variable_declaration_node() = default;

    virtual void accept(visitor& visitor);
};

struct array_declaration_node : variable_declaration_node {
    array_declaration_node(value_type type, std::string identifier, int size);

    virtual ~array_declaration_node() = default;

    virtual void accept(visitor& visitor);

    // Using int as the grammar specifies the size as just a number.
    // !! Check for bad values in semantic analysis. !!
    int size;
};

struct param_node : declaration_node {
    param_node(value_type type, std::string identifier);

    virtual ~param_node() = default;

    virtual void accept(visitor& visitor);
};

/***********************************************************************/
// Statement Nodes

struct statement_node : node {
    virtual ~statement_node();

    virtual void accept(visitor& visitor) = 0;
};

struct compound_statement_node : statement_node {
    compound_statement_node(
        std::vector<std::unique_ptr<variable_declaration_node>> decls,
        std::vector<std::unique_ptr<statement_node>> stmts
    );

    virtual ~compound_statement_node() = default;

    virtual void accept(visitor& visitor);

    std::vector<std::unique_ptr<variable_declaration_node>> local_decls;
    std::vector<std::unique_ptr<statement_node>> statements;
};

struct if_statement_node : statement_node {
    if_statement_node(
        std::unique_ptr<expression_node> condition,
        std::unique_ptr<statement_node> then_stmt,
        std::optional<std::unique_ptr<statement_node>> else_stmt = std::nullopt
    );

    virtual ~if_statement_node() = default;

    virtual void accept(visitor& visitor);

    std::unique_ptr<expression_node> condition;
    std::unique_ptr<statement_node> then_stmt;
    std::optional<std::unique_ptr<statement_node>> else_stmt;
};

/***********************************************************************/

#endif

