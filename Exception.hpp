#ifndef EXCEPTION_HPP
#define EXCEPTION_HPP

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

#endif
