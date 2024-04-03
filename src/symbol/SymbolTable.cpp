#include "SymbolTable.hpp"
#include "../ast/AST.hpp"

#include <iomanip>
#include <memory>
#include <ranges>
#include <sstream>
#include <string_view>

using std::shared_ptr;

/***********************************************************************/

SymbolTable::SymbolTable() : m_nestLevel {0}, m_table {} {
    // Add the global scope to the table at construction
    m_table.emplace_back();

    // Insert all builtins to global scope
    for (auto& builtin : g_builtins) {
        m_table[m_nestLevel].emplace(builtin->identifier, builtin);
    }
}

void SymbolTable::enterScope() {
    m_nestLevel++;
    m_table.emplace_back();
}

void SymbolTable::exitScope() {
    m_table.pop_back();
    m_nestLevel--;
}

unsigned SymbolTable::getNestLevel() const {
    return m_nestLevel;
}

void SymbolTable::insert(shared_ptr<DeclarationNode> node) {
    if (m_table[m_nestLevel].contains(node->identifier)) {
        throw MultipleDeclaredSymbolException {
            m_table[m_nestLevel].at(node->identifier), node};
    }

    m_table[m_nestLevel].emplace(node->identifier, node);
}

shared_ptr<DeclarationNode> SymbolTable::lookup(
    const std::string_view name) const {
    // Needed so it can be used to index ScopeTable
    std::string name_cp {name};
    for (ScopeTable const& scope : m_table | std::views::reverse) {
        if (scope.contains(name_cp)) {
            return scope.at(name_cp);
        }
    }

    return nullptr;
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

char const* MultipleDeclaredSymbolException::what() const noexcept {
    return m_errorMessage.c_str();
}

/***********************************************************************/
