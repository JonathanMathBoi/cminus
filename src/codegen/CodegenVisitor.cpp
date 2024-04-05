#include "CodegenVisitor.hpp"

#include <llvm/ADT/STLExtras.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Type.h>

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

/***********************************************************************/
