//------------------------------------------------------------------------------
// Commons.h - Common definitions and global utilities
//------------------------------------------------------------------------------

#ifndef _Map_Commons_H
#define _Map_Commons_H

#include <map>
#include "BaseUGraphProperty.h"

namespace Map {
    
    // Global mapping from vertex ID to vertex descriptor
    // This map is built once and used throughout the application
    extern std::map<int, boost::graph_traits<BaseUGraphProperty>::vertex_descriptor> vertexID2Desc;
    
    // Helper function to get vertex descriptor by ID
    boost::graph_traits<BaseUGraphProperty>::vertex_descriptor getVertexDescriptor(int vertexID);
    
} // namespace Map

#endif // _Map_Commons_H