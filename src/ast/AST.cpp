#include "AST.hpp"
#include "../MiscUtils.hpp"

#include <cassert>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using std::make_shared;
using std::shared_ptr;
using std::unique_ptr;
using std::vector;

/***********************************************************************/

vector<shared_ptr<FunctionDeclarationNode>> g_builtins {
    make_shared<FunctionDeclarationNode>(
        DeclarationType {
            Type {TypeKind::Primitive, PrimitiveType::Int},
            /*is_function=*/true},
        "input",
        vector<shared_ptr<ParameterNode>> {},
        nullptr,
        Location {-1, -1}),  // input()
    make_shared<FunctionDeclarationNode>(
        DeclarationType {
            Type {TypeKind::Primitive, PrimitiveType::Void},
            /*is_function=*/true},
        "output",
        vector<shared_ptr<ParameterNode>> {make_shared<ParameterNode>(
            DeclarationType {
                Type {TypeKind::Primitive, PrimitiveType::Int},
                /*is_function=*/false},
            "value",
            Location {-1, -1})},
        nullptr,
        Location {-1, -1})  // output(int)
};

/***********************************************************************/

std::ostream& operator<<(std::ostream& os, PrimitiveType const& prim_type) {
    switch (prim_type) {
    case PrimitiveType::Void:
        os << "void";
        break;
    case PrimitiveType::Int:
        os << "int";
        break;
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, Type const& type) {
    switch (type.kind) {
    case TypeKind::Primitive:
        os << type.base;
        break;
    case TypeKind::Array:
        os << type.base << "[]";
        break;
    }
    return os;
}

/***********************************************************************/
// Program Root Node

// Uses fixed args for node constructor as the program node is always the entire
// source file
ProgramNode::ProgramNode(vector<shared_ptr<DeclarationNode>> declarations)
    : Node {Location {1, 1}}, declarations {declarations} {}

void ProgramNode::accept(Visitor& visitor) {
    visitor.visit(*this);
}

/***********************************************************************/
// Declaration Nodes

FunctionDeclarationNode::FunctionDeclarationNode(
    DeclarationType type,
    std::string identifier,
    vector<shared_ptr<ParameterNode>> params,
    unique_ptr<CompoundStatementNode> body,
    Location loc)
    : Node {loc}
    , DeclarationNode {type, identifier}
    , parameters {params}
    , function_body {std::move(body)} {
    assert(
        type.is_function &&
        "Function declarations should be marked as function");
}

void FunctionDeclarationNode::accept(Visitor& visitor) {
    visitor.visit(*this);
}

VariableDeclarationNode::VariableDeclarationNode(
    DeclarationType type,
    std::string identifier,
    Location loc)
    : Node {loc}, DeclarationNode {type, identifier} {
    assert(
        !type.is_function &&
        "Variable declarations should not be marked as function");
}

void VariableDeclarationNode::accept(Visitor& visitor) {
    visitor.visit(*this);
}

ArrayDeclarationNode::ArrayDeclarationNode(
    DeclarationType type,
    std::string identifier,
    int size,
    Location loc)
    : Node {loc}, VariableDeclarationNode {type, identifier, loc}, size {size} {
    assert(
        !type.is_function &&
        "Array declarations should not be marked as function");
    assert(
        type.type.kind == TypeKind::Array &&
        "Array declarations should have an array type");
}

void ArrayDeclarationNode::accept(Visitor& visitor) {
    visitor.visit(*this);
}

ParameterNode::ParameterNode(
    DeclarationType type,
    std::string identifier,
    Location loc)
    : Node {loc}, DeclarationNode {type, identifier} {
    assert(
        !type.is_function &&
        "Parameter declarations should not be marked as function");
}

void ParameterNode::accept(Visitor& visitor) {
    visitor.visit(*this);
}

/***********************************************************************/
// Expression Nodes

AssignmentExpressionNode::AssignmentExpressionNode(
    unique_ptr<VariableExpressionNode> var,
    unique_ptr<ExpressionNode> expr,
    Location loc)
    : Node {loc}, variable {std::move(var)}, expression {std::move(expr)} {}

void AssignmentExpressionNode::accept(Visitor& visitor) {
    visitor.visit(*this);
}

VariableExpressionNode::VariableExpressionNode(
    std::string identifier,
    Location loc)
    : Node {loc}, SymbolUseNode {identifier} {}

void VariableExpressionNode::accept(Visitor& visitor) {
    visitor.visit(*this);
}

SubscriptExpressionNode::SubscriptExpressionNode(
    std::string identifier,
    unique_ptr<ExpressionNode> index,
    Location loc)
    : Node {loc}
    , VariableExpressionNode {identifier, loc}
    , index {std::move(index)} {}

void SubscriptExpressionNode::accept(Visitor& visitor) {
    visitor.visit(*this);
}

CallExpressionNode::CallExpressionNode(
    std::string identifier,
    vector<unique_ptr<ExpressionNode>> args,
    Location loc)
    : Node {loc}, SymbolUseNode {identifier}, arguments {std::move(args)} {}

void CallExpressionNode::accept(Visitor& visitor) {
    visitor.visit(*this);
}

AdditiveExpressionNode::AdditiveExpressionNode(
    AdditiveOp operation,
    unique_ptr<ExpressionNode> lhs,
    unique_ptr<ExpressionNode> rhs,
    Location loc)
    : Node {loc}
    , operation {operation}
    , left {std::move(lhs)}
    , right {std::move(rhs)} {}

void AdditiveExpressionNode::accept(Visitor& visitor) {
    visitor.visit(*this);
}

MultiplicativeExpressionNode::MultiplicativeExpressionNode(
    MultiplicativeOp operation,
    unique_ptr<ExpressionNode> lhs,
    unique_ptr<ExpressionNode> rhs,
    Location loc)
    : Node {loc}
    , operation {operation}
    , left {std::move(lhs)}
    , right {std::move(rhs)} {}

void MultiplicativeExpressionNode::accept(Visitor& visitor) {
    visitor.visit(*this);
}

RelationalExpressionNode::RelationalExpressionNode(
    RelationalOp operation,
    unique_ptr<ExpressionNode> lhs,
    unique_ptr<ExpressionNode> rhs,
    Location loc)
    : Node {loc}
    , operation {operation}
    , left {std::move(lhs)}
    , right {std::move(rhs)} {}

void RelationalExpressionNode::accept(Visitor& visitor) {
    visitor.visit(*this);
}

IntegerLiteralExpressionNode::IntegerLiteralExpressionNode(
    int value,
    Location loc)
    : Node {loc}, value {value} {}

void IntegerLiteralExpressionNode::accept(Visitor& visitor) {
    visitor.visit(*this);
}

/***********************************************************************/
// Statement Nodes

CompoundStatementNode::CompoundStatementNode(
    vector<shared_ptr<VariableDeclarationNode>> decls,
    vector<unique_ptr<StatementNode>> stmts,
    Location loc)
    : Node {loc}, local_decls {decls}, statements {std::move(stmts)} {}

void CompoundStatementNode::accept(Visitor& visitor) {
    visitor.visit(*this);
}

IfStatementNode::IfStatementNode(
    unique_ptr<ExpressionNode> condition,
    unique_ptr<StatementNode> then_stmt,
    std::optional<unique_ptr<StatementNode>> else_stmt,
    Location loc)
    : Node {loc}
    , condition {std::move(condition)}
    , then_stmt {std::move(then_stmt)}
    , else_stmt {std::move(else_stmt)} {}

IfStatementNode::IfStatementNode(
    unique_ptr<ExpressionNode> condition,
    unique_ptr<StatementNode> then_stmt,
    Location loc)
    : Node {loc}
    , condition {std::move(condition)}
    , then_stmt {std::move(then_stmt)}
    , else_stmt {std::nullopt} {}

void IfStatementNode::accept(Visitor& visitor) {
    visitor.visit(*this);
}

WhileStatementNode::WhileStatementNode(
    unique_ptr<ExpressionNode> condition,
    unique_ptr<StatementNode> stmt,
    Location loc)
    : Node {loc}, condition {std::move(condition)}, body {std::move(stmt)} {}

void WhileStatementNode::accept(Visitor& visitor) {
    visitor.visit(*this);
}

ReturnStatementNode::ReturnStatementNode(
    std::optional<unique_ptr<ExpressionNode>> expr,
    Location loc)
    : Node {loc}, expression {std::move(expr)} {}

ReturnStatementNode::ReturnStatementNode(Location loc)
    : Node {loc}, expression {std::nullopt} {}

void ReturnStatementNode::accept(Visitor& visitor) {
    visitor.visit(*this);
}

ExpressionStatementNode::ExpressionStatementNode(
    std::optional<unique_ptr<ExpressionNode>> expr,
    Location loc)
    : Node {loc}, expr {std::move(expr)} {}

ExpressionStatementNode::ExpressionStatementNode(Location loc)
    : Node {loc}, expr {std::nullopt} {}

void ExpressionStatementNode::accept(Visitor& visitor) {
    visitor.visit(*this);
}

/***********************************************************************/
