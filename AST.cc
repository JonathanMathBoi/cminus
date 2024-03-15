#include "AST.hpp"

#include <memory>
#include <string>
#include <vector>

/***********************************************************************/
// Program Root Node

program_node::program_node(
    std::vector<std::unique_ptr<declaration_node>> declarations
)
    : declarations {std::move (declarations)}
{}

void program_node::accept(visitor& visitor) {
    visitor.visit(*this);
}

/***********************************************************************/
// Declaration Nodes

declaration_node::declaration_node(value_type type, std::string identifier)
    : type {type}, identifier {identifier}
{}

void declaration_node::accept(visitor& visitor) {
    visitor.visit(*this);
}

function_declaration_node::function_declaration_node(
    value_type type,
    std::string identifier,
    std::vector<std::unique_ptr<param_node>> params,
    std::unique_ptr<compound_statement_node> body
)
    : declaration_node {type, identifier}
    , parameters {std::move (params)}
    , function_body {std::move (body)}
{}

void function_declaration_node::accept(visitor& visitor) {
    visitor.visit(*this);
}

variable_declaration_node::variable_declaration_node(
    value_type type,
    std::string identifier
)
    : declaration_node {type, identifier}
{}

void variable_declaration_node::accept(visitor& visitor) {
    visitor.visit(*this);
}

array_declaration_node::array_declaration_node(
    value_type type,
    std::string identifier,
    int size
)
    : variable_declaration_node {type, identifier}, size {size}
{}

void array_declaration_node::accept(visitor& visitor) {
    visitor.visit(*this);
}

param_node::param_node(value_type type, std::string identifier)
    : declaration_node {type, identifier}
{}

void param_node::accept(visitor& visitor) {
    visitor.visit(*this);
}

/***********************************************************************/

