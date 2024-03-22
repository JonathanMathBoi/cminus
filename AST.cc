#include "AST.hpp"
#include "MiscUtils.hpp"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using std::shared_ptr;
using std::unique_ptr;
using std::vector;

/***********************************************************************/
// Program Root Node

// Uses fixed args for node constructor as the program node is always the entire
// source file
ProgramNode::ProgramNode(vector<shared_ptr<declaration_node>> declarations)
    : Node {location {1, 1}}, declarations {declarations} {}

void ProgramNode::accept(Visitor& visitor) {
    visitor.visit(*this);
}

/***********************************************************************/
// Declaration Nodes

declaration_node::declaration_node(
    value_type type,
    std::string identifier,
    location loc)
    : Node {loc}, type {type}, identifier {identifier} {}

void declaration_node::accept(Visitor& visitor) {
    visitor.visit(*this);
}

function_declaration_node::function_declaration_node(
    value_type type,
    std::string identifier,
    vector<shared_ptr<param_node>> params,
    unique_ptr<compound_statement_node> body,
    location loc)
    : declaration_node {type, identifier, loc}
    , parameters {params}
    , function_body {std::move(body)} {}

void function_declaration_node::accept(Visitor& visitor) {
    visitor.visit(*this);
}

variable_declaration_node::variable_declaration_node(
    value_type type,
    std::string identifier,
    location loc)
    : declaration_node {type, identifier, loc} {}

void variable_declaration_node::accept(Visitor& visitor) {
    visitor.visit(*this);
}

array_declaration_node::array_declaration_node(
    value_type type,
    std::string identifier,
    int size,
    location loc)
    : variable_declaration_node {type, identifier, loc}, size {size} {}

void array_declaration_node::accept(Visitor& visitor) {
    visitor.visit(*this);
}

param_node::param_node(value_type type, std::string identifier, location loc)
    : declaration_node {type, identifier, loc} {}

void param_node::accept(Visitor& visitor) {
    visitor.visit(*this);
}

/***********************************************************************/
// Expression Nodes

assignment_expression_node::assignment_expression_node(
    unique_ptr<variable_expression_node> var,
    unique_ptr<expression_node> expr,
    location loc)
    : expression_node {loc}
    , variable {std::move(var)}
    , expression {std::move(expr)} {}

void assignment_expression_node::accept(Visitor& visitor) {
    visitor.visit(*this);
}

variable_expression_node::variable_expression_node(
    std::string identifier,
    location loc)
    : expression_node {loc}, identifier {identifier} {}

void variable_expression_node::accept(Visitor& visitor) {
    visitor.visit(*this);
}

subscript_expression_node::subscript_expression_node(
    std::string identifier,
    unique_ptr<expression_node> index,
    location loc)
    : variable_expression_node {identifier, loc}, index {std::move(index)} {}

void subscript_expression_node::accept(Visitor& visitor) {
    visitor.visit(*this);
}

call_expression_node::call_expression_node(
    std::string identifier,
    vector<unique_ptr<expression_node>> args,
    location loc)
    : expression_node {loc}
    , identifier {identifier}
    , arguments {std::move(args)} {}

void call_expression_node::accept(Visitor& visitor) {
    visitor.visit(*this);
}

additive_expression_node::additive_expression_node(
    add_op operation,
    unique_ptr<expression_node> lhs,
    unique_ptr<expression_node> rhs,
    location loc)
    : expression_node {loc}
    , operation {operation}
    , left {std::move(lhs)}
    , right {std::move(rhs)} {}

void additive_expression_node::accept(Visitor& visitor) {
    visitor.visit(*this);
}

multiplicative_expression_node::multiplicative_expression_node(
    mul_op operation,
    unique_ptr<expression_node> lhs,
    unique_ptr<expression_node> rhs,
    location)
    : expression_node {loc}
    , operation {operation}
    , left {std::move(lhs)}
    , right {std::move(rhs)} {}

void multiplicative_expression_node::accept(Visitor& visitor) {
    visitor.visit(*this);
}

relational_expression_node::relational_expression_node(
    rel_op operation,
    unique_ptr<expression_node> lhs,
    unique_ptr<expression_node> rhs,
    location loc)
    : expression_node {loc}
    , operation {operation}
    , left {std::move(lhs)}
    , right {std::move(rhs)} {}

void relational_expression_node::accept(Visitor& visitor) {
    visitor.visit(*this);
}

integer_literal_expression_node::integer_literal_expression_node(
    int value,
    location loc)
    : expression_node {loc}, value {value} {}

void integer_literal_expression_node::accept(Visitor& visitor) {
    visitor.visit(*this);
}

/***********************************************************************/
// Statement Nodes

compound_statement_node::compound_statement_node(
    vector<shared_ptr<variable_declaration_node>> decls,
    vector<unique_ptr<statement_node>> stmts,
    location loc)
    : statement_node {loc}
    , local_decls {decls}
    , statements {std::move(stmts)} {}

void compound_statement_node::accept(Visitor& visitor) {
    visitor.visit(*this);
}

if_statement_node::if_statement_node(
    unique_ptr<expression_node> condition,
    unique_ptr<statement_node> then_stmt,
    std::optional<unique_ptr<statement_node>> else_stmt,
    location loc)
    : statement_node {loc}
    , condition {std::move(condition)}
    , then_stmt {std::move(then_stmt)}
    , else_stmt {std::move(else_stmt)} {}

if_statement_node::if_statement_node(
    unique_ptr<expression_node> condition,
    unique_ptr<statement_node> then_stmt,
    location loc)
    : statement_node {loc}
    , condition {std::move(condition)}
    , then_stmt {std::move(then_stmt)}
    , else_stmt {std::nullopt} {}

void if_statement_node::accept(Visitor& visitor) {
    visitor.visit(*this);
}

while_statement_node::while_statement_node(
    unique_ptr<expression_node> condition,
    unique_ptr<statement_node> stmt,
    location loc)
    : statement_node {loc}
    , condition {std::move(condition)}
    , body {std::move(stmt)} {}

void while_statement_node::accept(Visitor& visitor) {
    visitor.visit(*this);
}

return_statement_node::return_statement_node(
    std::optional<unique_ptr<expression_node>> expr,
    location loc)
    : statement_node {loc}, expression {std::move(expr)} {}

return_statement_node::return_statement_node(location loc)
    : statement_node {loc}, expression {std::nullopt} {}

void return_statement_node::accept(Visitor& visitor) {
    visitor.visit(*this);
}

expression_statement_node::expression_statement_node(
    std::optional<unique_ptr<expression_node>> expr,
    location loc)
    : statement_node {loc}, expr {std::move(expr)} {}

expression_statement_node::expression_statement_node(location loc)
    : statement_node {loc}, expr {std::nullopt} {}

void expression_statement_node::accept(Visitor& visitor) {
    visitor.visit(*this);
}
/***********************************************************************/
