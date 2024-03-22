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

    virtual void visit(DeclarationNode& node) override;
    virtual void visit(FunctionDeclarationNode& node) override;
    virtual void visit(VariableDeclarationNode& node) override;
    virtual void visit(ArrayDeclarationNode& node) override;
    virtual void visit(ParameterNode& node) override;

    virtual void visit(StatementNode& node) override;
    virtual void visit(compound_statement_node& node) override;
    virtual void visit(if_statement_node& node) override;
    virtual void visit(while_statement_node& node) override;
    // Not parsing for statement yet
    // virtual void visit(for_statement_node& node) override;
    virtual void visit(return_statement_node& node) override;
    virtual void visit(expression_statement_node& node) override;

    virtual void visit(expression_node& node) override;
    virtual void visit(assignment_expression_node& node) override;
    virtual void visit(variable_expression_node& node) override;
    virtual void visit(subscript_expression_node& node) override;
    virtual void visit(call_expression_node& node) override;
    virtual void visit(additive_expression_node& node) override;
    virtual void visit(multiplicative_expression_node& node) override;
    virtual void visit(relational_expression_node& node) override;
    // Not parsing increment and decrement yet
    // virtual void visit(unary_expression_node& node) override;
    virtual void visit(integer_literal_expression_node& node) override;

private:
    /// Prints the indent in front of the next to be printed element
    std::string indent() const;
    friend struct nest_guard;

private:
    /// The output stream to be printed to
    std::ostream& output;
    /// The current nest depth of the tree
    unsigned current_depth;
};

/***********************************************************************/

#endif
