#ifndef SYMBOLTABLE_HPP
#define SYMBOLTABLE_HPP

/***********************************************************************/

#include "AST.hpp"
#include "MiscUtils.hpp"

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

class SymbolException : public CMinusException {
public:
    SymbolException(Location loc) : CMinusException {loc} {}
};

class MultipleDeclaredSymbolException;

/***********************************************************************/

class SymbolTable {
public:
    SymbolTable();

    void enterScope();
    void exitScope();
    void insert(std::shared_ptr<DeclarationNode> node);
    std::optional<std::shared_ptr<DeclarationNode>> lookup(
        const std::string_view name) const;
    unsigned getNestLevel() const;

private:
    unsigned m_nestLevel;
    std::vector<ScopeTable> m_table;
};

/***********************************************************************/

class MultipleDeclaredSymbolException : public SymbolException {
public:
    MultipleDeclaredSymbolException(
        std::shared_ptr<DeclarationNode> firstDeclaration,
        std::shared_ptr<DeclarationNode> badDeclaration);

    virtual char const* what() const noexcept;

private:
    std::string m_errorMessage;
    std::shared_ptr<DeclarationNode> m_firstDecl;
    std::shared_ptr<DeclarationNode> m_badDecl;
};

/***********************************************************************/

#endif
