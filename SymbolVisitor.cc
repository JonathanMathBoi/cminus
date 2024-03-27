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

void SymbolVisitor::visit(DeclarationNode& node) {
    node.accept(*this);
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

/***********************************************************************/
