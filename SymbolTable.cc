#include "SymbolTable.hpp"
#include "AST.hpp"

#include <iomanip>
#include <memory>
#include <sstream>

using std::shared_ptr;

/***********************************************************************/

SymbolTable::SymbolTable() : m_nestLevel {0}, m_table {} {}

void SymbolTable::enterScope() {
    m_nestLevel++;
    m_table.emplace_back();
}

void SymbolTable::exitScope() {
    m_table.pop_back();
    m_nestLevel--;
}

void SymbolTable::insert(shared_ptr<DeclarationNode> node) {
    if (m_table[m_nestLevel].contains(node->identifier)) {
        throw MultipleDeclaredSymbolException {
            m_table[m_nestLevel].at(node->identifier), node};
    }

    m_table[m_nestLevel].emplace(node->identifier, node);
}

/***********************************************************************/

MultipleDeclaredSymbolException::MultipleDeclaredSymbolException(
    shared_ptr<DeclarationNode> firstDecl,
    shared_ptr<DeclarationNode> badDecl)
    : SymbolException {badDecl->loc}
    , m_firstDecl {firstDecl}
    , m_badDecl {badDecl} {
    std::stringstream message_buffer;
    message_buffer << "Error in symbol parsing: "
                   << std::quoted(m_firstDecl->identifier)
                   << " declared mutiple times." << '\n'
                   << "First declared at line " << m_firstDecl->loc.line_num
                   << ", col " << m_firstDecl->loc.col_num << ".\n"
                   << "Declared again at line " << m_badDecl->loc.line_num
                   << ", col " << m_badDecl->loc.col_num << ".";
    m_errorMessage = message_buffer.str();
}

/***********************************************************************/
