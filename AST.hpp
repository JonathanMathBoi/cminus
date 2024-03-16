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
    enum type { VOID, INT } type;
    bool is_array;
};

enum class add_op { PLUS, MINUS };

enum class mul_op { TIMES, DIVIDE };

enum class rel_op { LT, LTE, GT, GTE, EQ, NEQ };

enum class unary_op { INCREMENT, DECREMENT };

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

/// Abstract AST Node
///
/// Serves as the abstract base of the ATS node inheritance tree.
struct node {
    /// Constructs a node
    ///
    /// Because node is abstract, this constructor is only useful for extending
    /// classes and structs.
    ///
    /// \param line_num the line number of the source code where the construct
    ///                 begins
    /// \param col_num the column number of the source code where the construct
    ///                begins
    node(int line_num, int col_num) : line_num {line_num}, col_num {col_num} {}

    virtual ~node();

    /// Accepts a visitor to visit this node
    ///
    /// This should always be a one line function calling the correct visit
    /// overload with the `this` parameter.
    virtual void accept(visitor& visitor) = 0;

    /// The line number of the start of the construct which the node represents.
    int line_num;

    /// The column number of the start of the construct which the node
    /// represents.
    int col_num;
};

/***********************************************************************/
// Program Root Node

struct program_node : node {
    program_node(std::vector<std::unique_ptr<declaration_node>> declarations);

    virtual ~program_node() = default;

    virtual void accept(visitor& visitor) override;

    std::vector<std::unique_ptr<declaration_node>> declarations;
};

/***********************************************************************/
// Declaration Nodes

struct declaration_node : node {
    declaration_node(value_type type, std::string identifier);

    virtual ~declaration_node() = default;

    virtual void accept(visitor& visitor) = 0;

    value_type type;
    std::string identifier;

    int nest_level;
};

struct function_declaration_node : declaration_node {
    function_declaration_node(
        value_type type,
        std::string identifier,
        std::vector<std::unique_ptr<param_node>> params,
        std::unique_ptr<compound_statement_node> body);

    virtual ~function_declaration_node() = default;

    virtual void accept(visitor& visitor) override;

    std::vector<std::unique_ptr<param_node>> parameters;
    std::unique_ptr<compound_statement_node> function_body;
};

struct variable_declaration_node : declaration_node {
    variable_declaration_node(value_type type, std::string identifier);

    virtual ~variable_declaration_node() = default;

    virtual void accept(visitor& visitor) override;
};

struct array_declaration_node : variable_declaration_node {
    array_declaration_node(value_type type, std::string identifier, int size);

    virtual ~array_declaration_node() = default;

    virtual void accept(visitor& visitor) override;

    // Using int as the grammar specifies the size as just a number.
    // !! Check for bad values in semantic analysis. !!
    int size;
};

struct param_node : declaration_node {
    param_node(value_type type, std::string identifier);

    virtual ~param_node() = default;

    virtual void accept(visitor& visitor) override;
};

/***********************************************************************/
// Expression Nodes

struct expression_node : node {
    virtual ~expression_node();

    virtual void accept(visitor& visitor) = 0;
};

struct assignment_expression_node : expression_node {
    assignment_expression_node(
        std::unique_ptr<variable_expression_node> var,
        std::unique_ptr<expression_node> expr);

    virtual ~assignment_expression_node() = default;

    virtual void accept(visitor& visitor) override;

    std::unique_ptr<variable_expression_node> variable;
    std::unique_ptr<expression_node> expression;
};

struct variable_expression_node : expression_node {
    variable_expression_node(std::string identifier);

    virtual ~variable_expression_node() = default;

    virtual void accept(visitor& visitor) override;

    std::string identifier;
};

struct subscript_expression_node : variable_expression_node {
    subscript_expression_node(
        std::string identifier,
        std::unique_ptr<expression_node> index);

    virtual ~subscript_expression_node() = default;

    virtual void accept(visitor& visitor) override;

    std::unique_ptr<expression_node> index;
};

struct call_expression_node : expression_node {
    call_expression_node(
        std::string identifier,
        std::vector<std::unique_ptr<expression_node>> args);

    virtual ~call_expression_node() = default;

    virtual void accept(visitor& visitor) override;

    std::string identifier;
    std::vector<std::unique_ptr<expression_node>> arguments;
};

struct additive_expression_node : expression_node {
    additive_expression_node(
        add_op operation,
        std::unique_ptr<expression_node> lhs,
        std::unique_ptr<expression_node> rhs);

    virtual ~additive_expression_node() = default;

    virtual void accept(visitor& visitor) override;

    add_op operation;
    std::unique_ptr<expression_node> left;
    std::unique_ptr<expression_node> right;
};

struct multiplicative_expression_node : expression_node {
    multiplicative_expression_node(
        mul_op operation,
        std::unique_ptr<expression_node> lhs,
        std::unique_ptr<expression_node> rhs);

    virtual ~multiplicative_expression_node() = default;

    virtual void accept(visitor& visitor) override;

    mul_op operation;
    std::unique_ptr<expression_node> left;
    std::unique_ptr<expression_node> right;
};

struct relational_expression_node : expression_node {
    relational_expression_node(
        rel_op operation,
        std::unique_ptr<expression_node> lhs,
        std::unique_ptr<expression_node> rhs);

    virtual ~relational_expression_node() = default;

    virtual void accept(visitor& visitor) override;

    rel_op operation;
    std::unique_ptr<expression_node> left;
    std::unique_ptr<expression_node> right;
};

// Future Work: parse unary expressions

struct integer_literal_expression_node : expression_node {
    integer_literal_expression_node(int value);

    virtual ~integer_literal_expression_node() = default;

    virtual void accept(visitor& visitor) override;

    int value;
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
        std::vector<std::unique_ptr<statement_node>> stmts);

    virtual ~compound_statement_node() = default;

    virtual void accept(visitor& visitor) override;

    std::vector<std::unique_ptr<variable_declaration_node>> local_decls;
    std::vector<std::unique_ptr<statement_node>> statements;
};

struct if_statement_node : statement_node {
    if_statement_node(
        std::unique_ptr<expression_node> condition,
        std::unique_ptr<statement_node> then_stmt,
        std::optional<std::unique_ptr<statement_node>> else_stmt =
            std::nullopt);

    virtual ~if_statement_node() = default;

    virtual void accept(visitor& visitor) override;

    std::unique_ptr<expression_node> condition;
    std::unique_ptr<statement_node> then_stmt;
    std::optional<std::unique_ptr<statement_node>> else_stmt;
};

struct while_statement_node : statement_node {
    while_statement_node(
        std::unique_ptr<expression_node> condition,
        std::unique_ptr<statement_node> stmt);

    virtual ~while_statement_node() = default;

    virtual void accept(visitor& visitor) override;

    std::unique_ptr<expression_node> condition;
    std::unique_ptr<statement_node> body;
};

// Future Work: for_statement_node

struct return_statement_node : statement_node {
    return_statement_node(
        std::optional<std::unique_ptr<expression_node>> expr = std::nullopt);

    virtual ~return_statement_node() = default;

    virtual void accept(visitor& visitor) override;

    std::optional<std::unique_ptr<expression_node>> expression;
};

struct expression_statement_node : statement_node {
    expression_statement_node(
        std::optional<std::unique_ptr<expression_node>> expr = std::nullopt);

    virtual ~expression_statement_node() = default;

    virtual void accept(visitor& visitor) override;

    std::optional<std::unique_ptr<expression_node>> expr;
};

/***********************************************************************/

#endif
