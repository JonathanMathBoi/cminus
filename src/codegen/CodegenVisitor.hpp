#ifndef CODEGENVISITOR_HPP
#define CODEGENVISITOR_HPP

/***********************************************************************/

#include "../ast/AST.hpp"

#include <memory>
#include <optional>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

/***********************************************************************/

class CodegenVisitor : public Visitor {
public:
    /// \brief Constructs a CodegenVisitor with the given LLVM context and
    /// module
    ///
    /// \param context the LLVM context
    /// \param module the module to generate code in
    CodegenVisitor(
        std::shared_ptr<llvm::LLVMContext> context,
        std::shared_ptr<llvm::Module> module);

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
    virtual void visit(ImplicitCastNode& node) override;
    virtual void visit(CallExpressionNode& node) override;
    virtual void visit(AdditiveExpressionNode& node) override;
    virtual void visit(MultiplicativeExpressionNode& node) override;
    virtual void visit(RelationalExpressionNode& node) override;
    virtual void visit(IntegerLiteralExpressionNode& node) override;
    virtual void visit(FloatLiteralExpressionNode& node) override;
    virtual void visit(BoolLiteralExpressionNode& node) override;

private:
    /// Generates the code for the compiler builtin functions
    void codegenBuiltins();

    /// Gets the LLVM type of the use of a variable
    llvm::Type* useType(Type const& type) const;
    /// Gets the LLVM type of the declaration of a variable
    llvm::Type* declType(
        Type const& type,
        std::optional<int> size = std::nullopt) const;
    /// Gets the LLVM type for the base type of a type
    llvm::Type* baseType(Type const& type) const;

private:
    std::shared_ptr<llvm::LLVMContext> m_context;
    std::shared_ptr<llvm::Module> m_module;
    std::unique_ptr<llvm::IRBuilder<>> m_irBuilder;
};

/***********************************************************************/

#endif
