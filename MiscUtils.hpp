#ifndef MISCUTILS_HPP
#define MISCUTILS_HPP

/***********************************************************************/

#include <exception>

/***********************************************************************/

class cminus_exception : public std::exception {
public:
    cminus_exception(int line_num, int col_num)
        : m_line_num(line_num), m_col_num(col_num) {}

protected:
    int m_line_num;
    int m_col_num;
};

/***********************************************************************/

/// Stores a location in the source code
struct location {
    /// The (one-based) line number in the source code file
    int line_num {-1};
    /// The (one-based) column number in the source code file
    int col_num {-1};
};

/***********************************************************************/

#endif
