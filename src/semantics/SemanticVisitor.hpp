#ifndef SEMANTICVISITOR_HPP
#define SEMANTICVISITOR_HPP

/***********************************************************************/

#include "../MiscUtils.hpp"
#include "../ast/AST.hpp"

#include <memory>
#include <vector>

/***********************************************************************/

class SemanticError;

/***********************************************************************/

class SemanticVisitor : public Visitor {
public:
    virtual void visit(ProgramNode& node) override;

    virtual void visit(FunctionDeclarationNode& node) override;
    virtual void visit(VariableDeclarationNode& node) override;
    virtual void visit(ArrayDeclarationNode& node) override;
    virtual void visit(ParameterNode& node) override;

    virtual void visit(CompoundStatementNode& node) override;
    virtual void visit(IfStatementNode& node) override;
    virtual void visit(WhileStatementNode& node) override;
    virtual void visit(ReturnStatementNode& node) override;
    virtual void visit(ExpressionStatementNode& node) override;

    virtual void visit(AssignmentExpressionNode& node) override;
    virtual void visit(VariableExpressionNode& node) override;
    virtual void visit(SubscriptExpressionNode& node) override;
    virtual void visit(CallExpressionNode& node) override;
    virtual void visit(AdditiveExpressionNode& node) override;
    virtual void visit(MultiplicativeExpressionNode& node) override;
    virtual void visit(RelationalExpressionNode& node) override;
    virtual void visit(IntegerLiteralExpressionNode& node) override;

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
    /// The pointer to the function currently being checked
    ///
    /// A raw pointer is used as it is non-owning. It is initilized as nullptr
    /// as it should not be set in global scope.
    FunctionDeclarationNode const* m_currentFunction {nullptr};
};

/***********************************************************************/

class SemanticError {
public:
    const std::string_view message() const;
    Location location() const;

public:
    static SemanticError earlyMain(DeclarationNode const& decl);
    static SemanticError voidVariable(VariableDeclarationNode const& varDecl);
    static SemanticError voidParam(ParameterNode const& paramDecl);
    static SemanticError nonPositiveArraySize(
        ArrayDeclarationNode const& arrDecl);
    static SemanticError invalidCondition(IfStatementNode const& ifStmt);
    static SemanticError invalidCondition(WhileStatementNode const& whileStmt);
    static SemanticError badReturn(
        ReturnStatementNode const& ret,
        FunctionDeclarationNode const& func);
    static SemanticError arrayAssignment(
        AssignmentExpressionNode const& assignExpr);
    static SemanticError mismatchAssignment(
        AssignmentExpressionNode const& assignExpr);
    static SemanticError functionAsVariable(
        VariableExpressionNode const& varExpr);
    static SemanticError variableAsFunction(CallExpressionNode const& callExpr);
    static SemanticError wrongArgumentCount(
        CallExpressionNode const& callExpr,
        FunctionDeclarationNode const& func);
    static SemanticError wrongArgumentType(
        CallExpressionNode const& callExpr,
        FunctionDeclarationNode const& func,
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

private:
    SemanticError(std::string error_message, Location loc);

private:
    std::string m_message;
    Location m_location;
};

/***********************************************************************/

#endif
