#ifndef _Map_DVPositioning_H
#define _Map_DVPositioning_H

//------------------------------------------------------------------------------
// Including Header Files
//------------------------------------------------------------------------------

#include <vector>
#include "BaseUGraphProperty.h"
#include "DynamicGrid.h"
#include "Coord2.h"

namespace Map {

    std::vector<int> findDVs(const DynamicGrid& grid, const BaseUGraphProperty& graph);
    

    bool isOnHAL(int vertexID, const DynamicGrid& grid, const BaseUGraphProperty& graph);
    bool isOnVAL(int vertexID, const DynamicGrid& grid, const BaseUGraphProperty& graph);
    
    std::vector<AuxiliaryLine> getAdjHALs(int vertexID, const DynamicGrid& grid, const BaseUGraphProperty& graph);
    
    std::vector<AuxiliaryLine> getAdjVALs(int vertexID, const DynamicGrid& grid, const BaseUGraphProperty& graph);
    
    // Note: overlapHappens is declared in CheckOverlap.h
    
    void addHAL(double position, DynamicGrid& grid, const BaseUGraphProperty& graph);
    void addVAL(double position, DynamicGrid& grid, const BaseUGraphProperty& graph);

    void processPDV(int vertexID, DynamicGrid& grid, BaseUGraphProperty& graph);
    void processFDV(int vertexID, DynamicGrid& grid, BaseUGraphProperty& graph);
    
    // Main function for dangling vertex positioning
    // Returns the number of modified vertices on success, -1 on failure
    int positionDanglingVertices(
        std::vector<BaseVertexProperty>& vertexList, 
        std::vector<BaseEdgeProperty>& edgeList, 
        BaseUGraphProperty& graph,
        DynamicGrid& grid);

} // namespace Map

#endif