#include <iostream>
#include "EdgeOrientation.h"
#include "MapFileReader.h"
#include "BaseVertexProperty.h"
#include "BaseEdgeProperty.h"
#include "BaseUGraphProperty.h"
#include "VertexAlignment.h"
#include "DynamicGrid.h"
#include "DVPositioning.h"
#include "AuxLineSpacing.h"
#include "VisualizeSVG.h"

int main() {
    std::cout << "=== Metro Map Optimization Test ===" << std::endl;
    
    std::vector<Map::BaseVertexProperty> vertexList;
    std::vector<Map::BaseEdgeProperty> edgeList;
    Map::BaseUGraphProperty graph;
    
    // Define input file
    std::string inputFile = "input/test2.txt";
    // std::string inputFile = "local_map.txt";

    // Extract test case name from input file (e.g., "input/test0.txt" -> "test0")
    std::string testCaseName;
    size_t lastSlash = inputFile.find_last_of("/\\");
    size_t lastDot = inputFile.find_last_of(".");
    if (lastSlash != std::string::npos && lastDot != std::string::npos && lastDot > lastSlash) {
        testCaseName = inputFile.substr(lastSlash + 1, lastDot - lastSlash - 1);
    } else if (lastDot != std::string::npos) {
        testCaseName = inputFile.substr(0, lastDot);
    } else {
        testCaseName = inputFile;
    }
    
    std::cout << "Test case: " << testCaseName << std::endl;
    
    std::cout << "\n=== Reading map file ===" << std::endl;
    if (!Map::readMapFileToGraph(inputFile, vertexList, edgeList, graph)) {
        std::cerr << "Failed to read map file!" << std::endl;
        return -1;
    }
    
    std::cout << "Successfully loaded " << vertexList.size() << " vertices and " 
              << edgeList.size() << " edges" << std::endl;
    
    // Create initial visualization before any optimization
    std::cout << "\n=== Creating Initial Visualization ===" << std::endl;
    std::string initialFile = "output/" + testCaseName + "_0.svg";
    Map::createVisualization(vertexList, edgeList, initialFile);
    std::cout << "Created initial visualization: " << initialFile << std::endl;
    
    std::cout << "\n=== Starting Edge Orientation Optimization ===" << std::endl;
    int result_2 = Map::optimizeEdgeOrientation(vertexList, edgeList, graph, testCaseName);
    
    if (result_2 == 0) {
        std::cout << "\n=== Edge Orientation Test completed successfully! ===" << std::endl;
    } else {
        std::cout << "\n=== Edge Orientation Test failed with error code: " << result_2 << " ===" << std::endl;
        return result_2;
    }

    std::cout << "\n=== Starting Vertex Alignment Optimization ===" << std::endl;
    int result_3 = Map::optimizeVertexAlignment(vertexList, edgeList, graph, testCaseName);

    if (result_3 == 0) {
        std::cout << "\n=== Vertex Alignment Test completed successfully! ===" << std::endl;
    } else {
        std::cout << "\n=== Vertex Alignment Test failed with error code: " << result_3 << " ===" << std::endl;
        return result_3;
    }
    
    std::cout << "\n=== Building Dynamic Grid ===" << std::endl;
    Map::DynamicGrid grid(2.315, 2);
    grid.buildAuxLines(graph);
    grid.printAuxLineInfo();

    std::cout << "\n=== Starting Dangling Vertex Positioning ===" << std::endl;
    // Test dangling vertex positioning with consistent parameter list
    int result_4 = Map::positionDanglingVertices(vertexList, edgeList, graph, grid, testCaseName);
    
    if (result_4 >= 0) {
        std::cout << "\n=== Dangling Vertex Positioning Test completed successfully! ===" << std::endl;
        std::cout << "Modified " << result_4 << " vertices." << std::endl;
    } else {
        std::cout << "\n=== Dangling Vertex Positioning Test failed with error code: " << result_4 << " ===" << std::endl;
        return result_4;
    }

    // Rebuild vertex-line mappings after DVPositioning to ensure consistency
    std::cout << "\n=== Rebuilding vertex-line mappings ===" << std::endl;
    grid.rebuildVertexLineMappings(graph);
    grid.printAuxLineInfo();

    std::cout << "\n=== Starting Auxiliary Line Spacing Optimization ===" << std::endl;
    int result_5 = Map::uniformAuxLineSpacing(vertexList, edgeList, graph, grid, 10.0, testCaseName);
    
    if (result_5 == 0) {
        std::cout << "\n=== Auxiliary Line Spacing Test completed successfully! ===" << std::endl;
    } else {
        std::cout << "\n=== Auxiliary Line Spacing Test failed with error code: " << result_5 << " ===" << std::endl;
        return result_5;
    }

    std::cout << "\n=== All Tests Completed Successfully! ===" << std::endl;
    return 0;
}