#ifndef SEMANTICVISITOR_HPP
#define SEMANTICVISITOR_HPP

/***********************************************************************/

#include "../MiscUtils.hpp"
#include "../ast/AST.hpp"

#include <vector>

/***********************************************************************/

class SemanticError;

/***********************************************************************/

class SemanticVisitor : public Visitor {
public:
    virtual void visit(ProgramNode& node) override;

    virtual void visit(Declaration& node) override;

    virtual void visit(CompoundStatementNode& node) override;
    virtual void visit(IfStatementNode& node) override;
    virtual void visit(WhileStatementNode& node) override;
    virtual void visit(ReturnStatementNode& node) override;
    virtual void visit(ExpressionStatementNode& node) override;

    virtual void visit(AssignmentExpressionNode& node) override;
    virtual void visit(VariableExpressionNode& node) override;
    virtual void visit(SubscriptExpressionNode& node) override;
    virtual void visit(ImplicitCastNode& node) override;
    virtual void visit(CallExpressionNode& node) override;
    virtual void visit(AdditiveExpressionNode& node) override;
    virtual void visit(MultiplicativeExpressionNode& node) override;
    virtual void visit(RelationalExpressionNode& node) override;
    virtual void visit(IntegerLiteralExpressionNode& node) override;
    virtual void visit(FloatLiteralExpressionNode& node) override;
    virtual void visit(BoolLiteralExpressionNode& node) override;

    /// Check if the program was valid
    ///
    /// Should only be called after visiting the AST
    ///
    /// \returns if the program is valid
    bool isValid() const;

    /// Get the semantic errors in the program
    ///
    /// \returns all the semantic errors in the program
    const std::vector<SemanticError>& errors() const;

private:
    void addError(SemanticError error);

private:
    /// A vector of all semantic error in the program
    std::vector<SemanticError> m_errors;
    /// The pointer to the function declaration currently being checked
    ///
    /// A raw pointer is used as it is non-owning. It is initilized as nullptr
    /// as it should not be set in global scope.
    Declaration const* m_currentFunction {nullptr};
};

/***********************************************************************/

class SemanticError {
public:
    const std::string_view message() const;
    Location location() const;

public:
    static SemanticError earlyMain(Declaration const& decl);
    static SemanticError missingMain();
    static SemanticError voidVariable(Declaration const& varDecl);
    static SemanticError voidParam(Declaration const& paramDecl);
    static SemanticError nonPositiveArraySize(Declaration const& arrDecl);
    static SemanticError invalidCondition(IfStatementNode const& ifStmt);
    static SemanticError invalidCondition(WhileStatementNode const& whileStmt);
    static SemanticError badReturn(
        ReturnStatementNode const& ret,
        Declaration const& func);
    static SemanticError arrayAssignment(
        AssignmentExpressionNode const& assignExpr);
    static SemanticError mismatchAssignment(
        AssignmentExpressionNode const& assignExpr);
    static SemanticError functionAsVariable(
        VariableExpressionNode const& varExpr);
    static SemanticError variableAsFunction(CallExpressionNode const& callExpr);
    static SemanticError wrongArgumentCount(
        CallExpressionNode const& callExpr,
        Declaration const& func);
    static SemanticError wrongArgumentType(
        CallExpressionNode const& callExpr,
        Declaration const& func,
        unsigned arg_num);
    static SemanticError indexNonArray(
        SubscriptExpressionNode const& subscriptExpr);
    static SemanticError badIndex(SubscriptExpressionNode const& subscriptExpr);
    static SemanticError invalidOperation(
        AdditiveExpressionNode const& addExpr,
        Type left,
        Type right);
    static SemanticError invalidOperation(
        MultiplicativeExpressionNode const& mulExpr,
        Type left,
        Type right);
    static SemanticError invalidOperation(
        RelationalExpressionNode const& relExpr,
        Type left,
        Type right);
    static SemanticError nonReturningFunction(Declaration const& func);

private:
    SemanticError(std::string error_message, Location loc);

private:
    std::string m_message;
    Location m_location;
};

/***********************************************************************/

#endif
