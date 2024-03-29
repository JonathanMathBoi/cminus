#ifndef SEMANTICVISITOR_HPP
#define SEMANTICVISITOR_HPP

/***********************************************************************/

#include "../MiscUtils.hpp"
#include "../ast/AST.hpp"

#include <memory>

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

private:
    /// The pointer to the function currently being checked
    ///
    /// A raw pointer is used as it is non-owning. It is initilized as nullptr
    /// as it should not be set in global scope.
    FunctionDeclarationNode const* m_currentFunction {nullptr};
};

/***********************************************************************/

class SemanticException : public CMinusException {
public:
    SemanticException(Location loc);

    virtual char const* what() const noexcept override;

protected:
    std::string m_errorMessage;
};

/***********************************************************************/

class EarlyMainException : public SemanticException {
public:
    EarlyMainException(std::shared_ptr<DeclarationNode> earlyDecl);

private:
    std::shared_ptr<DeclarationNode> m_earlyDecl;
};

class ArrayFunctionException : public SemanticException {
public:
    ArrayFunctionException(FunctionDeclarationNode const& badFunc);
};

class VoidVariableException : public SemanticException {
public:
    VoidVariableException(VariableDeclarationNode const& badVar);
    VoidVariableException(ParameterNode const& badParam);
};

class NonPositiveArraySizeException : public SemanticException {
public:
    NonPositiveArraySizeException(ArrayDeclarationNode const& badArray);
};

class InvalidConditionException : public SemanticException {
public:
    InvalidConditionException(IfStatementNode const& badIf);
    InvalidConditionException(WhileStatementNode const& badWhile);
};

class BadReturnException : public SemanticException {
public:
    BadReturnException(
        TypeSpecifier expected_type,
        TypeSpecifier received_type,
        FunctionDeclarationNode const& func,
        ReturnStatementNode const& ret);
};

/***********************************************************************/

#endif
