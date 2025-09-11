#ifndef _Map_BaseGraphProperty_H
#define _Map_BaseGraphProperty_H

//------------------------------------------------------------------------------
// Including Header Files
//------------------------------------------------------------------------------

#include <iostream>

#include "Coord2.h"

//------------------------------------------------------------------------------
// Defining Macros
//------------------------------------------------------------------------------

namespace Map {
    //------------------------------------------------------------------------------
    // Defining Classes
    //------------------------------------------------------------------------------
    class BaseGraphProperty {

    private:

    protected:

        //------------------------------------------------------------------------------
        // Special functions
        //------------------------------------------------------------------------------
        void _init( double &width, double &height );

    public:

        Coord2 * centerPtr;
        const double *  widthPtr;
        const double *  heightPtr;

        //------------------------------------------------------------------------------
        // Constructors & Destructors
        //------------------------------------------------------------------------------
        // default constructor
        BaseGraphProperty( void );
        // !!! copy constructor
        BaseGraphProperty( const BaseGraphProperty& c ) {}
        // destructor
        virtual ~BaseGraphProperty( void ) {}

        //------------------------------------------------------------------------------
        // Assignment operators
        //------------------------------------------------------------------------------

        //------------------------------------------------------------------------------
        // Reference to elements
        //------------------------------------------------------------------------------

        //------------------------------------------------------------------------------
        // Special functions
        //------------------------------------------------------------------------------

        void init( double&__width, double&__height ) {
            _init( __width, __height );
        }

        //------------------------------------------------------------------------------
        // Friend functions
        //------------------------------------------------------------------------------

        //------------------------------------------------------------------------------
        // I/O functions
        //------------------------------------------------------------------------------
        // output
        friend std::ostream& operator << ( std::ostream& s, const BaseGraphProperty& v );
        // input
        friend std::istream& operator >> ( std::istream& s, BaseGraphProperty& v );
        // class name
        virtual const char * className( void ) const { return "BaseGraphProperty"; }
    };

} // namespace Map

#endif