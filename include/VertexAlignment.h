//------------------------------------------------------------------------------
// VertexAlignment.h - Vertex Alignment Optimization Header File
//------------------------------------------------------------------------------

#ifndef _Map_VertexAlignment_H
#define _Map_VertexAlignment_H

#include "BaseUGraphProperty.h"

namespace Map {

    enum AlignmentMethod {
        CLUSTERING_BASED = 0,   // Two-phase: clustering + optimization
        MIXED_INTEGER = 1       // MILP approach (reserved for future)
    };
    
    // Main function for vertex alignment optimization
    // Returns 0 on success, -1 on failure
    int optimizeVertexAlignment(
        std::vector<BaseVertexProperty>& vertexList, 
        std::vector<BaseEdgeProperty>& edgeList, 
        BaseUGraphProperty& graph);

} // namespace Map

#endif // _Map_VertexAlignment_H