//------------------------------------------------------------------------------
// DynamicGrid.h - Dynamic Grid for Key Auxiliary Lines Management
//------------------------------------------------------------------------------

#ifndef _Map_DynamicGrid_H
#define _Map_DynamicGrid_H

#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <cmath>
#include "BaseUGraphProperty.h"
#include "Coord2.h"

namespace Map {

    // Structure to represent an auxiliary line with voting information
    struct AuxiliaryLine {

    private:
        double              position;       // x-coordinate for vertical line, y-coordinate for horizontal line
        bool                isHorizontal;   // true for horizontal line, false for vertical line        
        int                 voteCount;      // number of vertices that "vote" for this line
        std::vector<int>    vertexIDs;      // IDs of vertices on this line

    public:
        AuxiliaryLine() {}

        AuxiliaryLine(double pos, bool isH, int vc = 0): 
            position(pos), isHorizontal(isH), voteCount(vc) {}

        void setPosition(double pos)                { position = pos; }
        void setVoteCount(int vc)                   { voteCount = vc; }
        void setVertexIDs(std::vector<int> vids)    { vertexIDs = vids; }

        double              getPosition() const { return position; }
        bool                getIsHorizontal() const { return isHorizontal; }
        int                 getVoteCount() const { return voteCount; }
        std::vector<int>    getVertexIDs() const { return vertexIDs; }

        AuxiliaryLine& operator = (const AuxiliaryLine& line) {
            if (this != &line){
                this->position = line.position;
                this->isHorizontal = line.isHorizontal;
                this->voteCount = line.voteCount;
                this->vertexIDs = line.vertexIDs;
            }
            return *this;
        }
    };

    // Main class for managing dynamic grid of auxiliary lines
    class DynamicGrid {
    private:
        std::vector<AuxiliaryLine> horizontalAuxLines;  // horizontal auxiliary lines
        std::vector<AuxiliaryLine> verticalAuxLines;    // vertical auxiliary lines
        
        // Parameters for line selection
        double  tolerance;          // tolerance distance for vertex alignment
        double  minVoteThreshold;   // minimum votes required for a line to be considered "key"

        // Helper functions
        void sortAuxLines();
        
    public:
        // Constructor
        DynamicGrid();
        DynamicGrid(double tolerance = 2.315, double minVotes = 2.0);
        
        // Core functionality
        void buildAuxLines(const BaseUGraphProperty& graphProp);
        void electKeyAuxLines();
        
        // Getters
        const std::vector<AuxiliaryLine>& getHorizontalAuxLines() const { return horizontalAuxLines; }
        const std::vector<AuxiliaryLine>& getVerticalAuxLines() const { return verticalAuxLines; }
        
        std::vector<double> getHALPositions() const;
        std::vector<double> getVALPositions() const;
        
        // Information queries
        int getKeyAuxLineCount() const;

        // Configuration
        void setTolerance(double tolerance) { this->tolerance = tolerance; }
        void setMinVote(double threshold) { this->minVoteThreshold = threshold; }

        // Dynamic line addition
        void addHorizontalAuxLine(double position);
        void addVerticalAuxLine(double position);
        
        // Update auxiliary line positions (for spacing optimization)
        void updateHorizontalLinePositions(const std::vector<double>& newPositions);
        void updateVerticalLinePositions(const std::vector<double>& newPositions);
        
        // Rebuild vertex-line mappings (updates vertexIDs for all auxiliary lines)
        void rebuildVertexLineMappings(const BaseUGraphProperty& graph);

        // Debug and visualization
        void printAuxLineInfo() const;
        void clearAllAuxLines();
    };

} // namespace Map

#endif // _Map_DynamicGrid_H