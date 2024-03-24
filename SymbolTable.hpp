#ifndef SYMBOLTABLE_HPP
#define SYMBOLTABLE_HPP

/***********************************************************************/

#include "AST.hpp"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

/***********************************************************************/

using ScopeTable =
    std::unordered_map<std::string, std::shared_ptr<DeclarationNode>>;

/***********************************************************************/

class SymbolTable {
public:
    SymbolTable();

    void enterScope();
    void exitScope();
    bool insert(std::shared_ptr<DeclarationNode> node);
    std::optional<std::shared_ptr<DeclarationNode>> lookup(
        const std::string_view name) const;
    unsigned getNestLevel() const;

private:
    unsigned m_nestLevel;
    std::vector<ScopeTable> m_table;
};

/***********************************************************************/

#endif
