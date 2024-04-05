#ifndef AST_HPP
#define AST_HPP

/***********************************************************************/

#include "../MiscUtils.hpp"

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>

#include <memory>
#include <optional>
#include <ostream>
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
struct ReturnStatementNode;
struct ExpressionStatementNode;

struct ExpressionNode;
struct AssignmentExpressionNode;
struct VariableExpressionNode;
struct SubscriptExpressionNode;
struct CallExpressionNode;
struct AdditiveExpressionNode;
struct MultiplicativeExpressionNode;
struct RelationalExpressionNode;
struct IntegerLiteralExpressionNode;
struct FloatLiteralExpressionNode;
struct BoolLiteralExpressionNode;

struct SymbolUseNode;

/***********************************************************************/

/// A primative type of C-
enum class PrimitiveType { Void, Int, Float, Bool };

/// A type kind in C-
enum class TypeKind { Primitive, Array };

/// A C- type
struct Type {
    /// The kind of the type
    TypeKind kind;
    /// The underlying primitive type
    PrimitiveType base;

    bool operator==(Type const&) const& = default;
    /// \brief Gets the LLVM type corresponding to this type
    ///
    /// \return the primative for primatives, and a pointer for arrays.
    ///
    /// \note When a sized array is needed llvmVarType() should be called.
    llvm::Type* llvmParamType(llvm::LLVMContext& C) const;
    /// \brief Gets the LLVM type corresponding to this type
    ///
    /// \param C the LLVMContext for the type
    /// \param arraySize the size of the array
    ///
    /// \returns the primative type for primatives, and a sized array for arrays
    llvm::Type* llvmVarType(
        llvm::LLVMContext& C,
        std::optional<int> arraySize = std::nullopt);
};

namespace Types {
/// Void primative type
static const Type Void {TypeKind::Primitive, PrimitiveType::Void};
/// Int primative type
static const Type Int {TypeKind::Primitive, PrimitiveType::Int};
/// Float primative type
static const Type Float {TypeKind::Primitive, PrimitiveType::Float};
/// Bool primative type
static const Type Bool {TypeKind::Primitive, PrimitiveType::Bool};
}  // namespace Types

/// They type of a C- declaration
struct DeclarationType {
    Type type;
    bool is_function;
};

std::ostream& operator<<(std::ostream& os, PrimitiveType const& prim_type);
std::ostream& operator<<(std::ostream& os, Type const& type);

/***********************************************************************/

enum class AdditiveOp { PLUS, MINUS };
std::ostream& operator<<(std::ostream& os, AdditiveOp const& add_op);

enum class MultiplicativeOp { TIMES, DIVIDE };
std::ostream& operator<<(std::ostream& os, MultiplicativeOp const& mul_op);

enum class RelationalOp { LT, LTE, GT, GTE, EQ, NEQ };
std::ostream& operator<<(std::ostream& os, RelationalOp const& rel_op);

enum class UnaryOp { INCREMENT, DECREMENT };
std::ostream& operator<<(std::ostream& os, UnaryOp const& unary_op);

/***********************************************************************/

/// A vector of all the compiler builtin functions
///
/// Owned here as they are not part of a users source tree.
extern std::vector<std::shared_ptr<FunctionDeclarationNode>> g_builtins;

/***********************************************************************/

class Visitor {
public:
    virtual void visit(ProgramNode& node) = 0;

    virtual void visit(FunctionDeclarationNode& node) = 0;
    virtual void visit(VariableDeclarationNode& node) = 0;
    virtual void visit(ArrayDeclarationNode& node) = 0;
    virtual void visit(ParameterNode& node) = 0;

    virtual void visit(CompoundStatementNode& node) = 0;
    virtual void visit(IfStatementNode& node) = 0;
    virtual void visit(WhileStatementNode& node) = 0;
    // Not parsing for statement yet
    // virtual void visit(for_statement_node& node) = 0;
    virtual void visit(ReturnStatementNode& node) = 0;
    virtual void visit(ExpressionStatementNode& node) = 0;

    virtual void visit(AssignmentExpressionNode& node) = 0;
    virtual void visit(VariableExpressionNode& node) = 0;
    virtual void visit(SubscriptExpressionNode& node) = 0;
    virtual void visit(CallExpressionNode& node) = 0;
    virtual void visit(AdditiveExpressionNode& node) = 0;
    virtual void visit(MultiplicativeExpressionNode& node) = 0;
    virtual void visit(RelationalExpressionNode& node) = 0;
    // Not parsing increment and decrement yet
    // virtual void visit(unary_expression_node& node) = 0;
    virtual void visit(IntegerLiteralExpressionNode& node) = 0;
    virtual void visit(FloatLiteralExpressionNode& node) = 0;
    virtual void visit(BoolLiteralExpressionNode& node) = 0;
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
    Node(Location loc) : loc {loc} {}

    virtual ~Node() = default;

    /// Accepts a visitor to visit this node
    ///
    /// This should always be a one line function calling the correct visit
    /// overload with the `this` parameter.
    virtual void accept(Visitor& visitor) = 0;

    /// The location of the construct in the source code
    Location loc;
    /// \brief The LLVM IR value for the node
    ///
    /// Only used in codegen pass. Null until codegen visit.
    llvm::Value* ir_value;
};

/// Abstract Declaration Node
///
/// This node type serves as the base for all declaration nodes to inherit from.
///
/// When a smart pointer is needed to a declaration node, a `shared_ptr` is
/// likely the best choice, as these nodes eventually need to be pointed to at
/// both their parent and by all their references.
struct DeclarationNode : virtual Node {
    /// Constructs a Declaration Node
    ///
    /// \param type the type for the declared construct
    /// \param identifier the identifier for the declared construct
    /// \param loc the location where the declaration begins
    DeclarationNode(DeclarationType type, std::string identifier)
        : type {type}, identifier {identifier} {}

    virtual ~DeclarationNode() = default;

    virtual void accept(Visitor& visitor) = 0;

    /// The type of the declared construct
    DeclarationType type;
    /// The identifier of the declared construct
    std::string identifier;
    /// The nest level of the declaration
    ///
    /// An optional is used as this isn't set until the symbol visitor pass
    std::optional<unsigned> nest_level;
};

/// Abstract Symbol Use Node
///
/// This node is an abstract node to represent constructs that refer back to a
/// previous declaration.
struct SymbolUseNode : virtual Node {
    /// Construct a Symbol Use Node
    ///
    /// \param identifier the name of the construct referenced
    /// \param loc the location where the reference begins
    SymbolUseNode(std::string identifier) : identifier {identifier} {}

    virtual ~SymbolUseNode() = default;

    virtual void accept(Visitor& visitor) = 0;

    /// The name being refered to
    std::string identifier;
    /// The referent of this identifier
    ///
    /// This value is null until the symbol visitor pass
    std::shared_ptr<DeclarationNode> referent;
};

/// Abstract Expression Node
///
/// This node type serves as the base for all types of expression nodes to
/// derive from.
struct ExpressionNode : virtual Node {
    /// Constructs an Expression Node
    ///
    /// \param loc the location of the expression in the source code
    ExpressionNode() {}

    virtual ~ExpressionNode() = default;

    virtual void accept(Visitor& visitor) = 0;

    /// The type of the expression
    ///
    /// An optional is used as this isn't calculated until the semantic analysis
    /// pass.
    std::optional<Type> type;
};

/// Abstract Statement Node
///
/// This node type serves as the base for all types of statement nodes to derive
/// from.
struct StatementNode : virtual Node {
    /// Constructs a Statement Node
    ///
    /// \param loc the location of the statement in the source code
    StatementNode() {}

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
        Location loc);

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
    /// Whether or not this is the body of a function
    ///
    /// Defaults to false, should be set true by function parser call
    bool is_function_body {false};
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
        std::unique_ptr<ExpressionNode> condition,
        std::unique_ptr<StatementNode> then_stmt,
        std::unique_ptr<StatementNode> else_stmt,
        Location loc);

    /// Constructs an If Statement Node
    ///
    /// Constructs an If Statement with no attached else block.
    ///
    /// \param condition the conditional expression in the if
    /// \param then_stmt the then statement of the if statement
    /// \param loc the location of the start of the if statement in the source
    ///            code
    IfStatementNode(
        std::unique_ptr<ExpressionNode> condition,
        std::unique_ptr<StatementNode> then_stmt,
        Location loc);

    virtual ~IfStatementNode() = default;

    virtual void accept(Visitor& visitor) override;

    /// The conditional expression for the if statement
    std::unique_ptr<ExpressionNode> condition;
    /// The then statement for the if statement
    std::unique_ptr<StatementNode> then_stmt;
    /// An optional else statement attached to the if statement
    ///
    /// This is null if there is no else statement
    std::unique_ptr<StatementNode> else_stmt;
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
        std::unique_ptr<ExpressionNode> condition,
        std::unique_ptr<StatementNode> stmt,
        Location loc);

    virtual ~WhileStatementNode() = default;

    virtual void accept(Visitor& visitor) override;

    /// The condition for execution of the while loop
    std::unique_ptr<ExpressionNode> condition;
    /// The body of the while loop
    std::unique_ptr<StatementNode> body;
};

// Future Work: for_statement_node

/// Return Statement Node
///
/// The node for a return statement.
struct ReturnStatementNode : StatementNode {
    /// Constructs a Return Statement Node
    ///
    /// \param expr the expression to be returned
    /// \param loc the location of the return statement in the source code
    ReturnStatementNode(std::unique_ptr<ExpressionNode> expr, Location loc);

    /// Constructs a Return Statement Node
    ///
    /// Constructs a Return Statement Node with no associated returned
    /// expression.
    ///
    /// \param loc the location of the return statement in the source code
    ReturnStatementNode(Location loc);

    virtual ~ReturnStatementNode() = default;

    virtual void accept(Visitor& visitor) override;

    /// An option expression to be returned
    ///
    /// If there is no returned expression, this is a nullptr
    std::unique_ptr<ExpressionNode> expression;
};

/// Expression Statement Node
///
/// The node for any semicolon delimited expression statement.
struct ExpressionStatementNode : StatementNode {
    /// Constructs an Expression Statement
    ///
    /// \param expr the expression to be evaluated
    /// \param loc the location of the expression in the source code.
    ExpressionStatementNode(std::unique_ptr<ExpressionNode> expr, Location loc);

    /// Constructs an Expression Statement
    ///
    /// Constructs an Expression Statement with no associated expression. (i.e.
    /// the statement `;`.)
    ///
    /// \param loc the location of the expression in the source code.
    ExpressionStatementNode(Location loc);

    virtual ~ExpressionStatementNode() = default;

    virtual void accept(Visitor& visitor) override;

    /// The optional expression to be evaluated
    ///
    /// If there is no expression, this is a nullptr
    std::unique_ptr<ExpressionNode> expr;
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
        DeclarationType type,
        std::string identifier,
        std::vector<std::shared_ptr<ParameterNode>> params,
        std::unique_ptr<CompoundStatementNode> body,
        Location loc);

    virtual ~FunctionDeclarationNode() = default;

    virtual void accept(Visitor& visitor) override;

    /// This list of all the parameters to the function
    ///
    /// A vector of shared_ptr is used as usage of these parameters will
    /// eventually be linked back to their delarations here. As such a
    /// unique_ptr would not be applicable.
    std::vector<std::shared_ptr<ParameterNode>> parameters;
    /// The statement block serving as the body of the function
    ///
    /// This is a nullptr for compiler built-ins. Should be a valid pointer for
    /// all other functions.
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
        DeclarationType type,
        std::string identifier,
        Location loc);

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
        DeclarationType type,
        std::string identifier,
        int size,
        Location loc);

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
    ParameterNode(DeclarationType type, std::string identifier, Location loc);

    virtual ~ParameterNode() = default;

    virtual void accept(Visitor& visitor) override;
};

/***********************************************************************/

/// Variable Expression Node
///
/// The node represents a variable used in an expression
struct VariableExpressionNode
    : ExpressionNode
    , SymbolUseNode {
    /// Constructs a Variable Expression Node
    ///
    /// \param identifier the name of the variable referenced
    /// \param loc the location of the reference in the source code
    VariableExpressionNode(std::string identifier, Location loc);

    virtual ~VariableExpressionNode() = default;

    virtual void accept(Visitor& visitor) override;
};

/// Assignment Expression Node
///
/// This node represents an assignment expression of the form `var = <expr>`.
struct AssignmentExpressionNode : ExpressionNode {
    /// Constructs an Assignment Expression Node
    ///
    /// \param var the variable being assigned
    /// \param expr the expression to be assigned to the variable
    /// \param loc the location of the assignment in the source code
    AssignmentExpressionNode(
        std::unique_ptr<VariableExpressionNode> var,
        std::unique_ptr<ExpressionNode> expr,
        Location loc);

    virtual ~AssignmentExpressionNode() = default;

    virtual void accept(Visitor& visitor) override;

    /// The variable being assigned to
    std::unique_ptr<VariableExpressionNode> variable;
    /// The expression which will be assigned to the variable
    std::unique_ptr<ExpressionNode> expression;
};

/// Subscript Expression Node
///
/// A node representing a subscripted variable
struct SubscriptExpressionNode : VariableExpressionNode {
    /// Constructs a Subscript Expression Node
    ///
    /// \param identifier the identifier of the variable being subscripted
    /// \param index the expression indexing the variable
    /// \param loc the location of the subscript expression in the source code
    SubscriptExpressionNode(
        std::string identifier,
        std::unique_ptr<ExpressionNode> index,
        Location loc);

    virtual ~SubscriptExpressionNode() = default;

    virtual void accept(Visitor& visitor) override;

    /// The expression indexing the variable
    std::unique_ptr<ExpressionNode> index;
};

/// Function Call Expression Node
///
/// A node representing a function call expression
struct CallExpressionNode
    : ExpressionNode
    , SymbolUseNode {
    /// Constructs a Function Call Expression Node
    ///
    /// \param identifier the name of the function being called
    /// \param args the arguments to the function call
    /// \param loc the location of the function call in the source code
    CallExpressionNode(
        std::string identifier,
        std::vector<std::unique_ptr<ExpressionNode>> args,
        Location loc);

    virtual ~CallExpressionNode() = default;

    virtual void accept(Visitor& visitor) override;

    /// The list of arguments being passed to the function
    std::vector<std::unique_ptr<ExpressionNode>> arguments;
};

/// Additive Expression Node
///
/// Represents an additive expression in the source code.
struct AdditiveExpressionNode : ExpressionNode {
    /// Constructs an Additive Expression Node
    ///
    /// \param operation the additive operation to be applied
    /// \param lhs the left hand side expression of the binary operation
    /// \param rhs the right hand side expression of the binary operation
    AdditiveExpressionNode(
        AdditiveOp operation,
        std::unique_ptr<ExpressionNode> lhs,
        std::unique_ptr<ExpressionNode> rhs,
        Location loc);

    virtual ~AdditiveExpressionNode() = default;

    virtual void accept(Visitor& visitor) override;

    /// The additive operation to be applied
    AdditiveOp operation;
    /// The left hand side of the expression
    std::unique_ptr<ExpressionNode> left;
    /// The right hand side of the expression
    std::unique_ptr<ExpressionNode> right;
};

/// Multiplicative Expression Node
///
/// Represents a multiplicative expression in the source code.
struct MultiplicativeExpressionNode : ExpressionNode {
    /// Constructs a Multiplicative Expression Node
    ///
    /// \param operation the multiplicative operation to be applied
    /// \param lhs the left hand side expression of the binary operation
    /// \param rhs the right hand side expression of the binary operation
    MultiplicativeExpressionNode(
        MultiplicativeOp operation,
        std::unique_ptr<ExpressionNode> lhs,
        std::unique_ptr<ExpressionNode> rhs,
        Location loc);

    virtual ~MultiplicativeExpressionNode() = default;

    virtual void accept(Visitor& visitor) override;

    /// The multiplicative expression to be applied
    MultiplicativeOp operation;
    /// The left hand side of the expression
    std::unique_ptr<ExpressionNode> left;
    /// The right hand side of the expression
    std::unique_ptr<ExpressionNode> right;
};

/// Relational Expression Node
///
/// Represents a relation expression in the source code.
struct RelationalExpressionNode : ExpressionNode {
    /// Constructs a Relational Expression Node
    ///
    /// \param operation the comparison to be made
    /// \param lhs the left hand side expression of the comparison
    /// \param rhs the right hand side expression of the comparison
    /// \param loc the location of the relational expression in the source code
    RelationalExpressionNode(
        RelationalOp operation,
        std::unique_ptr<ExpressionNode> lhs,
        std::unique_ptr<ExpressionNode> rhs,
        Location loc);

    virtual ~RelationalExpressionNode() = default;

    virtual void accept(Visitor& visitor) override;

    /// The comparison to be made
    RelationalOp operation;
    /// The left hand side of the comparison expression
    std::unique_ptr<ExpressionNode> left;
    /// The right hand side of the comparison expression
    std::unique_ptr<ExpressionNode> right;
};

// Future Work: parse unary expressions

/// Integer Literal Expression Node
///
/// Represents an integer literal in an expression
struct IntegerLiteralExpressionNode : ExpressionNode {
    /// Constructs an Integer Literal Expression Node
    ///
    /// \param value the value of the literal
    /// \param loc the location of the literal in the source code
    IntegerLiteralExpressionNode(int value, Location loc);

    virtual ~IntegerLiteralExpressionNode() = default;

    virtual void accept(Visitor& visitor) override;

    /// The value of the integer literal
    int value;
};

/// Float Literal Expression Node
///
/// Represents a float literal
struct FloatLiteralExpressionNode : ExpressionNode {
    /// Constructs a Float Literal Expression Node
    ///
    /// \param value the value of the literal
    /// \param loc the location of the literal in the source code
    FloatLiteralExpressionNode(float value, Location loc);

    virtual ~FloatLiteralExpressionNode() = default;

    virtual void accept(Visitor& visitor) override;

    /// The value of the float literal
    float value;
};

/// Bool Literal Expression Node
///
/// Represents a bool literal
struct BoolLiteralExpressionNode : ExpressionNode {
    /// Constructs a Bool Literal Expression Node
    ///
    /// \param value the value of the bool literal
    /// \param loc the location of the literal in the source code
    BoolLiteralExpressionNode(bool value, Location loc);

    virtual ~BoolLiteralExpressionNode() = default;

    virtual void accept(Visitor& visitor) override;

    /// The value of the bool literal
    bool value;
};

/***********************************************************************/

#endif
