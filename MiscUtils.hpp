#ifndef MISCUTILS_HPP
#define MISCUTILS_HPP

/***********************************************************************/

#include <exception>

/***********************************************************************/

/// Stores a location in the source code
struct location {
    /// The (one-based) line number in the source code file
    int line_num {-1};
    /// The (one-based) column number in the source code file
    int col_num {-1};
};

/***********************************************************************/

class cminus_exception : public std::exception {
public:
    cminus_exception(location location) : location {location} {}

protected:
    location location;
};

/***********************************************************************/

#endif
