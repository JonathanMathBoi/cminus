#include "AST.hpp"
#include "MiscUtils.hpp"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using std::shared_ptr;
using std::unique_ptr;
using std::vector;

/***********************************************************************/
// Program Root Node

// Uses fixed args for node constructor as the program node is always the entire
// source file
ProgramNode::ProgramNode(vector<shared_ptr<DeclarationNode>> declarations)
    : Node {location {1, 1}}, declarations {declarations} {}

void ProgramNode::accept(Visitor& visitor) {
    visitor.visit(*this);
}

/***********************************************************************/
// Declaration Nodes

DeclarationNode::DeclarationNode(
    value_type type,
    std::string identifier,
    location loc)
    : Node {loc}, type {type}, identifier {identifier} {}

void DeclarationNode::accept(Visitor& visitor) {
    visitor.visit(*this);
}

FunctionDeclarationNode::FunctionDeclarationNode(
    value_type type,
    std::string identifier,
    vector<shared_ptr<ParameterNode>> params,
    unique_ptr<CompoundStatementNode> body,
    location loc)
    : DeclarationNode {type, identifier, loc}
    , parameters {params}
    , function_body {std::move(body)} {}

void FunctionDeclarationNode::accept(Visitor& visitor) {
    visitor.visit(*this);
}

VariableDeclarationNode::VariableDeclarationNode(
    value_type type,
    std::string identifier,
    location loc)
    : DeclarationNode {type, identifier, loc} {}

void VariableDeclarationNode::accept(Visitor& visitor) {
    visitor.visit(*this);
}

ArrayDeclarationNode::ArrayDeclarationNode(
    value_type type,
    std::string identifier,
    int size,
    location loc)
    : VariableDeclarationNode {type, identifier, loc}, size {size} {}

void ArrayDeclarationNode::accept(Visitor& visitor) {
    visitor.visit(*this);
}

ParameterNode::ParameterNode(
    value_type type,
    std::string identifier,
    location loc)
    : DeclarationNode {type, identifier, loc} {}

void ParameterNode::accept(Visitor& visitor) {
    visitor.visit(*this);
}

/***********************************************************************/
// Expression Nodes

AssignmentExpressionNode::AssignmentExpressionNode(
    unique_ptr<VariableExpressionNode> var,
    unique_ptr<ExpressionNode> expr,
    location loc)
    : ExpressionNode {loc}
    , variable {std::move(var)}
    , expression {std::move(expr)} {}

void AssignmentExpressionNode::accept(Visitor& visitor) {
    visitor.visit(*this);
}

VariableExpressionNode::VariableExpressionNode(
    std::string identifier,
    location loc)
    : ExpressionNode {loc}, identifier {identifier} {}

void VariableExpressionNode::accept(Visitor& visitor) {
    visitor.visit(*this);
}

SubscriptExpressionNode::SubscriptExpressionNode(
    std::string identifier,
    unique_ptr<ExpressionNode> index,
    location loc)
    : VariableExpressionNode {identifier, loc}, index {std::move(index)} {}

void SubscriptExpressionNode::accept(Visitor& visitor) {
    visitor.visit(*this);
}

CallExpressionNode::CallExpressionNode(
    std::string identifier,
    vector<unique_ptr<ExpressionNode>> args,
    location loc)
    : ExpressionNode {loc}
    , identifier {identifier}
    , arguments {std::move(args)} {}

void CallExpressionNode::accept(Visitor& visitor) {
    visitor.visit(*this);
}

AdditiveExpressionNode::AdditiveExpressionNode(
    add_op operation,
    unique_ptr<ExpressionNode> lhs,
    unique_ptr<ExpressionNode> rhs,
    location loc)
    : ExpressionNode {loc}
    , operation {operation}
    , left {std::move(lhs)}
    , right {std::move(rhs)} {}

void AdditiveExpressionNode::accept(Visitor& visitor) {
    visitor.visit(*this);
}

MultiplicativeExpressionNode::MultiplicativeExpressionNode(
    mul_op operation,
    unique_ptr<ExpressionNode> lhs,
    unique_ptr<ExpressionNode> rhs,
    location)
    : ExpressionNode {loc}
    , operation {operation}
    , left {std::move(lhs)}
    , right {std::move(rhs)} {}

void MultiplicativeExpressionNode::accept(Visitor& visitor) {
    visitor.visit(*this);
}

relational_expression_node::relational_expression_node(
    rel_op operation,
    unique_ptr<ExpressionNode> lhs,
    unique_ptr<ExpressionNode> rhs,
    location loc)
    : ExpressionNode {loc}
    , operation {operation}
    , left {std::move(lhs)}
    , right {std::move(rhs)} {}

void relational_expression_node::accept(Visitor& visitor) {
    visitor.visit(*this);
}

integer_literal_expression_node::integer_literal_expression_node(
    int value,
    location loc)
    : ExpressionNode {loc}, value {value} {}

void integer_literal_expression_node::accept(Visitor& visitor) {
    visitor.visit(*this);
}

/***********************************************************************/
// Statement Nodes

CompoundStatementNode::CompoundStatementNode(
    vector<shared_ptr<VariableDeclarationNode>> decls,
    vector<unique_ptr<StatementNode>> stmts,
    location loc)
    : StatementNode {loc}, local_decls {decls}, statements {std::move(stmts)} {}

void CompoundStatementNode::accept(Visitor& visitor) {
    visitor.visit(*this);
}

IfStatementNode::IfStatementNode(
    unique_ptr<ExpressionNode> condition,
    unique_ptr<StatementNode> then_stmt,
    std::optional<unique_ptr<StatementNode>> else_stmt,
    location loc)
    : StatementNode {loc}
    , condition {std::move(condition)}
    , then_stmt {std::move(then_stmt)}
    , else_stmt {std::move(else_stmt)} {}

IfStatementNode::IfStatementNode(
    unique_ptr<ExpressionNode> condition,
    unique_ptr<StatementNode> then_stmt,
    location loc)
    : StatementNode {loc}
    , condition {std::move(condition)}
    , then_stmt {std::move(then_stmt)}
    , else_stmt {std::nullopt} {}

void IfStatementNode::accept(Visitor& visitor) {
    visitor.visit(*this);
}

WhileStatementNode::WhileStatementNode(
    unique_ptr<ExpressionNode> condition,
    unique_ptr<StatementNode> stmt,
    location loc)
    : StatementNode {loc}
    , condition {std::move(condition)}
    , body {std::move(stmt)} {}

void WhileStatementNode::accept(Visitor& visitor) {
    visitor.visit(*this);
}

ReturnStatementNode::ReturnStatementNode(
    std::optional<unique_ptr<ExpressionNode>> expr,
    location loc)
    : StatementNode {loc}, expression {std::move(expr)} {}

ReturnStatementNode::ReturnStatementNode(location loc)
    : StatementNode {loc}, expression {std::nullopt} {}

void ReturnStatementNode::accept(Visitor& visitor) {
    visitor.visit(*this);
}

ExpressionStatementNode::ExpressionStatementNode(
    std::optional<unique_ptr<ExpressionNode>> expr,
    location loc)
    : StatementNode {loc}, expr {std::move(expr)} {}

ExpressionStatementNode::ExpressionStatementNode(location loc)
    : StatementNode {loc}, expr {std::nullopt} {}

void ExpressionStatementNode::accept(Visitor& visitor) {
    visitor.visit(*this);
}
/***********************************************************************/
