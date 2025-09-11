#include <iostream>
#include "EdgeOrientation.h"
#include "MapFileReader.h"
#include "BaseVertexProperty.h"
#include "BaseEdgeProperty.h"
#include "BaseUGraphProperty.h"
#include "VertexAlignment.h"

int main() {
    std::cout << "=== Edge Orientation Optimization Test ===" << std::endl;
    
    std::vector<Map::BaseVertexProperty> vertexList;
    std::vector<Map::BaseEdgeProperty> edgeList;
    Map::BaseUGraphProperty graph;
    Map::readMapFileToGraph("local_map.txt", vertexList, edgeList, graph);
    
    int result_2 = Map::optimizeEdgeOrientation(vertexList, edgeList, graph);
    
    if (result_2 == 0) {
        std::cout << "\n=== Test completed successfully! ===" << std::endl;
    } else {
        std::cout << "\n=== Test failed with error code: " << result_2 << " ===" << std::endl;
    }

    int result_3 = Map::optimizeVertexAlignment(vertexList, edgeList, graph);

    if (result_3 == 0) {
        std::cout << "\n=== Test completed successfully! ===" << std::endl;
    } else {
        std::cout << "\n=== Test failed with error code: " << result_3 << " ===" << std::endl;
    }
    
    return 0;
}