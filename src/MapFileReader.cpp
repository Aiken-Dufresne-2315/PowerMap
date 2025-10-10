//------------------------------------------------------------------------------
// MapFileReader.cpp - the example code to read the local_map.txt file
//------------------------------------------------------------------------------

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <regex>
#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "BaseVertexProperty.h"
#include "BaseEdgeProperty.h"
#include "BaseUGraphProperty.h"
#include "Commons.h"

namespace Map {
    
    // Global vertex ID to descriptor mapping
    std::map<int, boost::graph_traits<BaseUGraphProperty>::vertex_descriptor> vertexID2Desc;
    //------------------------------------------------------------------------------
    // calculate the angle between two vertices (in radians)
    //------------------------------------------------------------------------------
    double calculateAngle(const BaseVertexProperty& source, const BaseVertexProperty& target) {
        double dx = target.getCoord().x() - source.getCoord().x();
        double dy = target.getCoord().y() - source.getCoord().y();
        return std::atan2(dy, dx);
    }

    //------------------------------------------------------------------------------
    // auxiliary function: remove the whitespace characters at the beginning and end of the string
    //------------------------------------------------------------------------------
    std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(' ');
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(' ');
        return str.substr(first, (last - first + 1));
    }

    //------------------------------------------------------------------------------
    // Helper function to create vertex ID to index mapping
    //------------------------------------------------------------------------------
    std::map<unsigned int, int> createVertexID2Index(const std::vector<BaseVertexProperty>& vertexList) {
        std::map<unsigned int, int> vertexID2Index;
        for (int i = 0; i < vertexList.size(); ++i) {
            vertexID2Index[vertexList[i].getID()] = i;
        }
        return vertexID2Index;
    }

    //------------------------------------------------------------------------------
    // parse the vertex data: format "id. name (x, y)"
    // for example: 57. 塘朗 (617, 966)
    //------------------------------------------------------------------------------
    bool parseVertex(const std::string& line, BaseVertexProperty& vertex) {
        // use regular expression to parse
        std::regex vertexRegex(R"((\d+)\.\s*([^\(]+)\s*\((\d+),\s*(\d+)\))");
        std::smatch matches;
        
        if (std::regex_search(line, matches, vertexRegex) && matches.size() == 5) {
            unsigned int id = std::stoul(matches[1].str());
            std::string name = trim(matches[2].str());
            double x = std::stod(matches[3].str());
            double y = std::stod(matches[4].str());
            
            // create the vertex object
            vertex = BaseVertexProperty(id, x, y, name);
            return true;
        }
        return false;
    }

    //------------------------------------------------------------------------------
    // parse the edge data: format "source id - target id"
    // for example: 57 - 58
    //------------------------------------------------------------------------------
    bool parseEdge(const std::string& line, 
                std::vector<BaseVertexProperty>& vertices,
                const std::map<unsigned int, int>& vertexID2Index,
                BaseEdgeProperty& edge) {
        std::regex edgeRegex(R"((\d+)\s*-\s*(\d+))");
        std::smatch matches;
        
        if (std::regex_search(line, matches, edgeRegex) && matches.size() == 3) {
            unsigned int sourceID = std::stoul(matches[1].str());
            unsigned int targetID = std::stoul(matches[2].str());
            
            // Find vertex indices
            auto sourceIt = vertexID2Index.find(sourceID);
            auto targetIt = vertexID2Index.find(targetID);
            
            if (sourceIt == vertexID2Index.end() || targetIt == vertexID2Index.end()) {
                std::cerr << "error: vertex not found for edge " << sourceID << " - " << targetID << std::endl;
                return false;
            }
            
            // !!! reference type
            BaseVertexProperty& sourceVertex = vertices[sourceIt->second];
            BaseVertexProperty& targetVertex = vertices[targetIt->second];
            
            // Calculate angle
            double angle = calculateAngle(sourceVertex, targetVertex);
            
            // Calculate orientation feasibility for edge optimization
            double dx = std::abs(targetVertex.getCoord().x() - sourceVertex.getCoord().x());
            double dy = std::abs(targetVertex.getCoord().y() - sourceVertex.getCoord().y());
            
            // Constants for orientation determination (15 degrees threshold)
            // !!! To be set as hyper-parameter !!!
            const double ANGLE_THRESHOLD_DEG = 30;
            const double TAN_THRESHOLD = std::tan(ANGLE_THRESHOLD_DEG * M_PI / 180.0);
            
            bool close2H = (dy <= TAN_THRESHOLD * dx);
            bool close2V = (dx <= TAN_THRESHOLD * dy);
            
            // Create edge with auto-generated ID
            static unsigned int edgeCounter = 0;
            
            edge = BaseEdgeProperty(sourceVertex, targetVertex, edgeCounter++, angle, 1.0, false, 0, close2H, close2V);
            
            return true;
        }
        return false;
    }

    //----------------------------------------------------------------------------
    // !!! the main function to read the file - filling vertices and edges vectors
    //----------------------------------------------------------------------------
    bool readMapFile(const std::string& filename, 
                    std::vector<BaseVertexProperty>& vertices, 
                    std::vector<BaseEdgeProperty>& edges) {
        
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "error: cannot open file " << filename << std::endl;
            return false;
        }
        
        std::string line;
        bool readingVertices = false;
        bool readingEdges = false;
        
        // used to quickly find the vertex mapping
        std::map<unsigned int, int> vertexID2Index;
        
        while (std::getline(file, line)) {
            // remove the whitespace characters at the beginning and end of the line
            line = trim(line);
            
            // skip the empty line
            if (line.empty()) {
                continue;
            }
            
            // check the comment and section marker
            if (line.find("# Vertices") != std::string::npos) {
                readingVertices = true;
                readingEdges = false;
                std::cout << "start to read the vertex data..." << std::endl;
                continue;
            } 
            else if (line.find("# Edges") != std::string::npos) {
                readingVertices = false;
                readingEdges = true;
                std::cout << "start to read the edge data..." << std::endl;
                continue;
            }
            else if (line.find("# End") != std::string::npos) {
                std::cout << "reached end marker, stopping file reading..." << std::endl;
                break;
            }
            else if (line[0] == '#') {
                // skip the other comment line
                continue;
            }
            
            // parse the vertex data
            if (readingVertices) {
                BaseVertexProperty vertex;
                if (parseVertex(line, vertex)) {
                    // !!! ID mapping
                    vertexID2Index[vertex.getID()] = vertices.size();
                    // !!! add the vertex to the vertices vector
                    vertices.push_back(vertex);
                    std::cout << "read the vertex: " << vertex.getID() << ". " 
                            << vertex.getName() << " (" 
                            << vertex.getCoord().x() << ", " 
                            << vertex.getCoord().y() << ")" << std::endl;
                } 
                else {
                    std::cerr << "warning: cannot parse the vertex line: " << line << std::endl;
                }
            }
            // parse the edge data
            else if (readingEdges) {
                BaseEdgeProperty edge;
                if (parseEdge(line, vertices, vertexID2Index, edge)) {
                    edges.push_back(edge);
                    std::cout << "read the edge: " << edge.Source().getID() << " - " << edge.Target().getID()
                            << " (ID: " << edge.ID() << ", angle: " << edge.Angle() << ")" << std::endl;
                } 
                else {
                    std::cerr << "warning: cannot parse the edge line: " << line << std::endl;
                }
            }
        }
        
        file.close();
        
        std::cout << "\nfile read completed!" << std::endl;
        std::cout << "total read " << vertices.size() << " vertices" << std::endl;
        std::cout << "total read " << edges.size() << " edges" << std::endl;
        
        return true;
    }

    //------------------------------------------------------------------------------
    // validate the edge validity (check if the endpoints of the edge exist)
    //------------------------------------------------------------------------------
    bool validateEdges(const std::vector<BaseVertexProperty>& vertices, 
                    const std::vector<BaseEdgeProperty>& edges) {
        // create the vertex ID set
        std::set<unsigned int> vertexIDs;
        for (const auto& vertex : vertices) {
            vertexIDs.insert(vertex.getID());
        }
        
        bool allValid = true;
        for (const auto& edge : edges) {
            unsigned int sourceID = edge.Source().getID();
            unsigned int targetID = edge.Target().getID();
            
            if (vertexIDs.find(sourceID) == vertexIDs.end()) {
                std::cerr << "error: the source vertex " << sourceID << " of edge " << edge.ID() 
                        << " (" << sourceID << " - " << targetID << ") does not exist" << std::endl;
                allValid = false;
            }
            if (vertexIDs.find(targetID) == vertexIDs.end()) {
                std::cerr << "error: the target vertex " << targetID << " of edge " << edge.ID() 
                        << " (" << sourceID << " - " << targetID << ") does not exist" << std::endl;
                allValid = false;
            }
        }
        
        return allValid;
    }

    //------------------------------------------------------------------------------
    // print the statistics information
    //------------------------------------------------------------------------------
    void printStatistics(const std::vector<BaseVertexProperty>& vertices, 
                        const std::vector<BaseEdgeProperty>& edges) {
        
        std::cout << "\n=== map data statistics ===" << std::endl;
        std::cout << "number of vertices: " << vertices.size() << std::endl;
        std::cout << "number of edges: " << edges.size() << std::endl;
        
        // calculate the coordinate range
        if (!vertices.empty()) {
            double minX = vertices[0].getCoord().x();
            double maxX = vertices[0].getCoord().x();
            double minY = vertices[0].getCoord().y();
            double maxY = vertices[0].getCoord().y();
            
            for (const auto& vertex : vertices) {
                minX = std::min(minX, vertex.getCoord().x());
                maxX = std::max(maxX, vertex.getCoord().x());
                minY = std::min(minY, vertex.getCoord().y());
                maxY = std::max(maxY, vertex.getCoord().y());
            }
            
            std::cout << "coordinate range: X[" << minX << ", " << maxX << "], Y[" << minY << ", " << maxY << "]" << std::endl;
        }
    }

    //------------------------------------------------------------------------------
    // !!! build BaseUGraphProperty from vertices and edges vectors
    //------------------------------------------------------------------------------
    bool buildGraph(const std::vector<BaseVertexProperty>& vertices, 
                const std::vector<BaseEdgeProperty>& edges,
                BaseUGraphProperty& graph) {
        
        // Clear the graph first
        clearGraph(graph);
        
        // Create mapping from vertex ID to graph vertex descriptor
        std::map<unsigned int, BaseUGraphProperty::vertex_descriptor> vertexMap;
        
        // Add all vertices to the graph
        std::cout << "\nbuilding graph: adding vertices..." << std::endl;
        for (const auto& vertexProp : vertices) {
            BaseUGraphProperty::vertex_descriptor vd = add_vertex(vertexProp, graph);
            vertexMap[vertexProp.getID()] = vd;
        }
        
        // Add all edges to the graph
        std::cout << "\nbuilding graph: adding edges..." << std::endl;
        for (const auto& edgeProp : edges) {
            unsigned int sourceID = edgeProp.Source().getID();
            unsigned int targetID = edgeProp.Target().getID();
            
            // Find vertex descriptors
            auto sourceIt = vertexMap.find(sourceID);
            auto targetIt = vertexMap.find(targetID);
            
            // checking
            if (sourceIt == vertexMap.end() || targetIt == vertexMap.end()) {
                std::cerr << "error: vertex not found for edge " << sourceID 
                        << " - " << targetID << std::endl;
                continue;
            }
            
            BaseUGraphProperty::vertex_descriptor sourceDec = sourceIt->second;
            BaseUGraphProperty::vertex_descriptor targetDec = targetIt->second;
            
            // Create a new edge property that references vertices in the graph
            BaseEdgeProperty graphEdgeProp(
                graph[sourceDec],
                graph[targetDec], 
                edgeProp.ID(), 
                edgeProp.Angle(),
                edgeProp.Weight(),
                edgeProp.Visited(),
                edgeProp.VisitNum(),
                edgeProp.Close2H(),
                edgeProp.Close2V(),
                edgeProp.Oriented2H(),
                edgeProp.Oriented2V()
            );
            
            // Add edge to graph using the new edge property
            std::pair<BaseUGraphProperty::edge_descriptor, bool> result = 
                add_edge(sourceDec, targetDec, graphEdgeProp, graph);
            
            if (result.second) {
                std::cout << "added edge " << sourceID << " - " << targetID 
                        << " (ID: " << edgeProp.ID() << ", angle: " << edgeProp.Angle() << ")" << std::endl;
            } else {
                std::cerr << "failed to add edge " << sourceID << " - " << targetID << std::endl;
            }
        }
        
        std::cout << "\ngraph construction completed!" << std::endl;
        return true;
    }

    //------------------------------------------------------------------------------
    // Build global vertex ID to descriptor mapping
    //------------------------------------------------------------------------------
    void buildVertexMapping(const BaseUGraphProperty& graph) { 
        vertexID2Desc.clear();
        std::pair<BaseUGraphProperty::vertex_iterator, BaseUGraphProperty::vertex_iterator> vertices = boost::vertices(graph);
        for (auto vit = vertices.first; vit != vertices.second; ++vit) {
            int id = graph[*vit].getID();
            vertexID2Desc[id] = *vit;
            // std::cout << "\n" << id << " " << graph[*vit].getID() << std::endl;
        }
        std::cout << "built vertex mapping with " << vertexID2Desc.size() << " vertices" << std::endl;
    }

    //------------------------------------------------------------------------------
    // read map file and build BaseUGraphProperty directly
    // Hierarchical structure: readMapFile -> validateEdges -> buildGraph
    //------------------------------------------------------------------------------
    bool readMapFileToGraph(const std::string& filename, std::vector<BaseVertexProperty>& vertices, std::vector<BaseEdgeProperty>& edges, BaseUGraphProperty& graph) {
        // First read the file using existing function
        if (!readMapFile(filename, vertices, edges)) {
            return false;
        }
        
        // Validate the data
        if (!validateEdges(vertices, edges)) {
            std::cerr << "error: edge validation failed" << std::endl;
            return false;
        }
        
        // Build the graph
        if (buildGraph(vertices, edges, graph)) {
            // Build the global vertex mapping after successful graph construction
            buildVertexMapping(graph);
            return true;
        }
        return false;
    }
} // namespace Map