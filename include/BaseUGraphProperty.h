#ifndef _Map_BaseUGraphProperty_H
#define _Map_BaseUGraphProperty_H

//------------------------------------------------------------------------------
// Including Header Files
//------------------------------------------------------------------------------

#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <math.h>
#include <algorithm>
#include <ctime>
#include <cstdlib>

#include <boost/config.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/iteration_macros.hpp>

#include "Coord2.h"
#include "BaseGraphProperty.h"
#include "BaseVertexProperty.h"
#include "BaseEdgeProperty.h"

//------------------------------------------------------------------------------
// Defining Macros
//------------------------------------------------------------------------------

namespace Map {
    // !!! definition
    typedef boost::adjacency_list< 
        boost::listS,                               // !!! container of edges
        boost::listS,                               // !!! container of vertexes
        boost::undirectedS,                         // graph directedness tag
        BaseVertexProperty,                         
        BaseEdgeProperty,                               
        BaseGraphProperty >  BaseUGraphProperty;    

    //------------------------------------------------------------------------------
    // Special functions
    //------------------------------------------------------------------------------
    void printGraph( const BaseUGraphProperty & g );
    void clearGraph( BaseUGraphProperty & g );

} // namespace Map

#endif