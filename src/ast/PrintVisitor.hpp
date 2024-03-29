#ifndef PRINT_VISITOR_HPP
#define PRINT_VISITOR_HPP

/***********************************************************************/

#include "AST.hpp"

#include <ostream>

/***********************************************************************/

/// AST Visitor for printing AST
class PrintVisitor : public Visitor {
public:
    /// Constructs a print visitor
    ///
    /// \param out_stream the ostream to print the tree to
    PrintVisitor(std::ostream& out_stream);

    virtual void visit(ProgramNode& node) override;

    virtual void visit(FunctionDeclarationNode& node) override;
    virtual void visit(VariableDeclarationNode& node) override;
    virtual void visit(ArrayDeclarationNode& node) override;
    virtual void visit(ParameterNode& node) override;

    virtual void visit(CompoundStatementNode& node) override;
    virtual void visit(IfStatementNode& node) override;
    virtual void visit(WhileStatementNode& node) override;
    // Not parsing for statement yet
    // virtual void visit(for_statement_node& node) override;
    virtual void visit(ReturnStatementNode& node) override;
    virtual void visit(ExpressionStatementNode& node) override;

    virtual void visit(AssignmentExpressionNode& node) override;
    virtual void visit(VariableExpressionNode& node) override;
    virtual void visit(SubscriptExpressionNode& node) override;
    virtual void visit(CallExpressionNode& node) override;
    virtual void visit(AdditiveExpressionNode& node) override;
    virtual void visit(MultiplicativeExpressionNode& node) override;
    virtual void visit(RelationalExpressionNode& node) override;
    // Not parsing increment and decrement yet
    // virtual void visit(unary_expression_node& node) override;
    virtual void visit(IntegerLiteralExpressionNode& node) override;

private:
    /// Prints the indent in front of the next to be printed element
    std::string getIndent() const;
    friend struct NestGuard;

private:
    /// The output stream to be printed to
    std::ostream& m_output;
    /// The current nest depth of the tree
    unsigned m_currentDepth;
};

/***********************************************************************/

#endif
