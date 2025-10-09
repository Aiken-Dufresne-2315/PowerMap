//------------------------------------------------------------------------------
// EdgeOrientation.h - Edge Orientation Optimization Header File
//------------------------------------------------------------------------------

#ifndef _Map_EdgeOrientation_H
#define _Map_EdgeOrientation_H

#include "BaseUGraphProperty.h"

namespace Map {
    
    // Main function for edge orientation optimization
    // Returns 0 on success, -1 on failure
    int optimizeEdgeOrientation(
        std::vector<BaseVertexProperty>& vertexList, 
        std::vector<BaseEdgeProperty>& edgeList, 
        BaseUGraphProperty& graph,
        const std::string& testCaseName = "");

} // namespace Map

#endif // _Map_EdgeOrientation_H