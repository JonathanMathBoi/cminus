#include "CodegenVisitor.hpp"

#include <llvm/ADT/STLExtras.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Verifier.h>

#include <memory>
#include <vector>

using llvm::BasicBlock;
using llvm::Function;
using llvm::FunctionType;
using llvm::GlobalVariable;

/***********************************************************************/

CodegenVisitor::CodegenVisitor(
    std::shared_ptr<llvm::LLVMContext> context,
    std::shared_ptr<llvm::Module> module)
    : m_context {context}
    , m_module {module}
    , m_irBuilder {std::make_unique<llvm::IRBuilder<>>(*m_context)} {}

/***********************************************************************/

void CodegenVisitor::visit(ProgramNode& node) {
    for (auto& decl : node.declarations) {
        decl->accept(*this);
    }
}

void CodegenVisitor::visit(FunctionDeclarationNode& node) {
    std::vector<llvm::Type*> param_types;
    for (auto& param : node.parameters) {
        param_types.push_back(param->type.type.llvmParamType(*m_context));
    }

    FunctionType* func_type {FunctionType::get(
        node.type.type.llvmParamType(*m_context), param_types,
        /*isVarArg=*/false)};

    Function* func {Function::Create(
        func_type, Function::ExternalLinkage, node.identifier, *m_module)};

    for (auto [param, arg] : llvm::zip(node.parameters, func->args())) {
        arg.setName(param->identifier);
    }

    BasicBlock* preamble {BasicBlock::Create(*m_context, "preamble", func)};
    m_irBuilder->SetInsertPoint(preamble);

    for (auto [param, arg] : llvm::zip(node.parameters, func->args())) {
        // %param.local = alloca <type>
        param->ir_value = m_irBuilder->CreateAlloca(
            param->type.type.llvmParamType(*m_context), /*ArraySize=*/nullptr,
            param->identifier + ".local");
        // store <type> %param, ptr %param.local
        m_irBuilder->CreateStore(&arg, param->ir_value);
    }

    BasicBlock* body {BasicBlock::Create(*m_context, "body", func)};
    m_irBuilder->CreateBr(body);
    m_irBuilder->SetInsertPoint(body);

    node.function_body->accept(*this);

    // If the function is void and never returns, add return at end of function
    if (!*node.function_body->always_returns) {
        assert(
            node.type.type == Types::Void &&
            "Only void functions should return implicitly");
        m_irBuilder->CreateRetVoid();
    }

    llvm::verifyFunction(*func);

    node.ir_value = func;
}

void CodegenVisitor::visit(ParameterNode& node) {
    // Do nothing
    // Handled by function codegen
}

void CodegenVisitor::visit(VariableDeclarationNode& node) {
    assert(
        node.type.type.kind != TypeKind::Array &&
        "Variable declaration nodes should have primative type");

    if (node.nest_level == 0) {
        GlobalVariable* global = new GlobalVariable(
            node.type.type.llvmVarType(*m_context), /*isConstant=*/false,
            GlobalVariable::ExternalLinkage, /*Initializer=*/nullptr,
            node.identifier);
        node.ir_value = global;
        // give ownership to module
        m_module->insertGlobalVariable(global);
        return;
    }

    node.ir_value =
        m_irBuilder->CreateAlloca(node.type.type.llvmVarType(*m_context));
}

void CodegenVisitor::visit(ArrayDeclarationNode& node) {
    assert(
        node.type.type.kind == TypeKind::Array &&
        "Array declaration nodes should have an array type");

    if (node.nest_level == 0) {
        GlobalVariable* global = new GlobalVariable(
            node.type.type.llvmVarType(*m_context, node.size),
            /*isConstant=*/false, GlobalVariable::ExternalLinkage,
            /*Initializer=*/nullptr, node.identifier);
        node.ir_value = global;
        // give ownership to module
        m_module->insertGlobalVariable(global);
        return;
    }

    node.ir_value = m_irBuilder->CreateAlloca(
        node.type.type.llvmVarType(*m_context, node.size));
}

void CodegenVisitor::visit(CompoundStatementNode& node) {
    for (auto& decl : node.local_decls) {
        decl->accept(*this);
    }

    for (auto& stmt : node.statements) {
        stmt->accept(*this);
    }
}

void CodegenVisitor::visit(IfStatementNode& node) {
    node.condition->accept(*this);
    assert(
        node.condition->ir_value &&
        "Expression should have a Value after visit");

    Function* func {m_irBuilder->GetInsertBlock()->getParent()};

    BasicBlock* then_block {BasicBlock::Create(*m_context, "", func)};
    BasicBlock* merge_block {BasicBlock::Create(*m_context)};
    BasicBlock* else_block;
    if (node.else_stmt) {
        else_block = BasicBlock::Create(*m_context);
    } else {
        else_block = merge_block;
    }

    // br i1 <condition>, label <then_block>, label <else_block>
    m_irBuilder->CreateCondBr(node.condition->ir_value, then_block, else_block);

    // Write then block
    m_irBuilder->SetInsertPoint(then_block);
    node.then_stmt->accept(*this);
    m_irBuilder->CreateBr(merge_block);

    // Write else block
    if (node.else_stmt) {
        func->insert(func->end(), else_block);
        m_irBuilder->SetInsertPoint(else_block);
        node.condition->accept(*this);
        m_irBuilder->CreateBr(merge_block);
    }

    // Write merge block
    func->insert(func->end(), merge_block);
    m_irBuilder->SetInsertPoint(merge_block);
}

void CodegenVisitor::visit(WhileStatementNode& node) {
    Function* func {m_irBuilder->GetInsertBlock()->getParent()};

    BasicBlock* check {BasicBlock::Create(*m_context, "", func)};
    BasicBlock* loop {BasicBlock::Create(*m_context)};
    BasicBlock* post {BasicBlock::Create(*m_context)};

    // br label <check>
    m_irBuilder->CreateBr(check);

    // Write the check block
    m_irBuilder->SetInsertPoint(check);
    node.condition->accept(*this);
    assert(
        node.condition->ir_value &&
        "Expression should have a Value after visit");
    // br i1 <condition>, label <loop>, label <post>
    m_irBuilder->CreateCondBr(node.condition->ir_value, loop, post);

    // Write the loop block
    func->insert(func->end(), loop);
    m_irBuilder->SetInsertPoint(loop);
    node.body->accept(*this);
    // br label <check> ; returns to check to continue loop
    m_irBuilder->CreateBr(check);

    // Write the post block
    func->insert(func->end(), post);
    m_irBuilder->SetInsertPoint(post);
}

void CodegenVisitor::visit(ReturnStatementNode& node) {
    if (!node.expression) {
        m_irBuilder->CreateRetVoid();
        return;
    }

    node.expression->accept(*this);
    assert(
        node.expression->ir_value &&
        "Expression should have a Value after visit");
    m_irBuilder->CreateRet(node.expression->ir_value);
}

void CodegenVisitor::visit(ExpressionStatementNode& node) {
    if (node.expr) {
        node.expr->accept(*this);
        assert(
            node.expr->ir_value &&
            "Expression should have a Value after visit");
    }
}

/***********************************************************************/
