#include "SemanticVisitor.hpp"

#include <algorithm>
#include <cassert>
#include <iomanip>
#include <memory>
#include <sstream>

/***********************************************************************/

bool SemanticVisitor::isValid() const {
    return m_errors.empty();
}

const std::vector<SemanticError>& SemanticVisitor::errors() const {
    return m_errors;
}

void SemanticVisitor::addError(SemanticError error) {
    m_errors.push_back(error);
    // Sorts errors in order of occurance
    std::ranges::sort(m_errors, [](SemanticError a, SemanticError b) {
        if (a.location().line_num < b.location().line_num) {
            return true;
        }

        if (a.location().line_num > b.location().line_num) {
            return false;
        }

        return a.location().col_num < b.location().col_num;
    });
}

/***********************************************************************/

void SemanticVisitor::visit(ProgramNode& node) {
    for (auto& decl : node.declarations) {
        // Check if decl is main func, and that it is not the last element
        if (decl->identifier == "main" && &decl != &node.declarations.back()) {
            addError(SemanticError::earlyMain(*decl));
        }

        decl->accept(*this);
    }
}

void SemanticVisitor::visit(FunctionDeclarationNode& node) {
    assert(
        node.type.is_function &&
        "Function declarations should be marked as function");
    assert(
        node.type.type.kind != TypeKind::Array &&
        "Current grammar does not allow for array returning function");

    for (auto& param : node.parameters) {
        param->accept(*this);
    }

    m_currentFunction = &node;
    node.function_body->accept(*this);
    m_currentFunction = nullptr;

    assert(
        node.function_body->always_returns &&
        "Return nature of statement should be calculated by visit");
    // Non-void functions must return
    if (!*node.function_body->always_returns && node.type.type != Types::Void) {
        addError(SemanticError::nonReturningFunction(node));
    }
}

void SemanticVisitor::visit(VariableDeclarationNode& node) {
    assert(
        !node.type.is_function &&
        "Variable declarations should not be marked as function");
    assert(
        node.type.type.kind == TypeKind::Primitive &&
        "Primative variable declarations should not have array type");

    if (node.type.type == Types::Void) {
        addError(SemanticError::voidVariable(node));
    }
}

void SemanticVisitor::visit(ArrayDeclarationNode& node) {
    assert(
        !node.type.is_function &&
        "Array declarations should not be marked as function");
    assert(
        node.type.type.kind == TypeKind::Array &&
        "Array declarations should have array type");

    // Asserts force type kind to be array
    if (node.type.type.base == PrimitiveType::Void) {
        addError(SemanticError::voidVariable(node));
    }

    if (node.size <= 0) {
        addError(SemanticError::nonPositiveArraySize(node));
    }
}

void SemanticVisitor::visit(ParameterNode& node) {
    assert(
        !node.type.is_function &&
        "Parameters should not be marked as function");

    // Type kind is irrelevant for bad void checking
    if (node.type.type.base == PrimitiveType::Void) {
        addError(SemanticError::voidParam(node));
    }
}

void SemanticVisitor::visit(CompoundStatementNode& node) {
    for (auto& decl : node.local_decls) {
        decl->accept(*this);
    }

    for (auto& stmt : node.statements) {
        stmt->accept(*this);
        assert(
            stmt->always_returns &&
            "Return nature of statement should be calculated by visit");
    }

    // Compound Statement always returns if any of its statements always return
    node.always_returns = std::ranges::any_of(
        node.statements, [](auto& stmt) { return *stmt->always_returns; });
}

void SemanticVisitor::visit(IfStatementNode& node) {
    node.condition->accept(*this);
    assert(
        node.condition->type &&
        "Expression type should be calculated by visit");
    if (*node.condition->type != Types::Bool) {
        addError(SemanticError::invalidCondition(node));
    }

    node.then_stmt->accept(*this);
    assert(
        node.then_stmt->always_returns &&
        "Return nature of statement should be calculated by visit");

    // If statement always returns if and only if else exists and then and
    // else always return
    if (node.else_stmt) {
        node.else_stmt->accept(*this);
        assert(
            node.else_stmt->always_returns &&
            "Return nature of statement should be calculated by visit");

        node.always_returns =
            *node.then_stmt->always_returns && *node.else_stmt->always_returns;
        return;
    }

    node.always_returns = false;
}

void SemanticVisitor::visit(WhileStatementNode& node) {
    node.condition->accept(*this);
    assert(
        node.condition->type &&
        "Expression type should be calculated by visit");
    if (*node.condition->type != Types::Bool) {
        addError(SemanticError::invalidCondition(node));
    }

    node.body->accept(*this);

    // Without further static analyis while loops can not be said to always
    // return
    node.always_returns = false;
}

void SemanticVisitor::visit(ReturnStatementNode& node) {
    assert(
        m_currentFunction &&
        "Return statements should only occur within functions");

    Type func_type {m_currentFunction->type.type};

    if (!node.expression) {
        if (func_type != Types::Void) {
            addError(SemanticError::badReturn(node, *m_currentFunction));
        }
    } else {
        node.expression->accept(*this);
        assert(
            node.expression->type &&
            "Expression type should be calculated by visit");
        if (func_type != *node.expression->type) {
            addError(SemanticError::badReturn(node, *m_currentFunction));
        }
    }

    node.always_returns = true;
}

void SemanticVisitor::visit(ExpressionStatementNode& node) {
    if (node.expr) {
        node.expr->accept(*this);
    }

    // Expression statements never return
    node.always_returns = false;
}

void SemanticVisitor::visit(AssignmentExpressionNode& node) {
    node.variable->accept(*this);
    assert(
        node.variable->type && "Expression type should be calculated by visit");

    if (node.variable->type->kind == TypeKind::Array) {
        addError(SemanticError::arrayAssignment(node));
    }

    node.expression->accept(*this);
    assert(
        node.expression->type &&
        "Expression type should be calculated by visit");

    if (node.variable->type != node.expression->type) {
        addError(SemanticError::mismatchAssignment(node));
    }

    // Set result type to void type to ban chained assignments
    node.type = Types::Void;
}

void SemanticVisitor::visit(VariableExpressionNode& node) {
    assert(
        node.referent &&
        "Variable expression nodes should be linked to their declarations");

    if (node.referent->type.is_function) {
        addError(SemanticError::functionAsVariable(node));
    }

    node.type = node.referent->type.type;
}

void SemanticVisitor::visit(SubscriptExpressionNode& node) {
    assert(
        node.referent &&
        "Subscript nodes should be linked to their declarations");

    if (node.referent->type.is_function) {
        addError(SemanticError::functionAsVariable(node));
    }

    if (node.referent->type.type.kind != TypeKind::Array) {
        addError(SemanticError::indexNonArray(node));
    }

    node.index->accept(*this);
    assert(node.index->type && "Expression type should be calculated by visit");

    if (*node.index->type != Types::Int) {
        addError(SemanticError::badIndex(node));
    }

    node.type = Type {TypeKind::Primitive, node.referent->type.type.base};
}

void SemanticVisitor::visit(ImplicitCastNode& node) {
    node.lvalue->accept(*this);
    assert(
        node.lvalue->type && "Expression type should be calculated by visit");
    node.type = node.lvalue->type;
}

void SemanticVisitor::visit(CallExpressionNode& node) {
    assert(
        node.referent &&
        "Function call nodes should be linked to their declarations");

    if (!node.referent->type.is_function) {
        addError(SemanticError::variableAsFunction(node));
        for (auto& arg : node.arguments) {
            arg->accept(*this);
        }
        node.type = node.referent->type.type;
        return;
    }

    auto function {
        static_cast<FunctionDeclarationNode const*>(node.referent.get())};

    if (function->parameters.size() != node.arguments.size()) {
        addError(SemanticError::wrongArgumentCount(node, *function));
        for (auto& arg : node.arguments) {
            arg->accept(*this);
        }
        node.type = node.referent->type.type;
        return;
    }

    for (unsigned i {0}; i < node.arguments.size(); i++) {
        node.arguments[i]->accept(*this);
        assert(
            node.arguments[i]->type &&
            "Expression type should be calculated by visit");
        Type arg_type {*node.arguments[i]->type};
        Type param_type {function->parameters[i]->type.type};

        if (arg_type != param_type) {
            addError(SemanticError::wrongArgumentType(node, *function, i));
        }
    }

    node.type = node.referent->type.type;
}

void SemanticVisitor::visit(AdditiveExpressionNode& node) {
    node.left->accept(*this);
    assert(node.left->type && "Expression type should be calculated by visit");
    Type left_type {*node.left->type};

    node.right->accept(*this);
    assert(node.right->type && "Expression type should be calculated by visit");
    Type right_type {*node.right->type};

    if (left_type.kind == TypeKind::Array || left_type == Types::Void) {
        addError(SemanticError::invalidOperation(node, left_type, right_type));
        node.type = left_type;
        return;
    }

    if (right_type.kind == TypeKind::Array || right_type == Types::Void) {
        addError(SemanticError::invalidOperation(node, left_type, right_type));
        node.type = right_type;
        return;
    }

    if (left_type != right_type) {
        addError(SemanticError::invalidOperation(node, left_type, right_type));
    }

    node.type = left_type;
}

void SemanticVisitor::visit(MultiplicativeExpressionNode& node) {
    node.left->accept(*this);
    assert(node.left->type && "Expression type should be calculated by visit");
    Type left_type {*node.left->type};

    node.right->accept(*this);
    assert(node.right->type && "Expression type should be calculated by visit");
    Type right_type {*node.right->type};

    if (left_type.kind == TypeKind::Array || left_type == Types::Void) {
        addError(SemanticError::invalidOperation(node, left_type, right_type));
        node.type = left_type;
        return;
    }

    if (right_type.kind == TypeKind::Array || right_type == Types::Void) {
        addError(SemanticError::invalidOperation(node, left_type, right_type));
        node.type = right_type;
        return;
    }

    if (left_type != right_type) {
        addError(SemanticError::invalidOperation(node, left_type, right_type));
    }

    node.type = left_type;
}

void SemanticVisitor::visit(RelationalExpressionNode& node) {
    node.left->accept(*this);
    assert(node.left->type && "Expression type should be calculated by visit");
    Type left_type {*node.left->type};

    node.right->accept(*this);
    assert(node.right->type && "Expression type should be calculated by visit");
    Type right_type {*node.right->type};

    if (left_type.kind == TypeKind::Array || left_type == Types::Void) {
        addError(SemanticError::invalidOperation(node, left_type, right_type));
        node.type = Types::Bool;
        return;
    }

    if (right_type.kind == TypeKind::Array || right_type == Types::Void) {
        addError(SemanticError::invalidOperation(node, left_type, right_type));
        node.type = Types::Bool;
        return;
    }

    if (left_type != right_type) {
        addError(SemanticError::invalidOperation(node, left_type, right_type));
    }

    // Only == and != are defined for bool types
    if (left_type == Types::Bool && !(node.operation == RelationalOp::EQ ||
                                      node.operation == RelationalOp::NEQ)) {
        addError(SemanticError::invalidOperation(node, left_type, right_type));
    }

    node.type = Types::Bool;
}

void SemanticVisitor::visit(IntegerLiteralExpressionNode& node) {
    node.type = Types::Int;
}

void SemanticVisitor::visit(FloatLiteralExpressionNode& node) {
    node.type = Types::Float;
}

void SemanticVisitor::visit(BoolLiteralExpressionNode& node) {
    node.type = Types::Bool;
}

/***********************************************************************/

SemanticError::SemanticError(std::string error_message, Location loc)
    : m_message {error_message}, m_location {loc} {}

const std::string_view SemanticError::message() const {
    return m_message;
}

Location SemanticError::location() const {
    return m_location;
}

/***********************************************************************/

SemanticError SemanticError::earlyMain(DeclarationNode const& decl) {
    std::stringstream message_buffer;
    message_buffer << "Early main declaration at line: " << decl.loc.line_num
                   << ", col: " << decl.loc.col_num << ".\n"
                   << "  main must be the last function declared.";
    return {message_buffer.str(), decl.loc};
}

SemanticError SemanticError::voidVariable(
    VariableDeclarationNode const& varDecl) {
    std::stringstream message_buffer;
    message_buffer << "Error: variable " << std::quoted(varDecl.identifier)
                   << " declared with type " << varDecl.type.type << ".\n"
                   << "  line: " << varDecl.loc.line_num
                   << ", col: " << varDecl.loc.col_num << '.';
    return {message_buffer.str(), varDecl.loc};
}

SemanticError SemanticError::voidParam(ParameterNode const& paramDecl) {
    std::stringstream message_buffer;
    message_buffer << "Error: parameter " << std::quoted(paramDecl.identifier)
                   << " declared with type " << paramDecl.type.type << ".\n"
                   << "  line: " << paramDecl.loc.line_num
                   << ", col: " << paramDecl.loc.col_num << '.';
    return {message_buffer.str(), paramDecl.loc};
}

SemanticError SemanticError::nonPositiveArraySize(
    ArrayDeclarationNode const& arrDecl) {
    std::stringstream message_buffer;
    message_buffer << "Error: array " << std::quoted(arrDecl.identifier)
                   << " declared with non-positive size " << arrDecl.size
                   << '\n'
                   << "  line: " << arrDecl.loc.line_num
                   << ", col: " << arrDecl.loc.col_num << '.';
    return {message_buffer.str(), arrDecl.loc};
}

SemanticError SemanticError::invalidCondition(IfStatementNode const& ifStmt) {
    std::stringstream message_buffer;
    message_buffer << "Error: invalid condition for if statement.\n"
                   << "  If condition must be an bool.\n"
                   << "  line: " << ifStmt.condition->loc.line_num
                   << ", col: " << ifStmt.condition->loc.col_num << '.';
    return {message_buffer.str(), ifStmt.condition->loc};
}

SemanticError SemanticError::invalidCondition(
    WhileStatementNode const& whileStmt) {
    std::stringstream message_buffer;
    message_buffer << "Error: invalid condition for while statement.\n"
                   << "  While condition must be an bool.\n"
                   << "  line: " << whileStmt.condition->loc.line_num
                   << ", col: " << whileStmt.condition->loc.col_num << '.';
    return {message_buffer.str(), whileStmt.condition->loc};
}

SemanticError SemanticError::badReturn(
    ReturnStatementNode const& ret,
    FunctionDeclarationNode const& func) {
    std::stringstream message_buffer;
    message_buffer << "Error: Incorrect return type\n"
                   << "  In function " << std::quoted(func.identifier)
                   << " returning " << func.type.type
                   << " (line: " << func.loc.line_num
                   << ", col: " << func.loc.col_num << ")\n"
                   << "  Return statement returns ";

    if (ret.expression) {
        assert(
            ret.expression->type &&
            "Type should already be calculated for bad return error");
        message_buffer << *ret.expression->type;
    } else {
        message_buffer << Type {TypeKind::Primitive, PrimitiveType::Void};
    }

    message_buffer << " (line: " << ret.loc.line_num
                   << ", col: " << ret.loc.col_num << ")";

    return {message_buffer.str(), ret.loc};
}

SemanticError SemanticError::arrayAssignment(
    AssignmentExpressionNode const& assignExpr) {
    std::stringstream message_buffer;
    message_buffer << "Error: Assigning to array "
                   << std::quoted(assignExpr.variable->identifier)
                   << " (loc: " << assignExpr.loc.line_num
                   << ", col: " << assignExpr.loc.col_num << ")\n";
    return {message_buffer.str(), assignExpr.loc};
}

SemanticError SemanticError::mismatchAssignment(
    AssignmentExpressionNode const& assignExpr) {
    std::stringstream message_buffer;
    message_buffer << "Error: Variable assignment type mismatch\n"
                   << "  Attempting to assign " << *assignExpr.expression->type
                   << " to " << *assignExpr.variable->type << ' '
                   << std::quoted(assignExpr.variable->identifier)
                   << "\n  line: " << assignExpr.loc.line_num
                   << ", col: " << assignExpr.loc.col_num;
    return {message_buffer.str(), assignExpr.loc};
}

SemanticError SemanticError::functionAsVariable(
    VariableExpressionNode const& varExpr) {
    std::stringstream message_buffer;
    message_buffer << "Error: Function name used as variable\n"
                   << "  Function " << std::quoted(varExpr.identifier)
                   << " used as variable (line:" << varExpr.loc.line_num
                   << ", col: " << varExpr.loc.col_num << ")";
    return {message_buffer.str(), varExpr.loc};
}

SemanticError SemanticError::variableAsFunction(
    CallExpressionNode const& callExpr) {
    std::stringstream message_buffer;
    message_buffer << "Error: Variable name used as function\n"
                   << "  Variable " << std::quoted(callExpr.identifier)
                   << " used as a function (line:" << callExpr.loc.line_num
                   << ", col: " << callExpr.loc.col_num << ")";
    return {message_buffer.str(), callExpr.loc};
}

SemanticError SemanticError::wrongArgumentCount(
    CallExpressionNode const& callExpr,
    FunctionDeclarationNode const& func) {
    assert(callExpr.arguments.size() != func.parameters.size());

    std::stringstream message_buffer;
    message_buffer << "Error: ";
    if (callExpr.arguments.size() < func.parameters.size()) {
        message_buffer << "Too few arguments for function "
                       << std::quoted(func.identifier) << "\n  "
                       << func.parameters.size() << " required. ";
    } else {
        message_buffer << "Too many arguments for function "
                       << std::quoted(func.identifier) << "\n  "
                       << func.parameters.size() << " required. ";
    }

    message_buffer << callExpr.arguments.size() << " provided.\n"
                   << "  (line" << callExpr.loc.line_num
                   << ", col: " << callExpr.loc.col_num << ")\n";

    return {message_buffer.str(), callExpr.loc};
}

SemanticError SemanticError::wrongArgumentType(
    CallExpressionNode const& callExpr,
    FunctionDeclarationNode const& func,
    unsigned arg_num) {
    assert(callExpr.arguments.size() == func.parameters.size());
    assert(arg_num < callExpr.arguments.size());
    ExpressionNode const& badArg {*callExpr.arguments[arg_num]};
    ParameterNode const& badParam {*func.parameters[arg_num]};
    std::stringstream messgae_buffer;
    messgae_buffer << "Error: Incorrect argument type for function "
                   << std::quoted(func.identifier) << "\n  Argument "
                   << badParam.identifier << " expected " << badParam.type.type
                   << ". Received " << *badArg.type
                   << ".\n  (line:" << badArg.loc.line_num
                   << ", col: " << badArg.loc.col_num << ")";
    return {messgae_buffer.str(), badArg.loc};
}

SemanticError SemanticError::invalidOperation(
    AdditiveExpressionNode const& addExpr,
    Type left,
    Type right) {
    std::stringstream message_buffer;
    message_buffer << "Error: Invalid types for " << addExpr.operation << "\n  "
                   << left << " " << addExpr.operation << " " << right
                   << " is undefined\n  loc: " << addExpr.loc.line_num
                   << ", col: " << addExpr.loc.col_num;
    return {message_buffer.str(), addExpr.loc};
}

SemanticError SemanticError::invalidOperation(
    MultiplicativeExpressionNode const& mulExpr,
    Type left,
    Type right) {
    std::stringstream message_buffer;
    message_buffer << "Error: Invalid types for " << mulExpr.operation << "\n  "
                   << left << " " << mulExpr.operation << " " << right
                   << " is undefined\n  loc: " << mulExpr.loc.line_num
                   << ", col: " << mulExpr.loc.col_num;
    return {message_buffer.str(), mulExpr.loc};
}

SemanticError SemanticError::invalidOperation(
    RelationalExpressionNode const& relExpr,
    Type left,
    Type right) {
    std::stringstream message_buffer;
    message_buffer << "Error: Invalid types for " << relExpr.operation << "\n  "
                   << left << " " << relExpr.operation << " " << right
                   << " is undefined\n  loc: " << relExpr.loc.line_num
                   << ", col: " << relExpr.loc.col_num;
    return {message_buffer.str(), relExpr.loc};
}

SemanticError SemanticError::indexNonArray(
    SubscriptExpressionNode const& subscriptExpr) {
    std::stringstream message_buffer;
    message_buffer << "Error: Indexed non-array symbol "
                   << std::quoted(subscriptExpr.identifier)
                   << "\n  loc: " << subscriptExpr.loc.line_num
                   << ", col: " << subscriptExpr.loc.col_num;
    return {message_buffer.str(), subscriptExpr.loc};
}

SemanticError SemanticError::badIndex(SubscriptExpressionNode const& subExpr) {
    Type index_type {*subExpr.index->type};
    std::stringstream message_buffer;
    message_buffer << "Error: Non-int Index to array "
                   << std::quoted(subExpr.identifier) << "\n  Expected "
                   << Type {TypeKind::Primitive, PrimitiveType::Int}
                   << ", received " << index_type
                   << "\n  loc: " << subExpr.index->loc.line_num
                   << ", col: " << subExpr.index->loc.col_num;
    return {message_buffer.str(), subExpr.index->loc};
}

SemanticError SemanticError::nonReturningFunction(
    FunctionDeclarationNode const& func) {
    std::stringstream message_buffer;
    message_buffer << "Error: Non-void function "
                   << std::quoted(func.identifier)
                   << " doesn't return in all control paths.\n  loc:"
                   << func.loc.line_num << ", col: " << func.loc.col_num;
    return {message_buffer.str(), func.loc};
}

/***********************************************************************/
