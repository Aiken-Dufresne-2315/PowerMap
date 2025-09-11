#include <iostream>
#include <iomanip>
// #include <cctype>
// #include <cmath>
// #include <algorithm>

#include "BaseGraphProperty.h"

namespace Map {
    //------------------------------------------------------------------------------
    // Private Functions
    //------------------------------------------------------------------------------

    //------------------------------------------------------------------------------
    // Protected Functions
    //------------------------------------------------------------------------------
    void BaseGraphProperty::_init( double &width, double &height ) {
        centerPtr   = NULL;
        widthPtr    = &width;
        heightPtr   = &height;
    }

    //------------------------------------------------------------------------------
    // Public functions
    //------------------------------------------------------------------------------

    //------------------------------------------------------------------------------
    // Constructors & Destructors
    //------------------------------------------------------------------------------
    BaseGraphProperty::BaseGraphProperty() {
        // _init();
    }

    //------------------------------------------------------------------------------
    // Assignment operators
    //------------------------------------------------------------------------------

    //------------------------------------------------------------------------------
    // I/O functions
    //------------------------------------------------------------------------------
    std::ostream & operator << ( std::ostream & stream, const BaseGraphProperty & obj ) {
        // set the output formatting
        stream << std::setiosflags( std::ios::showpoint );
        stream << std::setprecision( 8 );
        stream << std::endl;

        return stream;
    }

    std::istream & operator >> ( std::istream & stream, BaseGraphProperty & obj ) {
        return stream;
    }

} // namespace Map