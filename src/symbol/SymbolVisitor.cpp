#include "SymbolVisitor.hpp"
#include "../MiscUtils.hpp"
#include "../ast/AST.hpp"
#include "SymbolTable.hpp"

#include <iomanip>
#include <sstream>
#include <string_view>

/***********************************************************************/

SymbolVisitor::SymbolVisitor() : m_table {} {}

void SymbolVisitor::visit(ProgramNode& node) {
    for (auto& decl : node.declarations) {
        m_table.insert(decl);
        decl->nest_level = m_table.getNestLevel();
        decl->accept(*this);
    }
}

void SymbolVisitor::visit(FunctionDeclarationNode& node) {
    m_table.enterScope();

    for (auto& param : node.parameters) {
        m_table.insert(param);
        param->nest_level = m_table.getNestLevel();
        param->accept(*this);
    }

    node.function_body->accept(*this);

    m_table.exitScope();
}

void SymbolVisitor::visit(VariableDeclarationNode& node) {
    // Don't do anything
    // Should be handled and added by it's enclosing scope
}

void SymbolVisitor::visit(ArrayDeclarationNode& node) {
    // Don't do anything
    // Should be handled and added by it's enclosing scope
}

void SymbolVisitor::visit(ParameterNode& node) {
    // Don't do anything
    // Should be handled and added by it's enclosing scope
}

void SymbolVisitor::visit(CompoundStatementNode& node) {
    if (!node.is_function_body) {
        m_table.enterScope();
    }

    for (auto& decl : node.local_decls) {
        m_table.insert(decl);
        decl->nest_level = m_table.getNestLevel();
        decl->accept(*this);
    }

    for (auto& stmt : node.statements) {
        stmt->accept(*this);
    }

    if (!node.is_function_body) {
        m_table.exitScope();
    }
}

void SymbolVisitor::visit(IfStatementNode& node) {
    node.condition->accept(*this);
    node.then_stmt->accept(*this);
    if (node.else_stmt) {
        node.else_stmt->accept(*this);
    }
}

void SymbolVisitor::visit(WhileStatementNode& node) {
    node.condition->accept(*this);
    node.body->accept(*this);
}

void SymbolVisitor::visit(ReturnStatementNode& node) {
    if (node.expression) {
        node.expression->accept(*this);
    }
}

void SymbolVisitor::visit(ExpressionStatementNode& node) {
    if (node.expr) {
        node.expr->accept(*this);
    }
}

void SymbolVisitor::visit(AssignmentExpressionNode& node) {
    node.variable->accept(*this);
    node.expression->accept(*this);
}

void SymbolVisitor::visit(VariableExpressionNode& node) {
    auto decl {m_table.lookup(node.identifier)};

    // Check if the var was declared
    if (!decl) {
        throw UndeclaredSymbolException {node.identifier, node.loc};
    }

    // Set the referent to the declaration
    // decl is not nullptr at this point
    node.referent = decl;
}

void SymbolVisitor::visit(SubscriptExpressionNode& node) {
    visit(static_cast<VariableExpressionNode&>(node));
    node.index->accept(*this);
}

void SymbolVisitor::visit(CallExpressionNode& node) {
    auto decl {m_table.lookup(node.identifier)};

    // Check if the function was declared
    if (!decl) {
        throw UndeclaredSymbolException {node.identifier, node.loc};
    }

    // Set the referent to the declaration
    // decl is not none at this point
    node.referent = decl;

    for (auto& arg : node.arguments) {
        arg->accept(*this);
    }
}

void SymbolVisitor::visit(AdditiveExpressionNode& node) {
    node.left->accept(*this);
    node.right->accept(*this);
}

void SymbolVisitor::visit(MultiplicativeExpressionNode& node) {
    node.left->accept(*this);
    node.right->accept(*this);
}

void SymbolVisitor::visit(RelationalExpressionNode& node) {
    node.left->accept(*this);
    node.right->accept(*this);
}

void SymbolVisitor::visit(IntegerLiteralExpressionNode& node) {
    // Don't do anything
    // It's a literal
}

void SymbolVisitor::visit(FloatLiteralExpressionNode& node) {
    // Don't do anything
    // It's a literal
}

void SymbolVisitor::visit(BoolLiteralExpressionNode& node) {
    // Don't do anything
    // It's a literal
}

/***********************************************************************/

UndeclaredSymbolException::UndeclaredSymbolException(
    const std::string_view name,
    Location loc)
    : SymbolException {loc} {
    std::stringstream message_buffer;
    message_buffer << "Undeclared symbol " << std::quoted(name)
                   << " at line: " << loc.line_num << ", col: " << loc.col_num;
    m_errorMessage = message_buffer.str();
}

char const* UndeclaredSymbolException::what() const noexcept {
    return m_errorMessage.c_str();
}

/***********************************************************************/
