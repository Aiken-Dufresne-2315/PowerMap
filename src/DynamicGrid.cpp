//------------------------------------------------------------------------------
// DynamicGrid.cpp - Dynamic Grid Implementation
//------------------------------------------------------------------------------

#include "DynamicGrid.h"
#include <iostream>
#include <numeric>

namespace Map {

    DynamicGrid::DynamicGrid(double tolerance, double minVotes) 
        : tolerance(tolerance), minVoteThreshold(minVotes) {
    }

    void DynamicGrid::buildAuxLinesFromGraph(const BaseUGraphProperty& graph) {
        clearAllAuxLines();
        
        // Use dictionaries to count votes for each coordinate
        std::map<double, int> xVotes;                   // x-coordinate -> vote count
        std::map<double, int> yVotes;                   // y-coordinate -> vote count
        std::map<double, std::vector<int>> x2Vertices;  // x-coordinate -> vertex IDs
        std::map<double, std::vector<int>> y2Vertices;  // y-coordinate -> vertex IDs
        
        // Iterate through vertices to collect coordinates and count votes
        auto vertexRange = boost::vertices(graph);
        for (auto vit = vertexRange.first; vit != vertexRange.second; ++vit) {
            const BaseVertexProperty& vertex = graph[*vit];
            Coord2 coord = vertex.getCoord();
            int vertexId = vertex.getID();
            
            double x = coord.x();
            double y = coord.y();
            
            // Find or create entry for x-coordinate (vertical line candidate)
            bool foundXMatch = false;
            for (auto& pair : xVotes) {
                if (std::abs(pair.first - x) <= tolerance) {
                    pair.second++;
                    x2Vertices[pair.first].push_back(vertexId);
                    foundXMatch = true;
                    break;
                }
            }
            if (!foundXMatch) {
                xVotes[x] = 1;
                x2Vertices[x].push_back(vertexId);
            }
            
            // Find or create entry for y-coordinate (horizontal line candidate)
            bool foundYMatch = false;
            for (auto& pair : yVotes) {
                if (std::abs(pair.first - y) <= tolerance) {
                    pair.second++;
                    y2Vertices[pair.first].push_back(vertexId);
                    foundYMatch = true;
                    break;
                }
            }
            if (!foundYMatch) {
                yVotes[y] = 1;
                y2Vertices[y].push_back(vertexId);
            }
        }
        
        // Create horizontal auxiliary lines for coordinates that meet the threshold
        for (const auto& pair : yVotes) {
            if (pair.second >= minVoteThreshold) {
                AuxiliaryLine hLine(pair.first, true);
                hLine.voteCount = pair.second;
                hLine.vertexIds = y2Vertices[pair.first];
                horizontalAuxLines.push_back(hLine);
            }
        }
        
        // Create vertical auxiliary lines for coordinates that meet the threshold
        for (const auto& pair : xVotes) {
            if (pair.second >= minVoteThreshold) {
                AuxiliaryLine vLine(pair.first, false);
                vLine.voteCount = pair.second;
                vLine.vertexIds = x2Vertices[pair.first];
                verticalAuxLines.push_back(vLine);
            }
        }
        
        // Sort lines by position for consistency
        std::sort(horizontalAuxLines.begin(), horizontalAuxLines.end(),
                  [](const AuxiliaryLine& a, const AuxiliaryLine& b) {
                      return a.position < b.position;
                  });
        std::sort(verticalAuxLines.begin(), verticalAuxLines.end(),
                  [](const AuxiliaryLine& a, const AuxiliaryLine& b) {
                      return a.position < b.position;
                  });
    }

    void DynamicGrid::electKeyAuxLines() {
        // Simple voting algorithm: lines are already filtered by minVoteThreshold
        // Just sort them by vote count for better organization
        sortAuxLinesByVotes();
    }

    void DynamicGrid::sortAuxLinesByVotes() {
        // Sort horizontal lines by vote count (descending)
        std::sort(horizontalAuxLines.begin(), horizontalAuxLines.end(),
                  [](const AuxiliaryLine& a, const AuxiliaryLine& b) {
                      return a.voteCount > b.voteCount;
                  });
        
        // Sort vertical lines by vote count (descending)
        std::sort(verticalAuxLines.begin(), verticalAuxLines.end(),
                  [](const AuxiliaryLine& a, const AuxiliaryLine& b) {
                      return a.voteCount > b.voteCount;
                  });
    }

    std::vector<double> DynamicGrid::getKeyHorizontalPositions() const {
        std::vector<double> positions;
        for (const auto& hLine : horizontalAuxLines) {
            positions.push_back(hLine.position);
        }
        return positions;
    }

    std::vector<double> DynamicGrid::getKeyVerticalPositions() const {
        std::vector<double> positions;
        for (const auto& vLine : verticalAuxLines) {
            positions.push_back(vLine.position);
        }
        return positions;
    }

    int DynamicGrid::getTotalVoteCount() const {
        int totalVotes = 0;
        for (const auto& hLine : horizontalAuxLines) {
            totalVotes += hLine.voteCount;
        }
        for (const auto& vLine : verticalAuxLines) {
            totalVotes += vLine.voteCount;
        }
        return totalVotes;
    }

    int DynamicGrid::getKeyAuxLineCount() const {
        return static_cast<int>(horizontalAuxLines.size() + verticalAuxLines.size());
    }

    void DynamicGrid::printAuxLineInfo() const {
        std::cout << "=== Dynamic Grid Auxiliary Lines Info ===" << std::endl;
        std::cout << "Horizontal Lines (" << horizontalAuxLines.size() << "):" << std::endl;
        for (size_t i = 0; i < horizontalAuxLines.size(); ++i) {
            const auto& hLine = horizontalAuxLines[i];
            std::cout << "  H" << i << ": y=" << hLine.position 
                      << ", votes=" << hLine.voteCount 
                      << ", vertices=" << hLine.vertexIds.size() << std::endl;
        }
        
        std::cout << "Vertical Lines (" << verticalAuxLines.size() << "):" << std::endl;
        for (size_t i = 0; i < verticalAuxLines.size(); ++i) {
            const auto& vLine = verticalAuxLines[i];
            std::cout << "  V" << i << ": x=" << vLine.position 
                      << ", votes=" << vLine.voteCount 
                      << ", vertices=" << vLine.vertexIds.size() << std::endl;
        }
        
        std::cout << "Total votes: " << getTotalVoteCount() << std::endl;
        std::cout << "Total key lines: " << getKeyAuxLineCount() << std::endl;
    }

    void DynamicGrid::clearAllAuxLines() {
        horizontalAuxLines.clear();
        verticalAuxLines.clear();
    }

} // namespace Map