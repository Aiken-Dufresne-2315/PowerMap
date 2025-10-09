//------------------------------------------------------------------------------
// AuxLineSpacing.h - Auxiliary Line Spacing Optimization Header File
//------------------------------------------------------------------------------

#ifndef _Map_AuxLineSpacing_H
#define _Map_AuxLineSpacing_H

#include "BaseUGraphProperty.h"
#include "DynamicGrid.h"

namespace Map {
    
    // Main function for auxiliary line spacing optimization
    // Uniformly distributes auxiliary lines to achieve equal spacing
    // Returns 0 on success, -1 on failure
    int uniformAuxLineSpacing(
        std::vector<BaseVertexProperty>& vertexList,
        std::vector<BaseEdgeProperty>& edgeList,
        BaseUGraphProperty& graph,
        DynamicGrid& grid,
        double minSpacing = 10.0,
        const std::string& testCaseName = "");

} // namespace Map

#endif // _Map_AuxLineSpacing_H

