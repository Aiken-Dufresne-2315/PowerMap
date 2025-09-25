#ifndef _Map_PowerMap_H
#define _Map_PowerMap_H

#include <iostream>
#include "BaseUGraphProperty.h"
#include "DynamicGrid.h"

namespace Map {

    class PowerMap {
    private:
        BaseUGraphProperty  graphAProp;     // Main graph structure containing vertices and edges
        DynamicGrid         gridAManager;   // Dynamic grid for auxiliary lines management

    public:
        // Constructor
        PowerMap();
        PowerMap(double tolerance, double minVotes = 2.0);
        
        // Destructor
        ~PowerMap();
        
        // Graph operations
        BaseUGraphProperty& getGraphAProp() { return graphAProp; }
        const BaseUGraphProperty& getGraphAProp() const { return graphAProp; }
        
        // Grid operations
        DynamicGrid& getGridAManager() { return gridAManager; }
        const DynamicGrid& getGridAManager() const { return gridAManager; }
        
        // Main processing pipeline
        void buildAuxLinesFromCurrentGraph();
        void electKeyAuxLines();
        
        // Information queries
        int getTotalVertexCount() const;
        int getTotalEdgeCount() const;
        int getKeyAuxLineCount() const;
        
        // Configuration
        void setGridParameters(double tolerance, double minVotes);
        
        // Debug and visualization
        void printGraphInfo() const;
        void printGridInfo() const;
        void clearAllData();
    };

} // namespace Map

#endif // _Map_PowerMap_H