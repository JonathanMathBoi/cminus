#include "AST.hpp"

#include <memory>
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

