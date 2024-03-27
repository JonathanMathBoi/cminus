#include "SymbolVisitor.hpp"
#include "AST.hpp"

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
        (*node.else_stmt)->accept(*this);
    }
}

void SymbolVisitor::visit(WhileStatementNode& node) {
    node.condition->accept(*this);
    node.body->accept(*this);
}

void SymbolVisitor::visit(ReturnStatementNode& node) {
    if (node.expression) {
        (*node.expression)->accept(*this);
    }
}

void SymbolVisitor::visit(ExpressionStatementNode& node) {
    if (node.expr) {
        (*node.expr)->accept(*this);
    }
}

/***********************************************************************/
