#include "AST.hpp"

#include <memory>
#include <string>
#include <vector>

/***********************************************************************/
// Program Root Node

// Uses fixed args for node constructor as the program node is always the entire
// source file
program_node::program_node(
    std::vector<std::shared_ptr<declaration_node>> declarations)
    : node {1, 1}, declarations {declarations} {}

void program_node::accept(visitor& visitor) {
    visitor.visit(*this);
}

/***********************************************************************/
// Declaration Nodes

declaration_node::declaration_node(
    value_type type,
    std::string identifier,
    int line_num,
    int col_num)
    : node {line_num, col_num}, type {type}, identifier {identifier} {}

void declaration_node::accept(visitor& visitor) {
    visitor.visit(*this);
}

function_declaration_node::function_declaration_node(
    value_type type,
    std::string identifier,
    std::vector<std::shared_ptr<param_node>> params,
    std::unique_ptr<compound_statement_node> body,
    int line_num,
    int col_num)
    : declaration_node {type, identifier, line_num, col_num}
    , parameters {params}
    , function_body {std::move(body)} {}

void function_declaration_node::accept(visitor& visitor) {
    visitor.visit(*this);
}

variable_declaration_node::variable_declaration_node(
    value_type type,
    std::string identifier,
    int line_num,
    int col_num)
    : declaration_node {type, identifier, line_num, col_num} {}

void variable_declaration_node::accept(visitor& visitor) {
    visitor.visit(*this);
}

array_declaration_node::array_declaration_node(
    value_type type,
    std::string identifier,
    int size)
    : variable_declaration_node {type, identifier}, size {size} {}

void array_declaration_node::accept(visitor& visitor) {
    visitor.visit(*this);
}

param_node::param_node(value_type type, std::string identifier)
    : declaration_node {type, identifier} {}

void param_node::accept(visitor& visitor) {
    visitor.visit(*this);
}

/***********************************************************************/
// Expression Nodes

assignment_expression_node::assignment_expression_node(
    std::unique_ptr<variable_expression_node> var,
    std::unique_ptr<expression_node> expr)
    : variable {std::move(var)}, expression {std::move(expr)} {}

void assignment_expression_node::accept(visitor& visitor) {
    visitor.visit(*this);
}

variable_expression_node::variable_expression_node(std::string identifier)
    : identifier {identifier} {}

void variable_expression_node::accept(visitor& visitor) {
    visitor.visit(*this);
}

subscript_expression_node::subscript_expression_node(
    std::string identifier,
    std::unique_ptr<expression_node> index)
    : variable_expression_node {identifier}, index {std::move(index)} {}

void subscript_expression_node::accept(visitor& visitor) {
    visitor.visit(*this);
}

call_expression_node::call_expression_node(
    std::string identifier,
    std::vector<std::unique_ptr<expression_node>> args)
    : identifier {identifier}, arguments {std::move(args)} {}

void call_expression_node::accept(visitor& visitor) {
    visitor.visit(*this);
}

additive_expression_node::additive_expression_node(
    add_op operation,
    std::unique_ptr<expression_node> lhs,
    std::unique_ptr<expression_node> rhs)
    : operation {operation}, left {std::move(lhs)}, right {std::move(rhs)} {}

void additive_expression_node::accept(visitor& visitor) {
    visitor.visit(*this);
}

multiplicative_expression_node::multiplicative_expression_node(
    mul_op operation,
    std::unique_ptr<expression_node> lhs,
    std::unique_ptr<expression_node> rhs)
    : operation {operation}, left {std::move(lhs)}, right {std::move(rhs)} {}

void multiplicative_expression_node::accept(visitor& visitor) {
    visitor.visit(*this);
}

relational_expression_node::relational_expression_node(
    rel_op operation,
    std::unique_ptr<expression_node> lhs,
    std::unique_ptr<expression_node> rhs)
    : operation {operation}, left {std::move(lhs)}, right {std::move(rhs)} {}

void relational_expression_node::accept(visitor& visitor) {
    visitor.visit(*this);
}

integer_literal_expression_node::integer_literal_expression_node(int value)
    : value {value} {}

void integer_literal_expression_node::accept(visitor& visitor) {
    visitor.visit(*this);
}

/***********************************************************************/
