#ifndef _Map_BaseEdgeProperty_H
#define _Map_BaseEdgeProperty_H

//------------------------------------------------------------------------------
// Including Header Files
//------------------------------------------------------------------------------

#include <iostream>
#include <vector>
#include <functional>  // for std::reference_wrapper

#include "BaseVertexProperty.h"

//------------------------------------------------------------------------------
// Defining Macros
//------------------------------------------------------------------------------

namespace Map {

    //------------------------------------------------------------------------------
    // Defining Classes
    //------------------------------------------------------------------------------
    class  BaseEdgeProperty {

    private:
        // static null vertex for default constructor
        static BaseVertexProperty& getNullVertex();

    protected:
        std::reference_wrapper<BaseVertexProperty>  source;
        std::reference_wrapper<BaseVertexProperty>  target;
        unsigned int                                id;
        double                                      angle;
        double                                      weight;
        bool                                        visited;
        unsigned int                                visitedTimes;
        bool                                        oriented2H;
        bool                                        oriented2V;

        //------------------------------------------------------------------------------
        // Special functions
        //------------------------------------------------------------------------------
        void _init( void );

    public:
        //------------------------------------------------------------------------------
        // Constructors & Destructors
        //------------------------------------------------------------------------------
        // default constructor - initialize with null vertices  
        BaseEdgeProperty( void );

        // para_constructor
        BaseEdgeProperty(
            BaseVertexProperty& s, 
            BaseVertexProperty& t,
            unsigned int i, 
            double a,
            double w = 1,
            bool v = false,
            unsigned int vT = 0,
            bool oriented2H = false,
            bool oriented2V = false
        ): source(s), target(t), id(i), angle(a), weight(w), visited(v), visitedTimes(vT), 
           oriented2H(oriented2H), oriented2V(oriented2V) {}

        // copy constructor
        BaseEdgeProperty( 
            const BaseEdgeProperty& e 
        ): source(e.source), target(e.target), 
        id(e.id), angle(e.angle), weight(e.weight), 
        visited(e.visited), visitedTimes(e.visitedTimes),
        oriented2H(e.oriented2H), oriented2V(e.oriented2V) {}

        // destructor
        virtual ~BaseEdgeProperty( void ) {}

        //------------------------------------------------------------------------------
        // Reference to elements
        //------------------------------------------------------------------------------
        BaseVertexProperty& Source()            const { return this->source; }
        BaseVertexProperty& Target()            const { return this->target; }
        unsigned int        ID()                const { return this->id; }
        double              Angle()             const { return this->angle; }
        double              Weight()            const { return this->weight; }
        bool                Visited()           const { return this->visited; }
        unsigned int        VisitNum()          const { return this->visitedTimes; }
        bool                Oriented2H()        const { return this->oriented2H; }
        bool                Oriented2V()        const { return this->oriented2V; }

        //------------------------------------------------------------------------------
        // Assignment operators
        //------------------------------------------------------------------------------

        BaseEdgeProperty& operator=(const BaseEdgeProperty& other) {
            if (this != &other) {
                this->source = other.source;
                this->target = other.target; 
                this->id = other.id;
                this->angle = other.angle;
                this->weight = other.weight;
                this->visited = other.visited;
                this->visitedTimes = other.visitedTimes;
                this->oriented2H = other.oriented2H;
                this->oriented2V = other.oriented2V;
            }
            return *this;
        }

        //------------------------------------------------------------------------------
        // Setters
        //------------------------------------------------------------------------------

        void setID(unsigned int _id)                    { this->id = _id; }
        void setAngle(double _angle)                    { this->angle = _angle; }
        void setWeight(double _weight)                  { this->weight = _weight; }
        void setVisited(bool _visited)                  { this->visited = _visited; }
        void setVisitNum(unsigned int _visitedTimes)    { this->visitedTimes = _visitedTimes; }
        void setOriented2H(bool _oriented2H)            { this->oriented2H = _oriented2H; }
        void setOriented2V(bool _oriented2V)            { this->oriented2V = _oriented2V; }
        
        // rebind vertices
        void setSource(BaseVertexProperty& newSource) { this->source = newSource; }
        void setTarget(BaseVertexProperty& newTarget) { this->target = newTarget; }
        
        // check if the edge is valid
        // !!! I don't know why.
        bool isValid() const { 
            return &source.get() != &getNullVertex() && &target.get() != &getNullVertex(); 
        }

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
        // output
        friend std::ostream& operator << ( std::ostream& s, const BaseEdgeProperty& v );
        // input
        friend std::istream& operator >> ( std::istream& s, BaseEdgeProperty& v );
        // class name
        virtual const char * className( void ) const { return "BaseEdgeProperty"; }
    };

} // namespace Map

#endif