#ifndef AST_HPP
#define AST_HPP

/***********************************************************************/

#include <string>
#include <vector>

/***********************************************************************/
// Foward Class Declarations

// Visitor
class visitor;

// Abstract Base Node
struct node;

// Program Root Node
struct program_node;

// Declaration Nodes
struct declaration_node;
struct function_declaration_node;
struct param_node;
struct variable_declaration_node;
struct array_declaration_node;

// Statement Nodes
struct statement_node;
struct compound_statement_node;
struct if_statement_node;
struct while_statement_node;
// Not parsing for statements yet
// struct for_statement_node;
struct return_statement_node;
struct expression_statement_node;

// Expression Nodes
struct expression_node;
struct assignment_expression_node;
struct variable_expression_node;
struct subscript_expression_node;
struct call_expression_node;
struct additive_expression_node;
struct multiplicative_expression_node;
struct relational_expression_node;
// Not parsing increment and decrement yet
// struct unary_expression_node;
struct integer_literal_expression_node;

/***********************************************************************/

#endif

