#ifndef AST_HPP
#define AST_HPP

/***********************************************************************/

#include "MiscUtils.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>

/***********************************************************************/

class Visitor;

struct Node;

struct ProgramNode;

struct DeclarationNode;
struct FunctionDeclarationNode;
struct ParameterNode;
struct VariableDeclarationNode;
struct ArrayDeclarationNode;

struct StatementNode;
struct CompoundStatementNode;
struct IfStatementNode;
struct WhileStatementNode;
struct return_statement_node;
struct expression_statement_node;

struct expression_node;
struct assignment_expression_node;
struct variable_expression_node;
struct subscript_expression_node;
struct call_expression_node;
struct additive_expression_node;
struct multiplicative_expression_node;
struct relational_expression_node;
struct integer_literal_expression_node;

/***********************************************************************/

enum class basic_type { VOID, INT };

struct value_type {
    basic_type type;
    bool is_array;
};

enum class add_op { PLUS, MINUS };

enum class mul_op { TIMES, DIVIDE };

enum class rel_op { LT, LTE, GT, GTE, EQ, NEQ };

enum class unary_op { INCREMENT, DECREMENT };

/***********************************************************************/

class Visitor {
public:
    virtual void visit(ProgramNode& node) = 0;

    virtual void visit(DeclarationNode& node) = 0;
    virtual void visit(FunctionDeclarationNode& node) = 0;
    virtual void visit(VariableDeclarationNode& node) = 0;
    virtual void visit(ArrayDeclarationNode& node) = 0;
    virtual void visit(ParameterNode& node) = 0;

    virtual void visit(StatementNode& node) = 0;
    virtual void visit(CompoundStatementNode& node) = 0;
    virtual void visit(IfStatementNode& node) = 0;
    virtual void visit(WhileStatementNode& node) = 0;
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
struct Node {
    /// Constructs a node
    ///
    /// Because node is abstract, this constructor is only useful for extending
    /// classes and structs.
    ///
    /// \param location the location in the source code where the construct
    ///                 begins
    Node(location loc) : loc {loc} {}

    virtual ~Node() = default;

    /// Accepts a visitor to visit this node
    ///
    /// This should always be a one line function calling the correct visit
    /// overload with the `this` parameter.
    virtual void accept(Visitor& visitor) = 0;

    /// The location of the construct in the source code
    location loc;
};

/// Abstract Declaration Node
///
/// This node type serves as the base for all declaration nodes to inherit from.
///
/// When a smart pointer is needed to a declaration node, a `shared_ptr` is
/// likely the best choice, as these nodes eventually need to be pointed to at
/// both their parent and by all their references.
struct DeclarationNode : Node {
    /// Constructs a Declaration Node
    ///
    /// \param type the type for the declared construct
    /// \param identifier the identifier for the declared construct
    /// \param loc the location where the declaration begins
    DeclarationNode(value_type type, std::string identifier, location loc);

    virtual ~DeclarationNode() = default;

    virtual void accept(Visitor& visitor) = 0;

    /// The type of the declared construct
    value_type type;
    /// The identifier of the declared construct
    std::string identifier;
};

/// Abstract Expression Node
///
/// This node type serves as the base for all types of expression nodes to
/// derive from.
struct expression_node : Node {
    /// Constructs an Expression Node
    ///
    /// \param loc the location of the expression in the source code
    expression_node(location loc) : Node {loc} {}

    virtual ~expression_node() = default;

    virtual void accept(Visitor& visitor) = 0;
};

/// Abstract Statement Node
///
/// This node type serves as the base for all types of statement nodes to derive
/// from.
struct StatementNode : Node {
    /// Constructs a Statement Node
    ///
    /// \param loc the location of the statement in the source code
    StatementNode(location loc) : Node {loc} {}

    virtual ~StatementNode() = default;

    virtual void accept(Visitor& visitor) = 0;
};

/***********************************************************************/

/// Root Program Node
///
/// This node type serves as the root of the AST. It contains only a list of all
/// the top level declarations.
struct ProgramNode : Node {
    /// Constructs a Program Node
    ///
    /// \param declarations a vector of all the top level declarations in the
    ///                     program
    ProgramNode(std::vector<std::shared_ptr<DeclarationNode>> declarations);

    virtual ~ProgramNode() = default;

    virtual void accept(Visitor& visitor) override;

    /// The list of all top level declarations in the program
    ///
    /// A vector of shared_ptr is used as usage of these identifers will
    /// eventually be linked back to their delarations. As such a unique_ptr
    /// would not be applicable.
    std::vector<std::shared_ptr<DeclarationNode>> declarations;
};

/***********************************************************************/

/// Compound Statement Node
///
/// The node for a braced block of statements.
struct CompoundStatementNode : StatementNode {
    /// Constructs a Compound Statement Node
    ///
    /// \param decls the list of variable declarations at the start of the block
    /// \param stmts the list of statments in the block
    /// \param loc the location of the start of the block in the source code
    CompoundStatementNode(
        std::vector<std::shared_ptr<VariableDeclarationNode>> decls,
        std::vector<std::unique_ptr<StatementNode>> stmts,
        location loc);

    virtual ~CompoundStatementNode() = default;

    virtual void accept(Visitor& visitor) override;

    /// The list of local declarations at the start of the block
    ///
    /// A vector of shared_ptr is used as usage of these variables will
    /// eventually be linked back to their delarations here. As such a
    /// unique_ptr would not be applicable.
    std::vector<std::shared_ptr<VariableDeclarationNode>> local_decls;
    /// The list of statements in the block
    std::vector<std::unique_ptr<StatementNode>> statements;
};

/// If Statement Node
///
/// The node for an if statement.
struct IfStatementNode : StatementNode {
    /// Constructs an If Statement Node
    ///
    /// \param condition the conditional expression in the if
    /// \param then_stmt the then statement of the if statement
    /// \param else_stmt the else statement attached to the if statment
    /// \param loc the location of the start of the if statement in the source
    ///            code
    IfStatementNode(
        std::unique_ptr<expression_node> condition,
        std::unique_ptr<StatementNode> then_stmt,
        std::optional<std::unique_ptr<StatementNode>> else_stmt,
        location loc);

    /// Constructs an If Statement Node
    ///
    /// Constructs an If Statement with no attached else block.
    ///
    /// \param condition the conditional expression in the if
    /// \param then_stmt the then statement of the if statement
    /// \param loc the location of the start of the if statement in the source
    ///            code
    IfStatementNode(
        std::unique_ptr<expression_node> condition,
        std::unique_ptr<StatementNode> then_stmt,
        location loc);

    virtual ~IfStatementNode() = default;

    virtual void accept(Visitor& visitor) override;

    /// The conditional expression for the if statement
    std::unique_ptr<expression_node> condition;
    /// The then statement for the if statement
    std::unique_ptr<StatementNode> then_stmt;
    /// An optional else statement attached to the if statement
    std::optional<std::unique_ptr<StatementNode>> else_stmt;
};

/// While Statement Node
///
/// The node for a while loop statement.
struct WhileStatementNode : StatementNode {
    /// Constructs a While Statement Node
    ///
    /// \param condition the condition for the while loop to continue
    /// \param stmt the statement to be executed in the loop
    /// \param loc the location of the start of the while statement in the
    ///            source code
    WhileStatementNode(
        std::unique_ptr<expression_node> condition,
        std::unique_ptr<StatementNode> stmt,
        location loc);

    virtual ~WhileStatementNode() = default;

    virtual void accept(Visitor& visitor) override;

    /// The condition for execution of the while loop
    std::unique_ptr<expression_node> condition;
    /// The body of the while loop
    std::unique_ptr<StatementNode> body;
};

// Future Work: for_statement_node

/// Return Statement Node
///
/// The node for a return statement.
struct return_statement_node : StatementNode {
    /// Constructs a Return Statement Node
    ///
    /// \param expr the expression to be returned
    /// \param loc the location of the return statement in the source code
    return_statement_node(
        std::optional<std::unique_ptr<expression_node>> expr,
        location loc);

    /// Constructs a Return Statement Node
    ///
    /// Constructs a Return Statement Node with no associated returned
    /// expression.
    ///
    /// \param loc the location of the return statement in the source code
    return_statement_node(location loc);

    virtual ~return_statement_node() = default;

    virtual void accept(Visitor& visitor) override;

    /// An option expression to be returned
    std::optional<std::unique_ptr<expression_node>> expression;
};

/// Expression Statement Node
///
/// The node for any semicolon delimited expression statement.
struct expression_statement_node : StatementNode {
    /// Constructs an Expression Statement
    ///
    /// \param expr the expression to be evaluated
    /// \param loc the location of the expression in the source code.
    expression_statement_node(
        std::optional<std::unique_ptr<expression_node>> expr,
        location loc);

    /// Constructs an Expression Statement
    ///
    /// Constructs an Expression Statement with no associated expression. (i.e.
    /// the statement `;`.)
    ///
    /// \param loc the location of the expression in the source code.
    expression_statement_node(location loc);

    virtual ~expression_statement_node() = default;

    virtual void accept(Visitor& visitor) override;

    /// The optional expression to be evaluated
    std::optional<std::unique_ptr<expression_node>> expr;
};

/***********************************************************************/

/// Function Declaration Node
///
/// This node type represents a function declaration.
struct FunctionDeclarationNode : DeclarationNode {
    /// Constructs a Function Declaration Node
    ///
    /// \param type the function return type
    /// \param identifier the function name
    /// \param params the list of parameters of the function
    /// \param body the function body
    /// \param loc the location where the function is declared
    FunctionDeclarationNode(
        value_type type,
        std::string identifier,
        std::vector<std::shared_ptr<ParameterNode>> params,
        std::unique_ptr<CompoundStatementNode> body,
        location loc);

    virtual ~FunctionDeclarationNode() = default;

    virtual void accept(Visitor& visitor) override;

    /// This list of all the parameters to the function
    ///
    /// A vector of shared_ptr is used as usage of these parameters will
    /// eventually be linked back to their delarations here. As such a
    /// unique_ptr would not be applicable.
    std::vector<std::shared_ptr<ParameterNode>> parameters;
    /// The statement block serving as the body of the function
    std::unique_ptr<CompoundStatementNode> function_body;
};

/// Variable Declaration Node
///
/// This node type represents a variable declaration.
struct VariableDeclarationNode : DeclarationNode {
    /// Constructs a Variable Declaration Node
    ///
    /// \param type the type of the variable
    /// \param identifier the identifier for the variable
    /// \param loc the location where the variable is declared
    VariableDeclarationNode(
        value_type type,
        std::string identifier,
        location loc);

    virtual ~VariableDeclarationNode() = default;

    virtual void accept(Visitor& visitor) override;
};

/// Array Declaration Node
///
/// This node type represents the declaration of an array variable.
struct ArrayDeclarationNode : VariableDeclarationNode {
    /// Constructs an Array Declaration Node
    ///
    /// \param type the type of the array
    /// \param identifier the identifier for the array
    /// \param size the length of the array
    /// \param loc the location where the array is declared
    ArrayDeclarationNode(
        value_type type,
        std::string identifier,
        int size,
        location loc);

    virtual ~ArrayDeclarationNode() = default;

    virtual void accept(Visitor& visitor) override;

    /// The size of the array
    ///
    /// Should be positive. This is to be checked at semantic analysis as the
    /// grammar allows any integer literal as the size durring parsing.
    int size;
};

/// Parameter Declaration Node
///
/// This node type represents a parameter in a function declaration.
struct ParameterNode : DeclarationNode {
    /// Constructs a Parameter Declaration Node
    ///
    /// \param type the type of the parameter
    /// \param identifier the name of the parameter
    /// \param loc the location where the parameter is declared
    ParameterNode(value_type type, std::string identifier, location loc);

    virtual ~ParameterNode() = default;

    virtual void accept(Visitor& visitor) override;
};

/***********************************************************************/

/// Variable Expression Node
///
/// The node represents a variable used in an expression
struct variable_expression_node : expression_node {
    /// Constructs a Variable Expression Node
    ///
    /// \param identifier the name of the variable referenced
    /// \param loc the location of the reference in the source code
    variable_expression_node(std::string identifier, location loc);

    virtual ~variable_expression_node() = default;

    virtual void accept(Visitor& visitor) override;

    /// The identifier of the variable being referenced
    std::string identifier;
};

/// Assignment Expression Node
///
/// This node represents an assignment expression of the form `var = <expr>`.
struct assignment_expression_node : expression_node {
    /// Constructs an Assignment Expression Node
    ///
    /// \param var the variable being assigned
    /// \param expr the expression to be assigned to the variable
    /// \param loc the location of the assignment in the source code
    assignment_expression_node(
        std::unique_ptr<variable_expression_node> var,
        std::unique_ptr<expression_node> expr,
        location loc);

    virtual ~assignment_expression_node() = default;

    virtual void accept(Visitor& visitor) override;

    /// The variable being assigned to
    std::unique_ptr<variable_expression_node> variable;
    /// The expression which will be assigned to the variable
    std::unique_ptr<expression_node> expression;
};

/// Subscript Expression Node
///
/// A node representing a subscripted variable
struct subscript_expression_node : variable_expression_node {
    /// Constructs a Subscript Expression Node
    ///
    /// \param identifier the identifier of the variable being subscripted
    /// \param index the expression indexing the variable
    /// \param loc the location of the subscript expression in the source code
    subscript_expression_node(
        std::string identifier,
        std::unique_ptr<expression_node> index,
        location loc);

    virtual ~subscript_expression_node() = default;

    virtual void accept(Visitor& visitor) override;

    /// The expression indexing the variable
    std::unique_ptr<expression_node> index;
};

/// Function Call Expression Node
///
/// A node representing a function call expression
struct call_expression_node : expression_node {
    /// Constructs a Function Call Expression Node
    ///
    /// \param identifier the name of the function being called
    /// \param args the arguments to the function call
    /// \param loc the location of the function call in the source code
    call_expression_node(
        std::string identifier,
        std::vector<std::unique_ptr<expression_node>> args,
        location loc);

    virtual ~call_expression_node() = default;

    virtual void accept(Visitor& visitor) override;

    /// The name of the function being called
    std::string identifier;
    /// The list of arguments being passed to the function
    std::vector<std::unique_ptr<expression_node>> arguments;
};

/// Additive Expression Node
///
/// Represents an additive expression in the source code.
struct additive_expression_node : expression_node {
    /// Constructs an Additive Expression Node
    ///
    /// \param operation the additive operation to be applied
    /// \param lhs the left hand side expression of the binary operation
    /// \param rhs the right hand side expression of the binary operation
    additive_expression_node(
        add_op operation,
        std::unique_ptr<expression_node> lhs,
        std::unique_ptr<expression_node> rhs,
        location loc);

    virtual ~additive_expression_node() = default;

    virtual void accept(Visitor& visitor) override;

    /// The additive operation to be applied
    add_op operation;
    /// The left hand side of the expression
    std::unique_ptr<expression_node> left;
    /// The right hand side of the expression
    std::unique_ptr<expression_node> right;
};

/// Multiplicative Expression Node
///
/// Represents a multiplicative expression in the source code.
struct multiplicative_expression_node : expression_node {
    /// Constructs a Multiplicative Expression Node
    ///
    /// \param operation the multiplicative operation to be applied
    /// \param lhs the left hand side expression of the binary operation
    /// \param rhs the right hand side expression of the binary operation
    multiplicative_expression_node(
        mul_op operation,
        std::unique_ptr<expression_node> lhs,
        std::unique_ptr<expression_node> rhs,
        location loc);

    virtual ~multiplicative_expression_node() = default;

    virtual void accept(Visitor& visitor) override;

    /// The multiplicative expression to be applied
    mul_op operation;
    /// The left hand side of the expression
    std::unique_ptr<expression_node> left;
    /// The right hand side of the expression
    std::unique_ptr<expression_node> right;
};

/// Relational Expression Node
///
/// Represents a relation expression in the source code.
struct relational_expression_node : expression_node {
    /// Constructs a Relational Expression Node
    ///
    /// \param operation the comparison to be made
    /// \param lhs the left hand side expression of the comparison
    /// \param rhs the right hand side expression of the comparison
    /// \param loc the location of the relational expression in the source code
    relational_expression_node(
        rel_op operation,
        std::unique_ptr<expression_node> lhs,
        std::unique_ptr<expression_node> rhs,
        location loc);

    virtual ~relational_expression_node() = default;

    virtual void accept(Visitor& visitor) override;

    /// The comparison to be made
    rel_op operation;
    /// The left hand side of the comparison expression
    std::unique_ptr<expression_node> left;
    /// The right hand side of the comparison expression
    std::unique_ptr<expression_node> right;
};

// Future Work: parse unary expressions

/// Integer Literal Expression Node
///
/// Represents an integer literal in an expression
struct integer_literal_expression_node : expression_node {
    /// Constructs an Integer Literal Expression Node
    ///
    /// \param value the value of the literal
    /// \param loc the location of the literal in the source code
    integer_literal_expression_node(int value, location loc);

    virtual ~integer_literal_expression_node() = default;

    virtual void accept(Visitor& visitor) override;

    /// The value of the integer literal
    int value;
};

/***********************************************************************/

#endif
