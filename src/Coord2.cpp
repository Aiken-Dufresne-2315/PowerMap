#include <iomanip>

#include "Coord2.h"

//------------------------------------------------------------------------------
// Protected Functions
//------------------------------------------------------------------------------
void Coord2::_init(void) {
    _element[0] = _element[1] = 0.0;
}

//------------------------------------------------------------------------------
// Public functions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Constuructors
//------------------------------------------------------------------------------
Coord2::Coord2() {
    _init();
}

Coord2::Coord2(const double x, const double y) {
    _element[0] = x;
    _element[1] = y;
}

Coord2::Coord2(const Coord2 & v) {
    _element[0] = v._element[0];
    _element[1] = v._element[1];
}

//------------------------------------------------------------------------------
// Unary Operators
//------------------------------------------------------------------------------
Coord2 & Coord2::operator = (const Coord2 & v) {
    if (this != &v) {
     _element[0] = v._element[0];
     _element[1] = v._element[1];
    }
    return *this;
}

Coord2 & Coord2::operator += (const Coord2 & v) {
    _element[0] += v._element[0];
    _element[1] += v._element[1];
    return *this;
}

Coord2 & Coord2::operator -= (const Coord2 & v) {
    _element[0] -= v._element[0];
    _element[1] -= v._element[1];
    return *this;
}

Coord2 & Coord2::operator *= (const double d) {
    _element[0] *= d;
    _element[1] *= d;
    return *this;
}

Coord2 & Coord2::operator /= (const double d) {
    double d_inv = 1./d;
    _element[0] *= d_inv;
    _element[1] *= d_inv;
    return *this;
}

const double & Coord2::operator [] (int i) const {
    return _element[i];
}

double & Coord2::operator [] (int i) {
    return _element[i];
}

void Coord2::set(const double x, const double y) {
    _element[0] = x;
    _element[1] = y;
}

Coord2 operator - (const Coord2 & a) {
    return Coord2(-a._element[0], -a._element[1]);
}

Coord2 operator + (const Coord2 & a, const Coord2 & b) {
    return Coord2(a._element[0] + b._element[0], a._element[1] + b._element[1]);
}

Coord2 operator - (const Coord2 & a, const Coord2 & b) {
    return Coord2(a._element[0] - b._element[0], a._element[1] - b._element[1]);
}

Coord2 operator * (const double d, const Coord2 & a) {
    return Coord2(d * a._element[0], d * a._element[1]);
}

double operator * (const Coord2 & a, const Coord2 & b) {
    return (a._element[0] * b._element[0] + a._element[1] * b._element[1]);
}

Coord2 operator / (const Coord2 & a, const double d) {
    double d_inv = 1./d;
    return Coord2(a._element[0] * d_inv, a._element[1] * d_inv);
}

int operator == (const Coord2 & a, const Coord2 & b) {
    return ((a._element[0] == b._element[0]) &&
      (a._element[1] == b._element[1]));
}

int operator < (const Coord2 & a, const Coord2 & b) {
    if (a._element[0] < b._element[0]) { return true; }
    else if (a._element[0] > b._element[0]) { return false; }
    else {
     if (a._element[1] < b._element[1]) {return true;}
     else if (a._element[1] > b._element[1]) {return false;}
     else {return false;}
    }
}

int operator > (const Coord2 & a, const Coord2 & b) {
    if (a._element[0] > b._element[0]) return true;
    else if (a._element[0] < b._element[0]) return false;
    else {
        if (a._element[1] > b._element[1]) { return true; }
        else if (a._element[1] < b._element[1]) { return false; }
        else {return false;}
    }
}

std::ostream & operator << (std::ostream & stream, const Coord2 & obj) {
    int i;
    for (i = 0; i < 2; i++) {
 //stream << setw(width) << obj._element[i];
 stream << std::setw(4) << obj._element[i];
 if (i != 1) stream << "\t";
    }
    stream << std::endl;

    return stream;
}

std::istream & operator >> (std::istream & stream, Coord2 & obj) {
    int i; // loop counter
    // reading the elements
    for (i = 0; i < 2; i++)
 stream >> obj._element[i];
    return stream;
}
