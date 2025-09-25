#ifndef _Map_BaseVertexProperty_H
#define _Map_BaseVertexProperty_H

//------------------------------------------------------------------------------
// Including Header Files
//------------------------------------------------------------------------------

#include <iostream>
#include <vector>
#include <array>

#include "Coord2.h"

//------------------------------------------------------------------------------
// Defining Macros
//------------------------------------------------------------------------------

namespace Map {

    //------------------------------------------------------------------------------
    // Defining Classes
    //------------------------------------------------------------------------------
    class BaseVertexProperty {

    private:

    protected:
        unsigned int    id;
        Coord2          coord;
        bool            valid;
        double          weight;

        double          namePixelWidth;
        double          namePixelHeight;
        std::string     name;
        
        // !!! Direction mask: [Up, Down, Left, Right] - true if outgoing edge exists
        std::array<bool, 4> dirMask;

        //------------------------------------------------------------------------------
        // Special functions
        //------------------------------------------------------------------------------
        void _init( void ) { 
            id = 0; 
            dirMask.fill(false); // Initialize all directions as false
        };

    public:
        //------------------------------------------------------------------------------
        // Constructors & Destructors
        //------------------------------------------------------------------------------
        // default constructor
        BaseVertexProperty( void );

        // parameterized constructor
        BaseVertexProperty( unsigned int _id, double x, double y, const std::string& _name )
            : id(_id), coord(x, y), valid(true), weight(1.0), 
              namePixelWidth(0.0), namePixelHeight(0.0), name(_name), 
              dirMask({false, false, false, false}) {}

        // copy constructor
        // !!! copy constructor of Coord2 called here 
        BaseVertexProperty( 
            const BaseVertexProperty& c 
        ): id(c.id), coord(c.coord), valid(c.valid), weight(c.weight), 
        namePixelWidth(c.namePixelWidth), namePixelHeight(c.namePixelHeight), 
        name(c.name), dirMask(c.dirMask) {}

        // destructor
        virtual ~BaseVertexProperty( void ) {}

        //------------------------------------------------------------------------------
        // Assignment operators
        //------------------------------------------------------------------------------

        //------------------------------------------------------------------------------
        // Reference to elements
        //------------------------------------------------------------------------------
        // Getters
        unsigned int        getID()                 const { return id; }
        const Coord2&       getCoord()              const { return coord; }
        bool                isValid()               const { return valid; }
        double              getWeight()             const { return weight; }
        const std::string&  getName()               const { return name; }
        double              getNamePixelWidth()     const { return namePixelWidth; }
        double              getNamePixelHeight()    const { return namePixelHeight; }
        
        // Direction mask getters (Up=0, Down=1, Left=2, Right=3)
        const std::array<bool, 4>& getDirMask() const { return dirMask; }
        bool UpOccupied()    const { return dirMask[0]; }
        bool DownOccupied()  const { return dirMask[1]; }
        bool LeftOccupied()  const { return dirMask[2]; }
        bool RightOccupied() const { return dirMask[3]; }
        
        // Setters
        void setId(unsigned int _id)            { id = _id; }
        void setCoord(const Coord2& _coord)     { coord = _coord; }
        void setCoord(double x, double y)       { coord.set(x, y); }
        void setValid(bool _valid)              { valid = _valid; }
        void setWeight(double _weight)          { weight = _weight; }
        void setName(const std::string& _name)  { name = _name; }
        void setNamePixelWidth(double _width)   { namePixelWidth = _width; }
        void setNamePixelHeight(double _height) { namePixelHeight = _height; }
        
        // Direction mask setters (Up=0, Down=1, Left=2, Right=3)
        void setDirMask(const std::array<bool, 4>& _mask) { dirMask = _mask; }
        void setUp(bool up)         { dirMask[0] = up; }
        void setDown(bool down)     { dirMask[1] = down; }
        void setLeft(bool left)     { dirMask[2] = left; }
        void setRight(bool right)   { dirMask[3] = right; }
        
        // Helper method to reset all directions
        void resetDirMask() { dirMask.fill(false); }

        //------------------------------------------------------------------------------
        // Special functions
        //------------------------------------------------------------------------------
        void init( void ) { _init(); }

        //------------------------------------------------------------------------------
        // Friend functions
        //------------------------------------------------------------------------------

        //------------------------------------------------------------------------------
        // I/O functions
        //------------------------------------------------------------------------------
        friend std::ostream& operator << ( std::ostream& s, const BaseVertexProperty& v );
        friend std::istream& operator >> ( std::istream& s, BaseVertexProperty& v );
        virtual const char * className( void ) const { return "BaseVertexProperty"; }
    };

} // namespace Map

#endif