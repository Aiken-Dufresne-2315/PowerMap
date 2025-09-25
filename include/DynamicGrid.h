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
        double              position;       // x-coordinate for vertical line, y-coordinate for horizontal line
        bool                isHorizontal;   // true for horizontal line, false for vertical line        
        int                 voteCount;      // number of vertices that "vote" for this line
        std::vector<int>    vertexIds;      // IDs of vertices aligned to this line

        AuxiliaryLine(double pos, bool isH)
            : position(pos), isHorizontal(isH), voteCount(0) {}
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
        void sortAuxLinesByVotes();
        
    public:
        // Constructor
        DynamicGrid(double tolerance = 20.0, double minVotes = 2.0);
        
        // Core functionality
        void buildAuxLinesFromGraph(const BaseUGraphProperty& graphProp);
        void electKeyAuxLines();
        
        // Getters
        const std::vector<AuxiliaryLine>& getHorizontalAuxLines() const { return horizontalAuxLines; }
        const std::vector<AuxiliaryLine>& getVerticalAuxLines() const { return verticalAuxLines; }
        
        std::vector<double> getKeyHorizontalPositions() const;
        std::vector<double> getKeyVerticalPositions() const;
        
        // Information queries
        int getTotalVoteCount() const;
        int getKeyAuxLineCount() const;

        // Configuration
        void setToleranceAlignDist(double tolerance) { tolerance = tolerance; }
        void setMinVoteThreshold(double threshold) { minVoteThreshold = threshold; }

        // Debug and visualization
        void printAuxLineInfo() const;
        void clearAllAuxLines();
    };

} // namespace Map

#endif // _Map_DynamicGrid_H