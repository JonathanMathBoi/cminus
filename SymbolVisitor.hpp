#ifndef SYMBOLVISITOR_HPP
#define SYMBOLVISITOR_HPP

/***********************************************************************/

#include "AST.hpp"
#include "SymbolTable.hpp"

/***********************************************************************/

class SymbolVisitor : public Visitor {
public:
    SymbolVisitor();

    virtual void visit(ProgramNode& node) override;

    virtual void visit(DeclarationNode& node) override;
    virtual void visit(FunctionDeclarationNode& node) override;
    virtual void visit(VariableDeclarationNode& node) override;
    virtual void visit(ArrayDeclarationNode& node) override;
    virtual void visit(ParameterNode& node) override;

    virtual void visit(StatementNode& node) override;
    virtual void visit(CompoundStatementNode& node) override;
    virtual void visit(IfStatementNode& node) override;
    virtual void visit(WhileStatementNode& node) override;
    virtual void visit(ReturnStatementNode& node) override;
    virtual void visit(ExpressionStatementNode& node) override;

    virtual void visit(ExpressionNode& node) override;
    virtual void visit(AssignmentExpressionNode& node) override;
    virtual void visit(VariableExpressionNode& node) override;
    virtual void visit(SubscriptExpressionNode& node) override;
    virtual void visit(CallExpressionNode& node) override;
    virtual void visit(AdditiveExpressionNode& node) override;
    virtual void visit(MultiplicativeExpressionNode& node) override;
    virtual void visit(RelationalExpressionNode& node) override;
    virtual void visit(IntegerLiteralExpressionNode& node) override;

private:
    SymbolTable m_table;
};

/***********************************************************************/

#endif
