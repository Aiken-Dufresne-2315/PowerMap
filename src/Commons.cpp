//------------------------------------------------------------------------------
// Commons.cpp - Common utilities implementation
//------------------------------------------------------------------------------

#include "Commons.h"
#include <stdexcept>

namespace Map {
    
    boost::graph_traits<BaseUGraphProperty>::vertex_descriptor getVertexDescriptor(int vertexID) {
        auto it = vertexID2Desc.find(vertexID);
        if (it != vertexID2Desc.end()) {
            return it->second;
        }
        throw std::runtime_error("Vertex ID not found in global mapping");
    }
    
} // namespace Map