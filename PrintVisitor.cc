#include "PrintVisitor.hpp"
#include "AST.hpp"

#include <map>
#include <ostream>
#include <string_view>

/***********************************************************************/

const std::map<basic_type, const std::string_view> types {
    {basic_type::INT, "Int"},
    {basic_type::VOID, "void"}};

const std::map<add_op, const std::string_view> add_symbols {
    {add_op::PLUS, "+"},
    {add_op::MINUS, "-"}};

const std::map<mul_op, const std::string_view> mul_symbols {
    {mul_op::TIMES, "*"},
    {mul_op::DIVIDE, "/"}};

const std::map<rel_op, const std::string_view> rel_symbols {
    {rel_op::LT, "<"},   {rel_op::LTE, "<="}, {rel_op::GT, ">"},
    {rel_op::GTE, ">="}, {rel_op::EQ, "=="},  {rel_op::NEQ, "!="}};

/***********************************************************************/

PrintVisitor::PrintVisitor(std::ostream& out_stream)
    : output {out_stream}, current_depth {0} {}

std::string PrintVisitor::indent() const {
    return std::string(2 * current_depth, ' ');
}

struct nest_guard {
    nest_guard(PrintVisitor& pv) : pv {pv} { pv.current_depth++; }

    ~nest_guard() { pv.current_depth--; }

    PrintVisitor& pv;
};

/***********************************************************************/

void PrintVisitor::visit(ProgramNode& node) {
    output << "ProgramNode:\n";

    {
        nest_guard guard {*this};

        for (auto& decl : node.declarations) {
            output << '\n';
            decl->accept(*this);
        }
    }

    output.flush();
}

void PrintVisitor::visit(DeclarationNode& node) {
    node.accept(*this);
}

void PrintVisitor::visit(FunctionDeclarationNode& node) {
    output << indent() << "Function: " << node.identifier << ": "
           << types.at(node.type.type) << " type\n";

    nest_guard guard {*this};

    for (auto& param : node.parameters) {
        param->accept(*this);
    }

    node.function_body->accept(*this);
}

void PrintVisitor::visit(VariableDeclarationNode& node) {
    output << indent() << "VariableDeclaration: " << node.identifier << ": "
           << types.at(node.type.type) << " type\n";
}

void PrintVisitor::visit(ArrayDeclarationNode& node) {
    output << indent() << "VariableDeclaration: " << node.identifier << "["
           << node.size << "]: " << types.at(node.type.type) << " type\n";
}

void PrintVisitor::visit(ParameterNode& node) {
    output << indent() << "Parameter: " << node.identifier;

    if (node.type.is_array) {
        output << "[]";
    }

    output << ": " << types.at(node.type.type) << " ";

    if (node.type.is_array) {
        output << "array ";
    }

    output << "type\n";
}

void PrintVisitor::visit(StatementNode& node) {
    node.accept(*this);
}

void PrintVisitor::visit(CompoundStatementNode& node) {
    output << indent() << "CompoundStatement:\n";

    nest_guard guard {*this};

    for (auto& decl : node.local_decls) {
        decl->accept(*this);
    }

    for (auto& stmt : node.statements) {
        stmt->accept(*this);
    }
}

void PrintVisitor::visit(IfStatementNode& node) {
    output << indent() << "If\n";

    nest_guard guard {*this};

    node.condition->accept(*this);

    node.then_stmt->accept(*this);

    if (node.else_stmt) {
        (*node.else_stmt)->accept(*this);
    }
}

void PrintVisitor::visit(WhileStatementNode& node) {
    output << indent() << "While\n";

    nest_guard guard {*this};

    node.condition->accept(*this);

    node.body->accept(*this);
}

void PrintVisitor::visit(ReturnStatementNode& node) {
    output << indent() << "Return\n";

    if (node.expression) {
        nest_guard guard {*this};

        (*node.expression)->accept(*this);
    }
}

void PrintVisitor::visit(ExpressionStatementNode& node) {
    output << indent() << "ExpressionStatement:\n";

    nest_guard guard {*this};

    if (!node.expr) {
        output << indent() << "Semicolon\n";
    } else {
        (*node.expr)->accept(*this);
    }
}

void PrintVisitor::visit(ExpressionNode& node) {
    node.accept(*this);
}

void PrintVisitor::visit(AssignmentExpressionNode& node) {
    output << indent() << "Assignment:\n";

    nest_guard guard {*this};

    node.variable->accept(*this);

    node.expression->accept(*this);
}

void PrintVisitor::visit(VariableExpressionNode& node) {
    output << indent() << "Variable: " << node.identifier << '\n';
}

void PrintVisitor::visit(SubscriptExpressionNode& node) {
    output << indent() << "Subscript: " << node.identifier << '\n';

    nest_guard guard {*this};

    output << indent() << "Index:\n";

    {
        nest_guard guard {*this};

        node.index->accept(*this);
    }
}

void PrintVisitor::visit(CallExpressionNode& node) {
    output << indent() << "FunctionCall: " << node.identifier << '\n';

    if (!node.arguments.empty()) {
        nest_guard guard {*this};

        output << indent() << "Arguments:\n";

        {
            nest_guard guard {*this};

            for (auto& arg : node.arguments) {
                arg->accept(*this);
            }
        }
    }
}

void PrintVisitor::visit(AdditiveExpressionNode& node) {
    output << indent()
           << "AdditiveExpression: " << add_symbols.at(node.operation) << '\n';

    nest_guard guard {*this};

    output << indent() << "Left:\n";

    {
        nest_guard guard {*this};

        node.left->accept(*this);
    }

    output << indent() << "Right:\n";

    {
        nest_guard guard {*this};

        node.right->accept(*this);
    }
}

void PrintVisitor::visit(multiplicative_expression_node& node) {
    output << indent()
           << "MultiplicativeExpression: " << mul_symbols.at(node.operation)
           << '\n';

    nest_guard guard {*this};

    output << indent() << "Left:\n";

    {
        nest_guard guard {*this};

        node.left->accept(*this);
    }

    output << indent() << "Right:\n";

    {
        nest_guard guard {*this};

        node.right->accept(*this);
    }
}

void PrintVisitor::visit(relational_expression_node& node) {
    output << indent()
           << "RelationalExpression: " << rel_symbols.at(node.operation)
           << '\n';

    nest_guard guard {*this};

    output << indent() << "Left:\n";

    {
        nest_guard guard {*this};

        node.left->accept(*this);
    }

    output << indent() << "Right:\n";

    {
        nest_guard guard {*this};

        node.right->accept(*this);
    }
}

void PrintVisitor::visit(integer_literal_expression_node& node) {
    output << indent() << "Integer: " << node.value << '\n';
}

/***********************************************************************/
