#ifndef SYMBOLTABLE_HPP
#define SYMBOLTABLE_HPP

/***********************************************************************/

#include "../MiscUtils.hpp"
#include "../ast/AST.hpp"

#include <memory>
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

/// Symbol Table for a C- program
class SymbolTable {
public:
    /// Constructs a new symbol table
    ///
    /// Builds a symbol table for a new C- program including initializing the
    /// global scope table and inserting compiler builtin functions.
    SymbolTable();

    /// Enters a new nested program scope
    ///
    /// Enters a new C- program scope, incrementing the nest level, and creating
    /// a new scope table.
    void enterScope();
    /// Exits the current nested program scope
    ///
    /// Exits the current C- program scope, decrementing the nest level, and
    /// destroying the exited scope table.
    void exitScope();
    /// Inserts a declaration in the current scope
    ///
    /// Inserts the declaration node into the current C- program scope.
    ///
    /// \param node the declaration node to insert
    /// \throws MutipleDeclaredSymbolException if another node with the same
    ///                                        symbol has already been inserted
    ///                                        into the current scope
    void insert(std::shared_ptr<DeclarationNode> node);
    /// Looks up a name in the table
    ///
    /// \param name the symbol name to lookup
    /// \ret the symbol declaration if declared, nullptr otherwise
    std::shared_ptr<DeclarationNode> lookup(const std::string_view name) const;
    /// Gets the current nest level
    ///
    /// \ret the current nest level of the symbol table
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
