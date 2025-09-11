#ifndef _Coord2_H
#define _Coord2_H

#include <iostream>
#include <cmath>

class Coord2 {

protected:
    double          _element[2];
    virtual void    _init(void); // 

public:
//------------------------------------------------------------------------------
// Constructions and Destruction
//------------------------------------------------------------------------------
    Coord2();
    Coord2(const double x, const double y);
    Coord2(const Coord2 & v);
    virtual ~Coord2() {}

//------------------------------------------------------------------------------
// Initialization
//------------------------------------------------------------------------------
    void init( void ) { _init(); }

//------------------------------------------------------------------------------
// Unary Operators
//------------------------------------------------------------------------------
    Coord2 & operator =     ( const Coord2 & v );
    Coord2 & operator +=    ( const Coord2 & v );
    Coord2 & operator -=    ( const Coord2 & v );
    Coord2 & operator *=    ( const double d );
    Coord2 & operator /=    ( const double d );

//------------------------------------------------------------------------------
// Access Coordinates
//------------------------------------------------------------------------------
    double & operator [] ( int i );
    const double & operator [] ( int i ) const;

    const double * element( void ) const { return _element; }

    double & x( void ) { return _element[0]; }
    double & y( void ) { return _element[1]; }
    const double & x( void ) const { return _element[0]; }
    const double & y( void ) const { return _element[1]; }

//------------------------------------------------------------------------------
// Set Coordinates
//------------------------------------------------------------------------------    
    void set( const double x, const double y );
    void setX( const double x ) { _element[ 0 ] = x; }
    void setY( const double y ) { _element[ 1 ] = y; }

//------------------------------------------------------------------------------
// Binary Operators
//------------------------------------------------------------------------------
    // negative
    friend Coord2 operator - ( const Coord2 & v ); 
    friend Coord2 operator + ( const Coord2 & a, const Coord2 & b );
    friend Coord2 operator - ( const Coord2 & a, const Coord2 & b );
    friend Coord2 operator * ( const double d, const Coord2 & a );
    // dot product
    friend double operator * ( const Coord2 & a, const Coord2 & b );
    friend Coord2 operator / ( const Coord2 & a, const double d );

    // comparisons
    friend int operator ==  ( const Coord2 & a, const Coord2 & b );
    friend int operator !=  ( const Coord2 & a, const Coord2 & b );
    friend int operator <   ( const Coord2 & a, const Coord2 & b );
    friend int operator >   ( const Coord2 & a, const Coord2 & b );
    friend int operator <=  ( const Coord2 & a, const Coord2 & b );
    friend int operator >=  ( const Coord2 & a, const Coord2 & b );

//------------------------------------------------------------------------------
// I/O functions
//------------------------------------------------------------------------------
    friend std::ostream & operator << ( std::ostream & s, const Coord2 & v );
    friend std::istream & operator >> ( std::istream & s, Coord2 & v );
};

#endif