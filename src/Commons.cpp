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
    
    boost::graph_traits<BaseUGraphProperty>::edge_descriptor getEdgeDescriptor(int edgeID) {
        auto it = edgeID2Desc.find(edgeID);
        if (it != edgeID2Desc.end()) {
            return it->second;
        }
        throw std::runtime_error("Edge ID not found in global mapping");
    }
} // namespace Map