#include "SemanticVisitor.hpp"

#include <cassert>
#include <iomanip>
#include <map>
#include <memory>
#include <sstream>

using std::shared_ptr;

/***********************************************************************/

void SemanticVisitor::visit(ProgramNode& node) {
    for (auto& decl : node.declarations) {
        // Check if decl is main func, and that it is not the last element
        // If main is declared early throw
        if (decl->identifier == "main" && &decl != &node.declarations.back()) {
            throw EarlyMainException {decl};
        }

        decl->accept(*this);
    }
}

void SemanticVisitor::visit(FunctionDeclarationNode& node) {
    assert(
        node.type.is_function &&
        "Function declarations should be marked as function");
    assert(
        node.type.type.kind != TypeKind::Array &&
        "Current grammar does not allow for array returning function");

    for (auto& param : node.parameters) {
        param->accept(*this);
    }

    m_currentFunction = &node;
    node.function_body->accept(*this);
    m_currentFunction = nullptr;
}

void SemanticVisitor::visit(VariableDeclarationNode& node) {
    assert(
        !node.type.is_function &&
        "Variable declarations should not be marked as function");
    assert(
        node.type.type.kind != TypeKind::Array &&
        "Primative variable declarations should not have array type");

    // Asserts force type kind to primative
    switch (node.type.type.base) {
    case PrimitiveType::Void:
        throw VoidVariableException {node};
    case PrimitiveType::Int:
        break;
    }
}

void SemanticVisitor::visit(ArrayDeclarationNode& node) {
    assert(
        !node.type.is_function &&
        "Array declarations should not be marked as function");
    assert(
        node.type.type.kind == TypeKind::Array &&
        "Array declarations should have array type");

    // Asserts force type kind to be array
    switch (node.type.type.base) {
    case PrimitiveType::Void:
        throw VoidVariableException {node};
    case PrimitiveType::Int:
        break;
    }

    if (node.size <= 0) {
        throw NonPositiveArraySizeException {node};
    }
}

void SemanticVisitor::visit(ParameterNode& node) {
    assert(
        !node.type.is_function &&
        "Parameters should not be marked as function");

    // Type kind is irrelevant for bad void checking
    switch (node.type.type.base) {
    case PrimitiveType::Void:
        throw VoidVariableException {node};
    case PrimitiveType::Int:
        break;
    }
}

void SemanticVisitor::visit(CompoundStatementNode& node) {
    for (auto& decl : node.local_decls) {
        decl->accept(*this);
    }

    for (auto& stmt : node.statements) {
        stmt->accept(*this);
    }
}

void SemanticVisitor::visit(IfStatementNode& node) {
    node.condition->accept(*this);
    assert(
        node.condition->type &&
        "Expression type should be calculated by visit");
    if (node.condition->type !=
        Type {TypeKind::Primitive, PrimitiveType::Int}) {
        throw InvalidConditionException {node};
    }

    node.then_stmt->accept(*this);

    if (node.else_stmt) {
        (*node.else_stmt)->accept(*this);
    }
}

void SemanticVisitor::visit(WhileStatementNode& node) {
    node.condition->accept(*this);
    assert(
        node.condition->type &&
        "Expression type should be calculated by visit");
    if (node.condition->type !=
        Type {TypeKind::Primitive, PrimitiveType::Int}) {
        throw InvalidConditionException {node};
    }

    node.body->accept(*this);
}

void SemanticVisitor::visit(ReturnStatementNode& node) {
    assert(
        m_currentFunction &&
        "Return statements should only occur within functions");

    Type func_type {m_currentFunction->type.type};

    if (!node.expression) {
        if (func_type != Type {TypeKind::Primitive, PrimitiveType::Void}) {
            throw BadReturnException {
                func_type, Type {TypeKind::Primitive, PrimitiveType::Void},
                *m_currentFunction, node};
        }
    } else {
        (*node.expression)->accept(*this);
        assert(
            (*node.expression)->type &&
            "Expression type should be calculated by visit");
        if (func_type != (*node.expression)->type.value()) {
            throw BadReturnException {
                func_type, (*node.expression)->type.value(), *m_currentFunction,
                node};
        }
    }
}

void SemanticVisitor::visit(ExpressionStatementNode& node) {
    if (node.expr) {
        (*node.expr)->accept(*this);
    }
}

/***********************************************************************/

SemanticException::SemanticException(Location loc) : CMinusException {loc} {}

char const* SemanticException::what() const noexcept {
    return m_errorMessage.c_str();
}

EarlyMainException::EarlyMainException(shared_ptr<DeclarationNode> earlyDecl)
    : SemanticException {earlyDecl->loc}, m_earlyDecl {earlyDecl} {
    std::stringstream message_buffer;
    message_buffer << "Early main declaration at line: " << location.line_num
                   << ", col: " << location.col_num << ".\n"
                   << "  main must be the last function declared.";
    m_errorMessage = message_buffer.str();
}

VoidVariableException::VoidVariableException(
    VariableDeclarationNode const& badVar)
    : SemanticException {badVar.loc} {
    std::stringstream message_buffer;
    message_buffer << "Error: variable " << std::quoted(badVar.identifier)
                   << " declared with type " << badVar.type.type << ".\n"
                   << "  line: " << location.line_num
                   << ", col: " << location.col_num << '.';
    m_errorMessage = message_buffer.str();
}

VoidVariableException::VoidVariableException(ParameterNode const& badParam)
    : SemanticException {badParam.loc} {
    std::stringstream message_buffer;
    message_buffer << "Error: variable " << std::quoted(badParam.identifier)
                   << " declared with type " << badParam.type.type << ".\n"
                   << "  line: " << location.line_num
                   << ", col: " << location.col_num << '.';
    m_errorMessage = message_buffer.str();
}

NonPositiveArraySizeException::NonPositiveArraySizeException(
    ArrayDeclarationNode const& badArray)
    : SemanticException {badArray.loc} {
    std::stringstream message_buffer;
    message_buffer << "Error: array " << std::quoted(badArray.identifier)
                   << " declared with non-positive size " << badArray.size
                   << '\n'
                   << "  line: " << location.line_num
                   << ", col: " << location.col_num << '.';
    m_errorMessage = message_buffer.str();
}

InvalidConditionException::InvalidConditionException(
    IfStatementNode const& badIf)
    : SemanticException {badIf.condition->loc} {
    std::stringstream message_buffer;
    message_buffer << "Error: invalid condition for if statement.\n"
                   << "  If condition must be an int.\n"
                   << "  line: " << location.line_num
                   << ", col: " << location.col_num << '.';
    m_errorMessage = message_buffer.str();
}

InvalidConditionException::InvalidConditionException(
    WhileStatementNode const& badWhile)
    : SemanticException {badWhile.condition->loc} {
    std::stringstream message_buffer;
    message_buffer << "Error: invalid condition for while statement.\n"
                   << "  While condition must be an int.\n"
                   << "  line: " << location.line_num
                   << ", col: " << location.col_num << '.';
    m_errorMessage = message_buffer.str();
}

BadReturnException::BadReturnException(
    Type expected_type,
    Type received_type,
    FunctionDeclarationNode const& func,
    ReturnStatementNode const& ret)
    : SemanticException {ret.loc} {
    std::stringstream message_buffer;
    message_buffer << "Error: Incorrect return type\n"
                   << "  Enclosing function " << std::quoted(func.identifier)
                   << " returns type " << func.type.type << ' '
                   << "(line: " << func.loc.line_num
                   << ", col: " << func.loc.col_num << ")\n"
                   << "  Return statement returns ";

    if (ret.expression) {
        assert(
            (*ret.expression)->type &&
            "Type should already be calculated for return expression");
        message_buffer << *(*ret.expression)->type;
    } else {
        message_buffer << Type {TypeKind::Primitive, PrimitiveType::Void};
    }

    message_buffer << " (line:" << ret.loc.line_num
                   << ", col: " << ret.loc.col_num << ")";
}

/***********************************************************************/
