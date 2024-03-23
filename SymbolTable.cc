#include "SymbolTable.hpp"

SymbolTable::SymbolTable() : m_nestLevel {0}, m_table {} {}

void SymbolTable::enterScope() {
    m_nestLevel++;
    m_table.emplace_back();
}
