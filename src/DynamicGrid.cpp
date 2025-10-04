//------------------------------------------------------------------------------
// DynamicGrid.cpp - Dynamic Grid Implementation
//------------------------------------------------------------------------------

#include "DynamicGrid.h"
#include <iostream>

namespace Map {

    DynamicGrid::DynamicGrid() {}

    DynamicGrid::DynamicGrid(double tolerance, double minVotes): 
        tolerance(tolerance), minVoteThreshold(minVotes) {
    }

    void DynamicGrid::buildAuxLines(const BaseUGraphProperty& graph) {
        clearAllAuxLines();
        
        // Use dictionaries to count votes for each coordinate 
        // !!! (sorted by coordinate value in ascending order)
        std::map<double, int> vALVotes;                   // x-coordinate -> vote count (ascending order)
        std::map<double, int> hALVotes;                   // y-coordinate -> vote count (ascending order)
        std::map<double, std::vector<int>> vAL2Vertices;  // x-coordinate -> vertex IDs
        std::map<double, std::vector<int>> hAL2Vertices;  // y-coordinate -> vertex IDs
        
        auto vp = boost::vertices(graph);
        for (auto vit = vp.first; vit != vp.second; ++vit) {
            const BaseVertexProperty& vertex = graph[*vit];
            Coord2 coord = vertex.getCoord();
            int vertexID = vertex.getID();
            
            double x = coord.x();
            double y = coord.y();
            
            // Find or create entry for x-coordinate (vertical line candidate)
            bool foundVAL = false;
            for (auto& pair : vALVotes) {
                if (std::abs(pair.first - x) <= tolerance) {
                    pair.second++;
                    vAL2Vertices[pair.first].push_back(vertexID);
                    foundVAL = true;
                    break;
                }
            }
            if (!foundVAL) {
                vALVotes[x] = 1;
                vAL2Vertices[x].push_back(vertexID);
            }
            
            // Find or create entry for y-coordinate (horizontal line candidate)
            bool foundHAL = false;
            for (auto& pair : hALVotes) {
                if (std::abs(pair.first - y) <= tolerance) {
                    pair.second++;
                    hAL2Vertices[pair.first].push_back(vertexID);
                    foundHAL = true;
                    break;
                }
            }
            if (!foundHAL) {
                hALVotes[y] = 1;
                hAL2Vertices[y].push_back(vertexID);
            }
        }
        
        // Create horizontal auxiliary lines for coordinates that meet the threshold
        for (const auto& pair : hALVotes) {
            if (pair.second >= minVoteThreshold) {
                AuxiliaryLine hLine(pair.first, true);
                // !!! not elegant at all (not unified)
                hLine.setVoteCount(pair.second);
                hLine.setVertexIDs(hAL2Vertices[pair.first]);
                horizontalAuxLines.push_back(hLine);
            }
        }
        
        // Create vertical auxiliary lines for coordinates that meet the threshold
        for (const auto& pair : vALVotes) {
            if (pair.second >= minVoteThreshold) {
                AuxiliaryLine vLine(pair.first, false);
                // !!! not elegant at all (not unified)
                vLine.setVoteCount(pair.second);
                vLine.setVertexIDs(vAL2Vertices[pair.first]);
                verticalAuxLines.push_back(vLine);
            }
        }
    }

    std::vector<double> DynamicGrid::getHALPositions() const {
        std::vector<double> positions;
        for (const auto& hLine : horizontalAuxLines) {
            positions.push_back(hLine.getPosition());
        }
        return positions;
    }

    std::vector<double> DynamicGrid::getVALPositions() const {
        std::vector<double> positions;
        for (const auto& vLine : verticalAuxLines) {
            positions.push_back(vLine.getPosition());
        }
        return positions;
    }

    // !!! why static type?
    int DynamicGrid::getKeyAuxLineCount() const {
        return static_cast<int>(horizontalAuxLines.size() + verticalAuxLines.size());
    }

    // no
    void DynamicGrid::printAuxLineInfo() const {
        std::cout << "=== Dynamic Grid Auxiliary Lines Info ===" << std::endl;
        std::cout << "Horizontal Lines (" << horizontalAuxLines.size() << "):" << std::endl;
        for (size_t i = 0; i < horizontalAuxLines.size(); ++i) {
            const auto& hLine = horizontalAuxLines[i];
            std::cout << "  H" << i << ": y=" << hLine.getPosition() 
                      << ", votes=" << hLine.getVoteCount() 
                      << ", vertices=" << hLine.getVertexIDs().size() << std::endl;
        }
        
        std::cout << "Vertical Lines (" << verticalAuxLines.size() << "):" << std::endl;
        for (size_t i = 0; i < verticalAuxLines.size(); ++i) {
            const auto& vLine = verticalAuxLines[i];
            std::cout << "  V" << i << ": x=" << vLine.getPosition() 
                      << ", votes=" << vLine.getVoteCount() 
                      << ", vertices=" << vLine.getVertexIDs().size() << std::endl;
        }
        
        std::cout << "Total key lines: " << getKeyAuxLineCount() << std::endl;
    }

    void DynamicGrid::clearAllAuxLines() {
        horizontalAuxLines.clear();
        verticalAuxLines.clear();
    }

    void DynamicGrid::addHorizontalAuxLine(double position) {
        // Check if a horizontal line already exists at this position (within tolerance)
        for (const auto& hLine : horizontalAuxLines) {
            if (std::abs(hLine.getPosition() - position) < 1e-9) {
                std::cout << "Horizontal auxiliary line already exists at y=" << hLine.getPosition() << std::endl;
                return;
            }
        }
        
        // Create new horizontal auxiliary line with initial vote count of 1
        AuxiliaryLine newLine(position, true, 1);
        horizontalAuxLines.push_back(newLine);
        
        // Keep the lines sorted by position
        std::sort(horizontalAuxLines.begin(), horizontalAuxLines.end(),
                  [](const AuxiliaryLine& a, const AuxiliaryLine& b) {
                      return a.getPosition() < b.getPosition();
                  });
        
        std::cout << "Added horizontal auxiliary line at y=" << position << std::endl;
    }

    void DynamicGrid::addVerticalAuxLine(double position) {
        // Check if a vertical line already exists at this position (within tolerance)
        for (const auto& vLine : verticalAuxLines) {
            if (std::abs(vLine.getPosition() - position) < 1e-9) {
                std::cout << "Vertical auxiliary line already exists at x=" << vLine.getPosition() << std::endl;
                return;
            }
        }
        
        // Create new vertical auxiliary line with initial vote count of 1
        AuxiliaryLine newLine(position, false, 1);
        verticalAuxLines.push_back(newLine);
        
        // Keep the lines sorted by position
        std::sort(verticalAuxLines.begin(), verticalAuxLines.end(),
                  [](const AuxiliaryLine& a, const AuxiliaryLine& b) {
                      return a.getPosition() < b.getPosition();
                  });
        
        std::cout << "Added vertical auxiliary line at x=" << position << std::endl;
    }

    // Placeholder implementation for electKeyAuxLines
    // TODO: Implement the logic to select key auxiliary lines if needed
    void DynamicGrid::electKeyAuxLines() {
        // Empty implementation for now
        // Future: Add logic to filter/select key auxiliary lines based on importance
    }

} // namespace Map