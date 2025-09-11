//------------------------------------------------------------------------------
// Including Header Files
//------------------------------------------------------------------------------

#include <iostream>
#include <iomanip>
// #include <cctype>
// #include <cmath>
// #include <algorithm>

#include "BaseEdgeProperty.h"

namespace Map {
    //------------------------------------------------------------------------------
    // Private Functions
    //------------------------------------------------------------------------------
    
    // static null vertex implementation
    BaseVertexProperty& BaseEdgeProperty::getNullVertex() {
        static BaseVertexProperty nullVertex;  // static local variable, only created once
        return nullVertex;
    }

    //------------------------------------------------------------------------------
    // Protected Functions
    //------------------------------------------------------------------------------
    void BaseEdgeProperty::_init( void ) {
        id = 0;
        angle = 0;
        weight = 1.0;
        visited = false;
        visitedTimes = 0;
        close2H = false;
        close2V = false;
    }

    //------------------------------------------------------------------------------
    // Public functions
    //------------------------------------------------------------------------------

    //------------------------------------------------------------------------------
    // Constructors & Destructors
    //------------------------------------------------------------------------------
    BaseEdgeProperty::BaseEdgeProperty() 
        : source(getNullVertex()), target(getNullVertex()) {
        _init();
    }

    //------------------------------------------------------------------------------
    // Assignment operators
    //------------------------------------------------------------------------------

    //------------------------------------------------------------------------------
    // I/O functions
    //------------------------------------------------------------------------------
    std::ostream & operator << ( std::ostream & stream, const BaseEdgeProperty & obj ) {
        // set the output formatting
        stream << std::setiosflags( std::ios::showpoint );
        stream << std::setprecision( 8 );
        stream << std::endl;
        return stream;
    }

    std::istream & operator >> ( std::istream & stream, BaseEdgeProperty & obj ) {
        return stream;
    }

} // namespace Map