#ifndef MISCUTILS_HPP
#define MISCUTILS_HPP

/***********************************************************************/

#include <exception>

/***********************************************************************/

/// Stores a location in the source code
struct Location {
    /// The (one-based) line number in the source code file
    int line_num {-1};
    /// The (one-based) column number in the source code file
    int col_num {-1};
};

/***********************************************************************/

class CMinusException : public std::exception {
public:
    CMinusException(Location location) : location {location} {}

protected:
    Location location;
};

/***********************************************************************/

#endif
