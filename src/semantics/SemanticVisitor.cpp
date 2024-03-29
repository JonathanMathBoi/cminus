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
    if (node.type.is_array) {
        throw ArrayFunctionException {node};
    }

    for (auto& param : node.parameters) {
        param->accept(*this);
    }

    m_currentFunction = &node;
    node.function_body->accept(*this);
    m_currentFunction = nullptr;
}

void SemanticVisitor::visit(VariableDeclarationNode& node) {
    switch (node.type.type) {
    case TypeSpecifier::VOID:
        throw VoidVariableException {node};
    case TypeSpecifier::INT:
        break;
    }
}

void SemanticVisitor::visit(ArrayDeclarationNode& node) {
    // Assert because this should happen at parsing
    assert(
        node.type.is_array && "ArrayDeclarationNode should have an array type");
    visit(static_cast<VariableDeclarationNode&>(node));

    if (node.size <= 0) {
        throw NonPositiveArraySizeException {node};
    }
}

void SemanticVisitor::visit(ParameterNode& node) {
    switch (node.type.type) {
    case TypeSpecifier::VOID:
        throw VoidVariableException {node};
    case TypeSpecifier::INT:
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
    // Getting the option out should be safe as type should be computed by the
    // above call
    if (node.condition->type->type != TypeSpecifier::INT) {
        throw InvalidConditionException {node};
    }

    node.then_stmt->accept(*this);

    if (node.else_stmt) {
        (*node.else_stmt)->accept(*this);
    }
}

void SemanticVisitor::visit(WhileStatementNode& node) {
    node.condition->accept(*this);
    // Getting the option out should be safe as type should be computed by the
    // above call
    if (node.condition->type->type != TypeSpecifier::INT) {
        throw InvalidConditionException {node};
    }

    node.body->accept(*this);
}

void SemanticVisitor::visit(ReturnStatementNode& node) {
    assert(
        m_currentFunction &&
        "Return statements should only occur within functions");

    if (!node.expression &&
        m_currentFunction->type.type != TypeSpecifier::VOID) {
        throw BadReturnException {
            m_currentFunction->type.type, TypeSpecifier::VOID,
            *m_currentFunction, node};
    }

    if (node.expression &&
        m_currentFunction->type.type == TypeSpecifier::VOID) {
        (*node.expression)->accept(*this);
        throw BadReturnException {
            TypeSpecifier::VOID, (*node.expression)->type->type,
            *m_currentFunction, node};
    }

    if (node.expression &&
        m_currentFunction->type.type != TypeSpecifier::VOID) {
        (*node.expression)->accept(*this);
        if ((*node.expression)->type->type != m_currentFunction->type.type) {
            throw BadReturnException {
                m_currentFunction->type.type, (*node.expression)->type->type,
                *m_currentFunction, node};
        }
        return;
    }

    // If return; and void function
    // no action needed
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

ArrayFunctionException::ArrayFunctionException(
    FunctionDeclarationNode const& badFunc)
    : SemanticException {badFunc.loc} {
    std::stringstream message_buffer;
    message_buffer << "Error: functions can not return array types.\n"
                   << "  function " << std::quoted(badFunc.identifier)
                   << " declared with an array type."
                   << "  line: " << location.line_num
                   << ", col: " << location.col_num << '.';
    m_errorMessage = message_buffer.str();
}

VoidVariableException::VoidVariableException(
    VariableDeclarationNode const& badVar)
    : SemanticException {badVar.loc} {
    std::stringstream message_buffer;
    message_buffer << "Error: variable " << std::quoted(badVar.identifier)
                   << " declared with type \"void\".\n"
                   << "  line: " << location.line_num
                   << ", col: " << location.col_num << '.';
    m_errorMessage = message_buffer.str();
}

VoidVariableException::VoidVariableException(ParameterNode const& badParam)
    : SemanticException {badParam.loc} {
    std::stringstream message_buffer;
    message_buffer << "Error: variable " << std::quoted(badParam.identifier)
                   << " declared with type \"void\".\n"
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
    TypeSpecifier expected_type,
    TypeSpecifier received_type,
    FunctionDeclarationNode const& func,
    ReturnStatementNode const& ret)
    : SemanticException {ret.loc} {
    static const std::map<TypeSpecifier, const std::string_view> types {
        {TypeSpecifier::VOID, "void"}, {TypeSpecifier::INT, "int"}};

    std::stringstream message_buffer;
    message_buffer << "Error: Incorrect return type\n"
                   << "  Enclosing function " << std::quoted(func.identifier)
                   << " returns type " << types.at(func.type.type)
                   << "(line: " << func.loc.line_num
                   << ", col: " << func.loc.col_num << ")\n"
                   << "  Return statement returns ";

    if (ret.expression) {
        message_buffer << types.at((*ret.expression)->type->type);
    } else {
        message_buffer << types.at(TypeSpecifier::VOID);
    }

    message_buffer << " (line:" << ret.loc.line_num
                   << ", col: " << ret.loc.col_num << ")";
}

/***********************************************************************/
