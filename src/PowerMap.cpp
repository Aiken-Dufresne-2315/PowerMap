//------------------------------------------------------------------------------
// PowerMap.cpp - PowerMap Implementation
//------------------------------------------------------------------------------

#include "PowerMap.h"
#include <boost/graph/graph_traits.hpp>

namespace Map {

    PowerMap::PowerMap() : gridAManager(20.0, 2.0) {
        // Default constructor with default grid parameters
    }

    PowerMap::PowerMap(double tolerance, double minVotes) 
        : gridAManager(tolerance, minVotes) {
        // Constructor with custom grid parameters
    }

    PowerMap::~PowerMap() {
        // Destructor
        clearAllData();
    }

    void PowerMap::buildAuxLinesFromCurrentGraph() {
        gridAManager.buildAuxLinesFromGraph(graphAProp);
    }

    void PowerMap::electKeyAuxLines() {
        gridAManager.electKeyAuxLines();
    }

    int PowerMap::getTotalVertexCount() const {
        return static_cast<int>(boost::num_vertices(graphAProp));
    }

    int PowerMap::getTotalEdgeCount() const {
        return static_cast<int>(boost::num_edges(graphAProp));
    }

    int PowerMap::getKeyAuxLineCount() const {
        return gridAManager.getKeyAuxLineCount();
    }

    void PowerMap::setGridParameters(double tolerance, double minVotes) {
        gridAManager.setToleranceAlignDist(tolerance);
        gridAManager.setMinVoteThreshold(minVotes);
    }

    void PowerMap::printGraphInfo() const {
        std::cout << "=== PowerMap Graph Information ===" << std::endl;
        std::cout << "Total vertices: " << getTotalVertexCount() << std::endl;
        std::cout << "Total edges: " << getTotalEdgeCount() << std::endl;
        std::cout << "Key auxiliary lines: " << getKeyAuxLineCount() << std::endl;
    }

    void PowerMap::printGridInfo() const {
        gridAManager.printAuxLineInfo();
    }

    void PowerMap::clearAllData() {
        // Clear graph data
        graphAProp.clear();
        
        // Clear grid data
        gridAManager.clearAllAuxLines();
    }

} // namespace Map