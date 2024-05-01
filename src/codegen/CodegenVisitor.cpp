#include "CodegenVisitor.hpp"

#include <llvm/ADT/STLExtras.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constant.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Verifier.h>

#include <cassert>
#include <memory>
#include <optional>
#include <ranges>
#include <variant>
#include <vector>

using llvm::BasicBlock;
using llvm::Constant;
using llvm::Function;
using llvm::FunctionType;
using llvm::GlobalVariable;
using llvm::Value;

/***********************************************************************/

CodegenVisitor::CodegenVisitor(
    std::shared_ptr<llvm::LLVMContext> context,
    std::shared_ptr<llvm::Module> module)
    : m_context {context}
    , m_module {module}
    , m_irBuilder {std::make_unique<llvm::IRBuilder<>>(*m_context)} {}

llvm::Type* CodegenVisitor::useType(Type const& type) const {
    if (type.kind == TypeKind::Array) {
        return m_irBuilder->getPtrTy();
    }

    return baseType(type);
}

llvm::Type* CodegenVisitor::declType(Type const& type, std::optional<int> size)
    const {
    if (type.kind == TypeKind::Array) {
        assert(size && "Array declarations must include a size");
        return llvm::ArrayType::get(baseType(type), *size);
    }

    return baseType(type);
}

llvm::Type* CodegenVisitor::baseType(Type const& type) const {
    switch (type.base) {
    case PrimitiveType::Void:
        return m_irBuilder->getVoidTy();
    case PrimitiveType::Int:
        return m_irBuilder->getInt32Ty();
    case PrimitiveType::Float:
        return m_irBuilder->getFloatTy();
    case PrimitiveType::Bool:
        return m_irBuilder->getInt1Ty();
    }
}

/***********************************************************************/

void CodegenVisitor::visit(ProgramNode& node) {
    codegenBuiltins();

    for (auto& decl : node.declarations) {
        decl->accept(*this);
    }
}

/***********************************************************************/

template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};

void CodegenVisitor::visit(Declaration& node) {
    auto var_handler = [this, &node](Declaration::Variable& var) {
        assert(
            node.type.kind != TypeKind::Array &&
            "Variable declarations should have primative type");
        if (node.nest_level == 0) {
            Constant* init_value;
            switch (node.type.base) {
            case PrimitiveType::Void:
                assert(
                    node.type.base != PrimitiveType::Void &&
                    "Variable declarations can not have void type");
                return;
            case PrimitiveType::Int:
                init_value = llvm::ConstantInt::get(
                    useType(Types::Int), 0, /*IsSigned=*/false);
                break;
            case PrimitiveType::Bool:
                init_value = llvm::ConstantInt::getFalse(*m_context);
                break;
            case PrimitiveType::Float:
                init_value = llvm::ConstantFP::getZero(useType(Types::Float));
                break;
            }
            GlobalVariable* global = new GlobalVariable {
                declType(node.type), /*isConstant=*/false,
                GlobalVariable::ExternalLinkage,
                /*Initializer=*/init_value, node.identifier + ".global"};
            node.ir_value = global;
            m_module->insertGlobalVariable(global);
            return;
        }

        node.ir_value = m_irBuilder->CreateAlloca(
            declType(node.type), /*ArraySize=*/nullptr,
            node.identifier + ".local");
    };

    auto arr_handler = [this, &node](Declaration::Array& arr) {
        assert(
            node.type.kind == TypeKind::Array &&
            "Array declarations should have array type");
        if (node.nest_level == 0) {
            GlobalVariable* global = new GlobalVariable {
                declType(node.type, arr.size), /*isConstant=*/false,
                GlobalVariable::ExternalLinkage,
                /*Initializer=*/
                llvm::ConstantAggregateZero::get(declType(node.type, arr.size)),
                node.identifier + ".global"};
            node.ir_value = global;
            m_module->insertGlobalVariable(global);
            return;
        }

        node.ir_value = m_irBuilder->CreateAlloca(
            declType(node.type, arr.size), /*ArraySize=*/nullptr,
            node.identifier + ".local");
    };

    auto param_handler = [](Declaration::Parameter& param) {
        // Do nothing
        // Handled by parent function
    };

    auto func_handler = [this, &node](Declaration::Function& func) {
        std::vector<llvm::Type*> param_types;
        for (auto& param : func.parameters) {
            param_types.push_back(useType(param->type));
        }

        FunctionType* func_type {FunctionType::get(
            useType(node.type), param_types, /*isVarArg=*/false)};
        Function* ir_func {Function::Create(
            func_type, Function::ExternalLinkage, node.identifier, *m_module)};

        node.ir_value = ir_func;

        BasicBlock* preamble {
            BasicBlock::Create(*m_context, "preamble", ir_func)};
        m_irBuilder->SetInsertPoint(preamble);

        for (auto [param, arg] : llvm::zip(func.parameters, ir_func->args())) {
            arg.setName(param->identifier);
            // %param.local = alloca <type>
            param->ir_value = m_irBuilder->CreateAlloca(
                useType(param->type), /*ArraySize=*/nullptr,
                /*Name=*/param->identifier + ".param");
            // store <type> %param, ptr %param.local
            m_irBuilder->CreateStore(&arg, param->ir_value);
        }

        BasicBlock* body {BasicBlock::Create(*m_context, "body", ir_func)};
        m_irBuilder->CreateBr(body);
        m_irBuilder->SetInsertPoint(body);
        func.body->accept(*this);

        // If the function implicitly returns, make sure to return
        if (!*func.body->always_returns) {
            assert(
                node.type == Types::Void &&
                "Only void functions should implicitly return");
            m_irBuilder->CreateRetVoid();
        }

        llvm::verifyFunction(*ir_func);
    };

    std::visit(
        overloaded {var_handler, arr_handler, param_handler, func_handler},
        node.kind);
}

/***********************************************************************/

void CodegenVisitor::visit(CompoundStatementNode& node) {
    for (auto& decl : node.local_decls) {
        decl->accept(*this);
    }

    for (auto& stmt : node.statements) {
        stmt->accept(*this);
        // stop genning code when the block definitly returns
        if (*stmt->always_returns) {
            break;
        }
    }
}

void CodegenVisitor::visit(IfStatementNode& node) {
    node.condition->accept(*this);
    assert(
        node.condition->ir_value &&
        "Expression should have a Value after visit");

    Function* func {m_irBuilder->GetInsertBlock()->getParent()};

    BasicBlock* then_block {BasicBlock::Create(*m_context, "then", func)};

    // if the if always returns, there is no need to merge
    BasicBlock* merge_block;
    if (*node.always_returns) {
        merge_block = nullptr;
    } else {
        merge_block = BasicBlock::Create(*m_context, "post");
    }

    // if the function always returns, it will have an else, so the control flow
    // will never be incomplete
    BasicBlock* else_block;
    if (node.else_stmt) {
        else_block = BasicBlock::Create(*m_context, "else");
    } else {
        else_block = merge_block;
    }

    // br i1 <condition>, label <then_block>, label <else_block>
    m_irBuilder->CreateCondBr(node.condition->ir_value, then_block, else_block);

    // Write then block
    m_irBuilder->SetInsertPoint(then_block);
    node.then_stmt->accept(*this);
    // if the then statement doesn't return, add the branch to post
    if (!*node.then_stmt->always_returns) {
        m_irBuilder->CreateBr(merge_block);
    }

    // Write else block
    if (node.else_stmt) {
        func->insert(func->end(), else_block);
        m_irBuilder->SetInsertPoint(else_block);
        node.else_stmt->accept(*this);
        // if the else statement doesn't return, add the branch to post
        if (!*node.else_stmt->always_returns) {
            m_irBuilder->CreateBr(merge_block);
        }
    }

    // if the if doesn't always return, add merge block
    if (!*node.always_returns) {
        func->insert(func->end(), merge_block);
        m_irBuilder->SetInsertPoint(merge_block);
    }
}

void CodegenVisitor::visit(WhileStatementNode& node) {
    Function* func {m_irBuilder->GetInsertBlock()->getParent()};

    BasicBlock* check {BasicBlock::Create(*m_context, "check", func)};
    BasicBlock* loop {BasicBlock::Create(*m_context, "loop")};
    BasicBlock* post {BasicBlock::Create(*m_context, "post")};

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
    // if the loop body always returns, don't add branch to check
    if (!*node.body->always_returns) {
        // br label <check> ; returns to check to continue loop
        m_irBuilder->CreateBr(check);
    }

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

void CodegenVisitor::visit(AssignmentExpressionNode& node) {
    node.expression->accept(*this);
    assert(
        node.expression->ir_value &&
        "Expression should have a value after visit");
    node.variable->accept(*this);
    assert(node.variable->ir_value && "Variable should get lvalue after visit");
    // value is set to the instruction as assigns return void
    node.ir_value = m_irBuilder->CreateStore(
        node.expression->ir_value, node.variable->ir_value);
}

void CodegenVisitor::visit(VariableExpressionNode& node) {
    // Inderection must be removed for array parameters
    if (node.type->kind == TypeKind::Array &&
        std::holds_alternative<Declaration::Parameter>(node.referent->kind)) {
        node.ir_value = m_irBuilder->CreateLoad(
            m_irBuilder->getPtrTy(), node.referent->ir_value);
        return;
    }

    // lvalues have the same value as their declaration for locals and globals
    // (the pointer to the var in memory)
    node.ir_value = node.referent->ir_value;
}

void CodegenVisitor::visit(SubscriptExpressionNode& node) {
    node.index->accept(*this);
    assert(
        node.index->ir_value && "Expression should have a value after visit");

    Value* data_ptr;
    if (std::holds_alternative<Declaration::Parameter>(node.referent->kind)) {
        data_ptr = m_irBuilder->CreateLoad(
            m_irBuilder->getPtrTy(), node.referent->ir_value);
    } else {
        data_ptr = node.referent->ir_value;
    }

    node.ir_value = m_irBuilder->CreateGEP(
        useType(*node.type), data_ptr, node.index->ir_value);
}

void CodegenVisitor::visit(ImplicitCastNode& node) {
    node.lvalue->accept(*this);
    assert(
        node.lvalue->ir_value && "Expression should have a value after visit");
    node.ir_value =
        m_irBuilder->CreateLoad(useType(*node.type), node.lvalue->ir_value);
}

void CodegenVisitor::visit(CallExpressionNode& node) {
    std::vector<Value*> args;
    for (auto& arg : node.arguments) {
        arg->accept(*this);
        assert(arg->ir_value && "Expression should have a value after visit");
        args.push_back(arg->ir_value);
    }

    // Referent of CallExpr should be a function
    Function* func {static_cast<Function*>(node.referent->ir_value)};
    node.ir_value = m_irBuilder->CreateCall(func, args);
}

void CodegenVisitor::visit(AdditiveExpressionNode& node) {
    node.left->accept(*this);
    assert(node.left->ir_value && "Expression should have a value after visit");
    node.right->accept(*this);
    assert(
        node.right->ir_value && "Expression should have a value after visit");

    if (*node.type == Types::Int) {
        switch (node.operation) {
        case AdditiveOp::PLUS:
            node.ir_value = m_irBuilder->CreateAdd(
                node.left->ir_value, node.right->ir_value);
            break;
        case AdditiveOp::MINUS:
            node.ir_value = m_irBuilder->CreateSub(
                node.left->ir_value, node.right->ir_value);
            break;
        }
        return;
    }

    if (*node.type == Types::Float) {
        switch (node.operation) {
        case AdditiveOp::PLUS:
            node.ir_value = m_irBuilder->CreateFAdd(
                node.left->ir_value, node.right->ir_value);
            break;
        case AdditiveOp::MINUS:
            node.ir_value = m_irBuilder->CreateFSub(
                node.left->ir_value, node.right->ir_value);
            break;
        }
        return;
    }
}

void CodegenVisitor::visit(MultiplicativeExpressionNode& node) {
    node.left->accept(*this);
    assert(node.left->ir_value && "Expression should have a value after visit");
    node.right->accept(*this);
    assert(
        node.right->ir_value && "Expression should have a value after visit");

    if (*node.type == Types::Int) {
        switch (node.operation) {
        case MultiplicativeOp::TIMES:
            node.ir_value = m_irBuilder->CreateMul(
                node.left->ir_value, node.right->ir_value);
            break;
        case MultiplicativeOp::DIVIDE:
            node.ir_value = m_irBuilder->CreateSDiv(
                node.left->ir_value, node.right->ir_value);
            break;
        case MultiplicativeOp::MOD:
            node.ir_value = m_irBuilder->CreateSRem(
                node.left->ir_value, node.right->ir_value);
            break;
        }
        return;
    }

    if (*node.type == Types::Float) {
        switch (node.operation) {
        case MultiplicativeOp::TIMES:
            node.ir_value = m_irBuilder->CreateFMul(
                node.left->ir_value, node.right->ir_value);
            break;
        case MultiplicativeOp::DIVIDE:
            node.ir_value = m_irBuilder->CreateFDiv(
                node.left->ir_value, node.right->ir_value);
            break;
        case MultiplicativeOp::MOD:
            node.ir_value = m_irBuilder->CreateFRem(
                node.left->ir_value, node.right->ir_value);
            break;
        }
        return;
    }
}

void CodegenVisitor::visit(RelationalExpressionNode& node) {
    node.left->accept(*this);
    assert(node.left->ir_value && "Expression should have a value after visit");
    node.right->accept(*this);
    assert(
        node.right->ir_value && "Expression should have a value after visit");

    assert(
        *node.left->type == *node.right->type &&
        "Relational expression must compare args of the same type");
    auto type {*node.left->type};

    if (type == Types::Int || type == Types::Bool) {
        switch (node.operation) {
        case RelationalOp::EQ:
            node.ir_value = m_irBuilder->CreateICmp(
                llvm::CmpInst::ICMP_EQ, node.left->ir_value,
                node.right->ir_value);
            break;
        case RelationalOp::NEQ:
            node.ir_value = m_irBuilder->CreateICmp(
                llvm::CmpInst::ICMP_NE, node.left->ir_value,
                node.right->ir_value);
            break;
        case RelationalOp::LT:
            node.ir_value = m_irBuilder->CreateICmp(
                llvm::CmpInst::ICMP_SLT, node.left->ir_value,
                node.right->ir_value);
            break;
        case RelationalOp::LTE:
            node.ir_value = m_irBuilder->CreateICmp(
                llvm::CmpInst::ICMP_SLE, node.left->ir_value,
                node.right->ir_value);
            break;
        case RelationalOp::GT:
            node.ir_value = m_irBuilder->CreateICmp(
                llvm::CmpInst::ICMP_SGT, node.left->ir_value,
                node.right->ir_value);
            break;
        case RelationalOp::GTE:
            node.ir_value = m_irBuilder->CreateICmp(
                llvm::CmpInst::ICMP_SGE, node.left->ir_value,
                node.right->ir_value);
            break;
        }
        return;
    }

    if (type == Types::Float) {
        switch (node.operation) {
        case RelationalOp::EQ:
            node.ir_value = m_irBuilder->CreateFCmp(
                llvm::CmpInst::FCMP_OEQ, node.left->ir_value,
                node.right->ir_value);
            break;
        case RelationalOp::NEQ:
            node.ir_value = m_irBuilder->CreateFCmp(
                llvm::CmpInst::FCMP_ONE, node.left->ir_value,
                node.right->ir_value);
            break;
        case RelationalOp::LT:
            node.ir_value = m_irBuilder->CreateFCmp(
                llvm::CmpInst::FCMP_OLT, node.left->ir_value,
                node.right->ir_value);
            break;
        case RelationalOp::LTE:
            node.ir_value = m_irBuilder->CreateFCmp(
                llvm::CmpInst::FCMP_OLE, node.left->ir_value,
                node.right->ir_value);
            break;
        case RelationalOp::GT:
            node.ir_value = m_irBuilder->CreateFCmp(
                llvm::CmpInst::FCMP_OGT, node.left->ir_value,
                node.right->ir_value);
            break;
        case RelationalOp::GTE:
            node.ir_value = m_irBuilder->CreateFCmp(
                llvm::CmpInst::FCMP_UGE, node.left->ir_value,
                node.right->ir_value);
            break;
        }
        return;
    }
}

void CodegenVisitor::visit(IntegerLiteralExpressionNode& node) {
    node.ir_value =
        llvm::ConstantInt::getSigned(m_irBuilder->getInt32Ty(), node.value);
}

void CodegenVisitor::visit(FloatLiteralExpressionNode& node) {
    node.ir_value =
        llvm::ConstantFP::get(m_irBuilder->getFloatTy(), node.value);
}

void CodegenVisitor::visit(BoolLiteralExpressionNode& node) {
    node.ir_value = llvm::ConstantInt::getBool(*m_context, node.value);
}

/***********************************************************************/

void CodegenVisitor::codegenBuiltins() {
    // Global format string for printf and scanf
    auto format_str {m_irBuilder->CreateGlobalString(
        "%d\n", "format_str", /*AddressSpace=*/0, m_module.get())};

    // Declare printf and scanf
    auto printf_type {FunctionType::get(
        m_irBuilder->getInt32Ty(), m_irBuilder->getPtrTy(),
        /*isVarArg=*/true)};
    auto printf {Function::Create(
        printf_type, Function::ExternalLinkage, "printf", *m_module)};

    auto scanf_type {FunctionType::get(
        m_irBuilder->getInt32Ty(), m_irBuilder->getPtrTy(), /*isVarArg=*/true)};
    auto scanf {Function::Create(
        scanf_type, Function::ExternalLinkage, "scanf", *m_module)};

    // Gen input()
    {
        auto func_type {
            FunctionType::get(m_irBuilder->getInt32Ty(), /*isVarArg=*/false)};
        auto input {Function::Create(
            func_type, Function::ExternalLinkage, "input", *m_module)};
        g_builtins[0]->ir_value = input;

        BasicBlock* body {BasicBlock::Create(*m_context, "body", input)};
        m_irBuilder->SetInsertPoint(body);
        // memory location to read int to
        auto buffer {m_irBuilder->CreateAlloca(
            m_irBuilder->getInt32Ty(), /*ArraySize=*/nullptr, "buffer")};
        m_irBuilder->CreateCall(scanf_type, scanf, {format_str, buffer});
        auto ret_val {
            m_irBuilder->CreateLoad(m_irBuilder->getInt32Ty(), buffer)};
        m_irBuilder->CreateRet(ret_val);
    }

    // Gen output(int)
    {
        std::vector<llvm::Type*> params {m_irBuilder->getInt32Ty()};
        auto output_type {FunctionType::get(
            m_irBuilder->getVoidTy(), params, /*isVarArg=*/false)};
        auto output {Function::Create(
            output_type, Function::ExternalLinkage, "output", *m_module)};
        g_builtins[1]->ir_value = output;

        output->getArg(0)->setName("value");

        BasicBlock* body {BasicBlock::Create(*m_context, "body", output)};
        m_irBuilder->SetInsertPoint(body);
        m_irBuilder->CreateCall(
            printf_type, printf, {format_str, output->getArg(0)});
        m_irBuilder->CreateRetVoid();
    }
}

/***********************************************************************/
