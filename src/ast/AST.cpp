#include "AST.hpp"
#include "../MiscUtils.hpp"

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>

#include <cassert>
#include <memory>
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
        DeclarationType {Types::Int, /*is_function=*/true},
        "input",
        vector<shared_ptr<ParameterNode>> {},
        nullptr,
        Location {-1, -1}),  // input()
    make_shared<FunctionDeclarationNode>(
        DeclarationType {Types::Void, /*is_function=*/true},
        "output",
        vector<shared_ptr<ParameterNode>> {make_shared<ParameterNode>(
            DeclarationType {Types::Int, /*is_function=*/false},
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
    case PrimitiveType::Float:
        os << "float";
        break;
    case PrimitiveType::Bool:
        os << "bool";
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

llvm::Type* Type::llvmType(llvm::LLVMContext& C) const {
    if (kind != TypeKind::Primitive) {
        return llvm::PointerType::getUnqual(C);
    }

    // kind must be Primative at this point
    switch (base) {
    case PrimitiveType::Void:
        return llvm::Type::getVoidTy(C);
    case PrimitiveType::Int:
        return llvm::Type::getInt32Ty(C);
    case PrimitiveType::Float:
        return llvm::Type::getFloatTy(C);
    case PrimitiveType::Bool:
        return llvm::Type::getInt1Ty(C);
    }
}

llvm::Type* Type::llvmSizedType(
    llvm::LLVMContext& C,
    std::optional<int> arraySize) const {
    if (kind == TypeKind::Array) {
        assert(arraySize && "LLVM Array types must be sized");
        llvm::Type* elem_type;
        switch (base) {
        case PrimitiveType::Void:
            elem_type = llvm::Type::getVoidTy(C);
        case PrimitiveType::Int:
            elem_type = llvm::Type::getInt32Ty(C);
        case PrimitiveType::Float:
            elem_type = llvm::Type::getFloatTy(C);
        case PrimitiveType::Bool:
            elem_type = llvm::Type::getInt1Ty(C);
        }

        return llvm::ArrayType::get(elem_type, *arraySize);
    }

    switch (base) {
    case PrimitiveType::Void:
        return llvm::Type::getVoidTy(C);
    case PrimitiveType::Int:
        return llvm::Type::getInt32Ty(C);
    case PrimitiveType::Float:
        return llvm::Type::getFloatTy(C);
    case PrimitiveType::Bool:
        return llvm::Type::getInt1Ty(C);
    }
}

llvm::Type* Type::llvmBaseType(llvm::LLVMContext& C) const {
    switch (base) {
    case PrimitiveType::Void:
        return llvm::Type::getVoidTy(C);
    case PrimitiveType::Int:
        return llvm::Type::getInt32Ty(C);
    case PrimitiveType::Float:
        return llvm::Type::getFloatTy(C);
    case PrimitiveType::Bool:
        return llvm::Type::getInt1Ty(C);
    }
}

/***********************************************************************/

std::ostream& operator<<(std::ostream& os, AdditiveOp const& add_op) {
    switch (add_op) {
    case AdditiveOp::PLUS:
        os << '+';
        break;
    case AdditiveOp::MINUS:
        os << '-';
        break;
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, MultiplicativeOp const& mul_op) {
    switch (mul_op) {
    case MultiplicativeOp::TIMES:
        os << '*';
        break;
    case MultiplicativeOp::DIVIDE:
        os << '/';
        break;
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, RelationalOp const& rel_op) {
    switch (rel_op) {
    case RelationalOp::LT:
        os << '<';
        break;
    case RelationalOp::LTE:
        os << "<=";
        break;
    case RelationalOp::GT:
        os << '>';
        break;
    case RelationalOp::GTE:
        os << ">=";
        break;
    case RelationalOp::EQ:
        os << "==";
        break;
    case RelationalOp::NEQ:
        os << "!=";
        break;
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, UnaryOp const& unary_op) {
    switch (unary_op) {
    case UnaryOp::INCREMENT:
        os << "++";
        break;
    case UnaryOp::DECREMENT:
        os << "--";
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

ImplicitCastNode::ImplicitCastNode(unique_ptr<VariableExpressionNode> lvalue)
    : Node {lvalue->loc}, lvalue {std::move(lvalue)} {}

void ImplicitCastNode::accept(Visitor& visitor) {
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

FloatLiteralExpressionNode::FloatLiteralExpressionNode(
    float value,
    Location loc)
    : Node {loc}, value {value} {}

void FloatLiteralExpressionNode::accept(Visitor& visitor) {
    visitor.visit(*this);
}

BoolLiteralExpressionNode::BoolLiteralExpressionNode(bool value, Location loc)
    : Node {loc}, value {value} {}

void BoolLiteralExpressionNode::accept(Visitor& visitor) {
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
    unique_ptr<StatementNode> else_stmt,
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
    , else_stmt {nullptr} {}

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
    unique_ptr<ExpressionNode> expr,
    Location loc)
    : Node {loc}, expression {std::move(expr)} {}

ReturnStatementNode::ReturnStatementNode(Location loc)
    : Node {loc}, expression {nullptr} {}

void ReturnStatementNode::accept(Visitor& visitor) {
    visitor.visit(*this);
}

ExpressionStatementNode::ExpressionStatementNode(
    unique_ptr<ExpressionNode> expr,
    Location loc)
    : Node {loc}, expr {std::move(expr)} {}

ExpressionStatementNode::ExpressionStatementNode(Location loc)
    : Node {loc}, expr {nullptr} {}

void ExpressionStatementNode::accept(Visitor& visitor) {
    visitor.visit(*this);
}

/***********************************************************************/
