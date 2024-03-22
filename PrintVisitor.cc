#include "PrintVisitor.hpp"
#include "AST.hpp"

#include <map>
#include <ostream>
#include <string_view>

/***********************************************************************/

const std::map<TypeSpecifier, const std::string_view> types {
    {TypeSpecifier::INT, "Int"},
    {TypeSpecifier::VOID, "void"}};

const std::map<AdditiveOp, const std::string_view> add_symbols {
    {AdditiveOp::PLUS, "+"},
    {AdditiveOp::MINUS, "-"}};

const std::map<MultiplicativeOp, const std::string_view> mul_symbols {
    {MultiplicativeOp::TIMES, "*"},
    {MultiplicativeOp::DIVIDE, "/"}};

const std::map<RelationalOp, const std::string_view> rel_symbols {
    {RelationalOp::LT, "<"},  {RelationalOp::LTE, "<="},
    {RelationalOp::GT, ">"},  {RelationalOp::GTE, ">="},
    {RelationalOp::EQ, "=="}, {RelationalOp::NEQ, "!="}};

/***********************************************************************/

PrintVisitor::PrintVisitor(std::ostream& out_stream)
    : m_output {out_stream}, m_currentDepth {0} {}

std::string PrintVisitor::getIndent() const {
    return std::string(2 * m_currentDepth, ' ');
}

struct NestGuard {
    NestGuard(PrintVisitor& pv) : pv {pv} { pv.m_currentDepth++; }

    ~NestGuard() { pv.m_currentDepth--; }

    PrintVisitor& pv;
};

/***********************************************************************/

void PrintVisitor::visit(ProgramNode& node) {
    m_output << "ProgramNode:\n";

    {
        NestGuard guard {*this};

        for (auto& decl : node.declarations) {
            m_output << '\n';
            decl->accept(*this);
        }
    }

    m_output.flush();
}

void PrintVisitor::visit(DeclarationNode& node) {
    node.accept(*this);
}

void PrintVisitor::visit(FunctionDeclarationNode& node) {
    m_output << getIndent() << "Function: " << node.identifier << ": "
             << types.at(node.type.type) << " type\n";

    NestGuard guard {*this};

    for (auto& param : node.parameters) {
        param->accept(*this);
    }

    node.function_body->accept(*this);
}

void PrintVisitor::visit(VariableDeclarationNode& node) {
    m_output << getIndent() << "VariableDeclaration: " << node.identifier
             << ": " << types.at(node.type.type) << " type\n";
}

void PrintVisitor::visit(ArrayDeclarationNode& node) {
    m_output << getIndent() << "VariableDeclaration: " << node.identifier << "["
             << node.size << "]: " << types.at(node.type.type) << " type\n";
}

void PrintVisitor::visit(ParameterNode& node) {
    m_output << getIndent() << "Parameter: " << node.identifier;

    if (node.type.is_array) {
        m_output << "[]";
    }

    m_output << ": " << types.at(node.type.type) << " ";

    if (node.type.is_array) {
        m_output << "array ";
    }

    m_output << "type\n";
}

void PrintVisitor::visit(StatementNode& node) {
    node.accept(*this);
}

void PrintVisitor::visit(CompoundStatementNode& node) {
    m_output << getIndent() << "CompoundStatement:\n";

    NestGuard guard {*this};

    for (auto& decl : node.local_decls) {
        decl->accept(*this);
    }

    for (auto& stmt : node.statements) {
        stmt->accept(*this);
    }
}

void PrintVisitor::visit(IfStatementNode& node) {
    m_output << getIndent() << "If\n";

    NestGuard guard {*this};

    node.condition->accept(*this);

    node.then_stmt->accept(*this);

    if (node.else_stmt) {
        (*node.else_stmt)->accept(*this);
    }
}

void PrintVisitor::visit(WhileStatementNode& node) {
    m_output << getIndent() << "While\n";

    NestGuard guard {*this};

    node.condition->accept(*this);

    node.body->accept(*this);
}

void PrintVisitor::visit(ReturnStatementNode& node) {
    m_output << getIndent() << "Return\n";

    if (node.expression) {
        NestGuard guard {*this};

        (*node.expression)->accept(*this);
    }
}

void PrintVisitor::visit(ExpressionStatementNode& node) {
    m_output << getIndent() << "ExpressionStatement:\n";

    NestGuard guard {*this};

    if (!node.expr) {
        m_output << getIndent() << "Semicolon\n";
    } else {
        (*node.expr)->accept(*this);
    }
}

void PrintVisitor::visit(ExpressionNode& node) {
    node.accept(*this);
}

void PrintVisitor::visit(AssignmentExpressionNode& node) {
    m_output << getIndent() << "Assignment:\n";

    NestGuard guard {*this};

    node.variable->accept(*this);

    node.expression->accept(*this);
}

void PrintVisitor::visit(VariableExpressionNode& node) {
    m_output << getIndent() << "Variable: " << node.identifier << '\n';
}

void PrintVisitor::visit(SubscriptExpressionNode& node) {
    m_output << getIndent() << "Subscript: " << node.identifier << '\n';

    NestGuard guard {*this};

    m_output << getIndent() << "Index:\n";

    {
        NestGuard guard {*this};

        node.index->accept(*this);
    }
}

void PrintVisitor::visit(CallExpressionNode& node) {
    m_output << getIndent() << "FunctionCall: " << node.identifier << '\n';

    if (!node.arguments.empty()) {
        NestGuard guard {*this};

        m_output << getIndent() << "Arguments:\n";

        {
            NestGuard guard {*this};

            for (auto& arg : node.arguments) {
                arg->accept(*this);
            }
        }
    }
}

void PrintVisitor::visit(AdditiveExpressionNode& node) {
    m_output << getIndent()
             << "AdditiveExpression: " << add_symbols.at(node.operation)
             << '\n';

    NestGuard guard {*this};

    m_output << getIndent() << "Left:\n";

    {
        NestGuard guard {*this};

        node.left->accept(*this);
    }

    m_output << getIndent() << "Right:\n";

    {
        NestGuard guard {*this};

        node.right->accept(*this);
    }
}

void PrintVisitor::visit(MultiplicativeExpressionNode& node) {
    m_output << getIndent()
             << "MultiplicativeExpression: " << mul_symbols.at(node.operation)
             << '\n';

    NestGuard guard {*this};

    m_output << getIndent() << "Left:\n";

    {
        NestGuard guard {*this};

        node.left->accept(*this);
    }

    m_output << getIndent() << "Right:\n";

    {
        NestGuard guard {*this};

        node.right->accept(*this);
    }
}

void PrintVisitor::visit(RelationalExpressionNode& node) {
    m_output << getIndent()
             << "RelationalExpression: " << rel_symbols.at(node.operation)
             << '\n';

    NestGuard guard {*this};

    m_output << getIndent() << "Left:\n";

    {
        NestGuard guard {*this};

        node.left->accept(*this);
    }

    m_output << getIndent() << "Right:\n";

    {
        NestGuard guard {*this};

        node.right->accept(*this);
    }
}

void PrintVisitor::visit(IntegerLiteralExpressionNode& node) {
    m_output << getIndent() << "Integer: " << node.value << '\n';
}

/***********************************************************************/
