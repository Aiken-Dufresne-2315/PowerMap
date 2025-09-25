//------------------------------------------------------------------------------
// MapFileReader.h - the header file of the map file reader
//------------------------------------------------------------------------------

#ifndef _Map_MapFileReader_H
#define _Map_MapFileReader_H

#include <vector>
#include <string>
#include <utility>
#include <map>
#include "BaseVertexProperty.h"
#include "BaseEdgeProperty.h"
#include "BaseUGraphProperty.h"

namespace Map {

    double calculateAngle(const BaseVertexProperty& source, const BaseVertexProperty& target);

    std::string trim(const std::string& str);

    // Helper function to create vertex ID to index mapping
    std::map<unsigned int, int> createVertexID2Index(const std::vector<BaseVertexProperty>& vertexList);

    bool parseVertex(const std::string& line, BaseVertexProperty& vertex);

    bool parseEdge(
        const std::string& line, 
        const std::vector<BaseVertexProperty>& vertices,
        const std::map<unsigned int, int>& vertexIndexMap,
        BaseEdgeProperty& edge);

    // Basic file reading functions
    bool readMapFile(
        const std::string& filename, 
        std::vector<BaseVertexProperty>& vertices, 
        std::vector<BaseEdgeProperty>& edges);

    bool validateEdges(
        const std::vector<BaseVertexProperty>& vertices, 
        const std::vector<BaseEdgeProperty>& edges);

    bool buildGraph(
        const std::vector<BaseVertexProperty>& vertices, 
        const std::vector<BaseEdgeProperty>& edges,
        BaseUGraphProperty& graph);

    void printStatistics(
        const std::vector<BaseVertexProperty>& vertices, 
        const std::vector<BaseEdgeProperty>& edges);

    // Graph building functions
    bool readMapFileToGraph(const std::string& filename, std::vector<BaseVertexProperty>& vertices, std::vector<BaseEdgeProperty>& edges, BaseUGraphProperty& graph);
} // namespace Map

#endif // _Map_MapFileReader_H