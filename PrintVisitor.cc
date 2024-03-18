#include "PrintVisitor.hpp"
#include "AST.hpp"

#include <ostream>

/***********************************************************************/

print_visitor::print_visitor(std::ostream& out_stream)
    : output {out_stream}, current_depth {0} {}

void print_visitor::visit(program_node& node) {
    output << "ProgramNode:\n";
    current_depth++;

    for (auto& decl : node.declarations) {
        output << '\n';
        visit(*decl);
    }

    current_depth--;
}

/***********************************************************************/
