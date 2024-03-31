#include "SemanticVisitor.hpp"

#include <algorithm>
#include <cassert>
#include <iomanip>
#include <memory>
#include <sstream>

/***********************************************************************/

bool SemanticVisitor::isValid() const {
    return m_errors.empty();
}

const std::vector<SemanticError>& SemanticVisitor::errors() const {
    return m_errors;
}

void SemanticVisitor::addError(SemanticError error) {
    m_errors.push_back(error);
    // Sorts errors in order of occurance
    std::ranges::sort(m_errors, [](SemanticError a, SemanticError b) {
        if (a.location().line_num < b.location().line_num) {
            return true;
        }

        if (a.location().line_num > b.location().line_num) {
            return false;
        }

        return a.location().col_num < b.location().col_num;
    });
}

/***********************************************************************/

void SemanticVisitor::visit(ProgramNode& node) {
    for (auto& decl : node.declarations) {
        // Check if decl is main func, and that it is not the last element
        if (decl->identifier == "main" && &decl != &node.declarations.back()) {
            addError(SemanticError::earlyMain(*decl));
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
        addError(SemanticError::voidVariable(node));
        break;
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
        addError(SemanticError::voidVariable(node));
        break;
    case PrimitiveType::Int:
        break;
    }

    if (node.size <= 0) {
        addError(SemanticError::nonPositiveArraySize(node));
    }
}

void SemanticVisitor::visit(ParameterNode& node) {
    assert(
        !node.type.is_function &&
        "Parameters should not be marked as function");

    // Type kind is irrelevant for bad void checking
    switch (node.type.type.base) {
    case PrimitiveType::Void:
        addError(SemanticError::voidParam(node));
        break;
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
        addError(SemanticError::invalidCondition(node));
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
        addError(SemanticError::invalidCondition(node));
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
            addError(SemanticError::badReturn(node, *m_currentFunction));
        }
    } else {
        (*node.expression)->accept(*this);
        assert(
            (*node.expression)->type &&
            "Expression type should be calculated by visit");
        if (func_type != (*node.expression)->type.value()) {
            addError(SemanticError::badReturn(node, *m_currentFunction));
        }
    }
}

void SemanticVisitor::visit(ExpressionStatementNode& node) {
    if (node.expr) {
        (*node.expr)->accept(*this);
    }
}

/***********************************************************************/

SemanticError::SemanticError(std::string error_message, Location loc)
    : m_message {error_message}, m_location {loc} {}

const std::string_view SemanticError::message() const {
    return m_message;
}

Location SemanticError::location() const {
    return m_location;
}

/***********************************************************************/

SemanticError SemanticError::earlyMain(DeclarationNode const& decl) {
    std::stringstream message_buffer;
    message_buffer << "Early main declaration at line: " << decl.loc.line_num
                   << ", col: " << decl.loc.col_num << ".\n"
                   << "  main must be the last function declared.";
    return {message_buffer.str(), decl.loc};
}

SemanticError SemanticError::voidVariable(
    VariableDeclarationNode const& varDecl) {
    std::stringstream message_buffer;
    message_buffer << "Error: variable " << std::quoted(varDecl.identifier)
                   << " declared with type " << varDecl.type.type << ".\n"
                   << "  line: " << varDecl.loc.line_num
                   << ", col: " << varDecl.loc.col_num << '.';
    return {message_buffer.str(), varDecl.loc};
}

SemanticError SemanticError::voidParam(ParameterNode const& paramDecl) {
    std::stringstream message_buffer;
    message_buffer << "Error: parameter " << std::quoted(paramDecl.identifier)
                   << " declared with type " << paramDecl.type.type << ".\n"
                   << "  line: " << paramDecl.loc.line_num
                   << ", col: " << paramDecl.loc.col_num << '.';
    return {message_buffer.str(), paramDecl.loc};
}

SemanticError SemanticError::nonPositiveArraySize(
    ArrayDeclarationNode const& arrDecl) {
    std::stringstream message_buffer;
    message_buffer << "Error: array " << std::quoted(arrDecl.identifier)
                   << " declared with non-positive size " << arrDecl.size
                   << '\n'
                   << "  line: " << arrDecl.loc.line_num
                   << ", col: " << arrDecl.loc.col_num << '.';
    return {message_buffer.str(), arrDecl.loc};
}

SemanticError SemanticError::invalidCondition(IfStatementNode const& ifStmt) {
    std::stringstream message_buffer;
    message_buffer << "Error: invalid condition for if statement.\n"
                   << "  If condition must be an int.\n"
                   << "  line: " << ifStmt.condition->loc.line_num
                   << ", col: " << ifStmt.condition->loc.col_num << '.';
    return {message_buffer.str(), ifStmt.condition->loc};
}

SemanticError SemanticError::invalidCondition(
    WhileStatementNode const& whileStmt) {
    std::stringstream message_buffer;
    message_buffer << "Error: invalid condition for while statement.\n"
                   << "  While condition must be an int.\n"
                   << "  line: " << whileStmt.condition->loc.line_num
                   << ", col: " << whileStmt.condition->loc.col_num << '.';
    return {message_buffer.str(), whileStmt.condition->loc};
}

SemanticError SemanticError::badReturn(
    ReturnStatementNode const& ret,
    FunctionDeclarationNode const& func) {
    std::stringstream message_buffer;
    message_buffer << "Error: Incorrect return type\n"
                   << "  In function " << std::quoted(func.identifier)
                   << " returning " << func.type.type
                   << " (line: " << func.loc.line_num
                   << ", col: " << func.loc.col_num << ")\n"
                   << "  Return statement returns ";

    if (ret.expression) {
        assert(
            (*ret.expression)->type &&
            "Type should already be calculated for bad return error");
        message_buffer << (*ret.expression)->type.value();
    } else {
        message_buffer << Type {TypeKind::Primitive, PrimitiveType::Void};
    }

    message_buffer << " (line: " << ret.loc.line_num
                   << ", col: " << ret.loc.col_num << ")";

    return {message_buffer.str(), ret.loc};
}

/***********************************************************************/
