//------------------------------------------------------------------------------
// DVPositioning.cpp - Functions for positioning dangling vertices
//------------------------------------------------------------------------------

#include "DVPositioning.h"
#include "BaseUGraphProperty.h"
#include "Commons.h"
#include "CheckOverlap.h"
#include "DynamicGrid.h"
#include "MapFileReader.h"
#include "VisualizeSVG.h"

#include <iostream>
#include <vector>
#include <algorithm>
#include <limits>
#include <map>

#define EPSILON 1e-2

namespace Map {

    // Find all dangling vertices in the graph (not on auxiliary line intersections)
    std::vector<int> findDVs(const DynamicGrid& grid, const BaseUGraphProperty& graph) {
        std::vector<int> DVIDList;
        std::vector<double> hPositions = grid.getHALPositions();
        std::vector<double> vPositions = grid.getVALPositions();
        
        auto vp = boost::vertices(graph);
        for (auto vit = vp.first; vit != vp.second; ++vit) {
            const BaseVertexProperty& vertex = graph[*vit];
            int vertexID = vertex.getID();
            Coord2 pos = vertex.getCoord();
            
            bool isOnIntersection = false;
            for (double hPos : hPositions) {
                if (std::abs(pos.y() - hPos) < EPSILON) {
                    for (double vPos : vPositions) {
                        if (std::abs(pos.x() - vPos) < EPSILON) {
                            isOnIntersection = true;
                            break;
                        }
                    }
                    if (isOnIntersection) break;
                }
            }
            
            if (!isOnIntersection) {
                // the vertex does not locate on any intersections
                DVIDList.push_back(vertexID);
            }
        }
        return DVIDList;
    }
    
    // Check if vertex is on any horizontal auxiliary line
    bool isOnHAL(int vertexID, const DynamicGrid& grid, const BaseUGraphProperty& graph) {
        auto vertexDesc = getVertexDescriptor(vertexID);
        Coord2 pos = graph[vertexDesc].getCoord();

        std::vector<double> hPositions = grid.getHALPositions();
        for (double hPos : hPositions) {
            if (std::abs(pos.y() - hPos) < EPSILON) {
                return true;
            }
        }
        return false;
    }
    
    // Check if vertex is on any vertical auxiliary line
    bool isOnVAL(int vertexID, const DynamicGrid& grid, const BaseUGraphProperty& graph) {
        auto vertexDesc = getVertexDescriptor(vertexID);
        Coord2 pos = graph[vertexDesc].getCoord();

        std::vector<double> vPositions = grid.getVALPositions();
        for (double vPos : vPositions) {
            if (std::abs(pos.x() - vPos) < EPSILON) {
                return true;
            }
        }
        return false;
    }
    
    //---------------------------------------------------------------------------------------------------------
    //  Get adjacent auxiliary lines
    //---------------------------------------------------------------------------------------------------------

    std::vector<AuxiliaryLine> getAdjHALs(int vertexID, const DynamicGrid& grid, const BaseUGraphProperty& graph) {

        std::vector<AuxiliaryLine> adjHALs;
        bool flagN = true, flagS = true; // flagN: above all ALs; flagS: below all ALs

        BaseUGraphProperty::vertex_descriptor vertexDesc = getVertexDescriptor(vertexID);
        double currentX = graph[vertexDesc].getCoord().x();
        double currentY = graph[vertexDesc].getCoord().y();
        std::cout << "Initial X: " << currentX << " Initial Y: " << currentY << std::endl;

        for (const AuxiliaryLine& hline: grid.getHorizontalAuxLines()) {
            if (currentY < hline.getPosition()) {
                flagS = false;
            }
            else if (currentY > hline.getPosition()) {
                flagN = false;
            }
            else {
                continue;
            }
        }

        auto vp = boost::vertices(graph);
        std::vector<int> DVIDList = findDVs(grid, graph);
        bool flag = true;

        if (flagN) {
            for (BaseUGraphProperty::vertex_iterator vit = vp.first; vit != vp.second; ++vit) {
                auto it = std::find(DVIDList.begin(), DVIDList.end(), graph[*vit].getID());
                if (it != DVIDList.end()) { // the vertex is dangling
                    BaseUGraphProperty::vertex_descriptor tempVertexDesc = getVertexDescriptor(*it);
                    if (std::abs(graph[tempVertexDesc].getCoord().x() - currentX) < 1e-6 &&
                        graph[tempVertexDesc].getCoord().y() > currentY && 
                        graph[tempVertexDesc].getCoord().y() < grid.getHorizontalAuxLines()[0].getPosition()) {
                        flag = false;
                        std::cout << "Vertex " << graph[vertexDesc].getID() << " has no adjacent HALs" << std::endl;
                    }
                }
            }
            if (flag) {
                adjHALs.push_back(grid.getHorizontalAuxLines()[0]);
            }
        }
        else if (flagS) {
            for (BaseUGraphProperty::vertex_iterator vit = vp.first; vit != vp.second; ++vit) {
                auto it = std::find(DVIDList.begin(), DVIDList.end(), graph[*vit].getID());
                if (it != DVIDList.end()) { // the vertex is dangling
                    BaseUGraphProperty::vertex_descriptor tempVertexDesc = getVertexDescriptor(*it);
                    if (std::abs(graph[tempVertexDesc].getCoord().x() - currentX) < 1e-6 &&
                        graph[tempVertexDesc].getCoord().y() < currentY && 
                        graph[tempVertexDesc].getCoord().y() > grid.getHorizontalAuxLines().back().getPosition()) {
                        flag = false;
                        std::cout << "Vertex " << graph[vertexDesc].getID() << " has no adjacent HALs" << std::endl;
                    }
                }
            }
            if (flag) {
                adjHALs.push_back(grid.getHorizontalAuxLines().back());
            }
        }
        else if (!flagN && !flagS) {

            AuxiliaryLine cand1, cand2;
            double index1, index2;
            bool flag1 = true, flag2 = true;

            for (int i = 0; i < grid.getHorizontalAuxLines().size(); i++) {
                if (currentY < grid.getHorizontalAuxLines()[i].getPosition()) { // find the adjacent lines to be further selected
                    cand1 = grid.getHorizontalAuxLines()[i-1];
                    cand2 = grid.getHorizontalAuxLines()[i];
                    index1 = i-1;
                    index2 = i;
                    break;
                }
            }

            for (BaseUGraphProperty::vertex_iterator vit = vp.first; vit != vp.second; ++vit) {
                auto it = std::find(DVIDList.begin(), DVIDList.end(), graph[*vit].getID());
                if (it != DVIDList.end()) {
                    BaseUGraphProperty::vertex_descriptor tempVertexDesc = getVertexDescriptor(*it);
                    if (std::abs(graph[tempVertexDesc].getCoord().x() - currentX) < 1e-6 &&
                        graph[tempVertexDesc].getCoord().y() < currentY && 
                        graph[tempVertexDesc].getCoord().y() > grid.getHorizontalAuxLines()[index1].getPosition()) {
                        flag1 = false;
                    } else if (
                        std::abs(graph[tempVertexDesc].getCoord().x() - currentX) < 1e-6 &&
                        graph[tempVertexDesc].getCoord().y() > currentY && 
                        graph[tempVertexDesc].getCoord().y() < grid.getHorizontalAuxLines()[index2].getPosition()) {
                        flag2 = false;
                    }
                }
            }

            if (flag1) {
                adjHALs.push_back(cand1);
            }
            if (flag2) {
                adjHALs.push_back(cand2);
            }
        }

        else {
            throw std::runtime_error("Invalid flag combination in getAdjHALs");
        }

        return adjHALs;
    }
    
    std::vector<AuxiliaryLine> getAdjVALs(int vertexID, const DynamicGrid& grid, const BaseUGraphProperty& graph) {

        std::vector<AuxiliaryLine> adjVALs;
        bool flagW = true, flagE = true; 

        BaseUGraphProperty::vertex_descriptor vertexDesc = getVertexDescriptor(vertexID);
        double currentX = graph[vertexDesc].getCoord().x();
        double currentY = graph[vertexDesc].getCoord().y();
        for (const AuxiliaryLine& vline: grid.getVerticalAuxLines()) {
            if (currentX < vline.getPosition()) {
                flagE = false;
            }
            else if (currentX > vline.getPosition()) {
                flagW = false;
            }
            else {
                continue;
            }
        }

        auto vp = boost::vertices(graph);
        std::vector<int> DVIDList = findDVs(grid, graph);
        bool flag = true;

        if (flagW) {
            for (BaseUGraphProperty::vertex_iterator vit = vp.first; vit != vp.second; ++vit) {
                auto it = std::find(DVIDList.begin(), DVIDList.end(), graph[*vit].getID());
                if (it != DVIDList.end()) { // the vertex is dangling
                    BaseUGraphProperty::vertex_descriptor tempVertexDesc = getVertexDescriptor(*it);
                    if (std::abs(graph[tempVertexDesc].getCoord().y() - currentY) < 1e-6 &&
                        graph[tempVertexDesc].getCoord().x() > currentX && 
                        graph[tempVertexDesc].getCoord().x() < grid.getVerticalAuxLines()[0].getPosition()) {
                        flag = false;
                        std::cout << "Vertex " << graph[vertexDesc].getID() << " has no adjacent VALs" << std::endl;
                    }
                }
            }
            if (flag) {
                adjVALs.push_back(grid.getVerticalAuxLines()[0]);
            }
        }
        else if (flagE) {
            for (BaseUGraphProperty::vertex_iterator vit = vp.first; vit != vp.second; ++vit) {
                auto it = std::find(DVIDList.begin(), DVIDList.end(), graph[*vit].getID());
                if (it != DVIDList.end()) { // the vertex is dangling
                    BaseUGraphProperty::vertex_descriptor tempVertexDesc = getVertexDescriptor(*it);
                    if (std::abs(graph[tempVertexDesc].getCoord().y() - currentY) < 1e-6 &&
                        graph[tempVertexDesc].getCoord().x() < currentX && 
                        graph[tempVertexDesc].getCoord().x() > grid.getVerticalAuxLines().back().getPosition()) {
                        flag = false;
                        std::cout << "Vertex " << graph[vertexDesc].getID() << " has no adjacent VALs" << std::endl;
                    }
                }
            }
            if (flag) {
                adjVALs.push_back(grid.getVerticalAuxLines().back());
            }
        }
        else if (!flagW && !flagE) {

            AuxiliaryLine cand1, cand2;
            double index1, index2;
            bool flag1 = true, flag2 = true;

            for (int i = 0; i < grid.getVerticalAuxLines().size(); i++) {
                if (currentX < grid.getVerticalAuxLines()[i].getPosition()) { // find the adjacent lines to be further selected
                    cand1 = grid.getVerticalAuxLines()[i-1];
                    cand2 = grid.getVerticalAuxLines()[i];
                    index1 = i-1;
                    index2 = i;
                    break;
                }
            }

            for (BaseUGraphProperty::vertex_iterator vit = vp.first; vit != vp.second; ++vit) {
                auto it = std::find(DVIDList.begin(), DVIDList.end(), graph[*vit].getID());
                if (it != DVIDList.end()) {
                    BaseUGraphProperty::vertex_descriptor tempVertexDesc = getVertexDescriptor(*it);
                    if (std::abs(graph[tempVertexDesc].getCoord().y() - currentY) < 1e-6 &&
                        graph[tempVertexDesc].getCoord().x() < currentX && 
                        graph[tempVertexDesc].getCoord().x() > grid.getVerticalAuxLines()[index1].getPosition()) {
                        flag1 = false;
                    } else if (
                        std::abs(graph[tempVertexDesc].getCoord().y() - currentY) < 1e-6 &&
                        graph[tempVertexDesc].getCoord().x() > currentX && 
                        graph[tempVertexDesc].getCoord().x() < grid.getVerticalAuxLines()[index2].getPosition()) {
                        flag2 = false;
                    }
                }
            }

            if (flag1) {
                adjVALs.push_back(cand1);
            }
            if (flag2) {
                adjVALs.push_back(cand2);
            }
        }

        else {
            throw std::runtime_error("Invalid flag combination in getAdjVALs");
        }

        return adjVALs;
    }
    
    //---------------------------------------------------------------------------------------------------------
    //  Add auxiliary lines
    //---------------------------------------------------------------------------------------------------------

    void addHAL(double position, DynamicGrid& grid, const BaseUGraphProperty& graph) {
        grid.addHorizontalAuxLine(position);
    }
    
    void addVAL(double position, DynamicGrid& grid, const BaseUGraphProperty& graph) {
        grid.addVerticalAuxLine(position);
    }

    //---------------------------------------------------------------------------------------------------------
    //  Process dangling vertices
    //---------------------------------------------------------------------------------------------------------

    void processPDV(int vertexID, DynamicGrid& grid, BaseUGraphProperty& graph) {

        BaseUGraphProperty::vertex_descriptor vertexDesc = getVertexDescriptor(vertexID);
        Coord2 pos = graph[vertexDesc].getCoord();

        if (isOnHAL(vertexID, grid, graph)) {
            double currentX = pos.x();
            std::vector<AuxiliaryLine> adjVALs = getAdjVALs(vertexID, grid, graph);
            double minDistance = std::numeric_limits<double>::max();
            double bestX = currentX;

            bool flag = false;
            for (int i = 0; i < adjVALs.size(); ++i) {
                const AuxiliaryLine& adjVAL = adjVALs[i];
                Coord2 newPos = Coord2(adjVAL.getPosition(), pos.y());

                if (!overlapHappens(vertexID, newPos, graph)) { // there is no overlap!
                    flag = true;
                    double dist = std::abs(currentX - adjVAL.getPosition());
                    if (dist < minDistance) {
                        minDistance = dist;
                        bestX = adjVAL.getPosition();
                    }
                }
                else {
                    continue;
                }
            }

            // update the vertex position
            if (flag) {
                graph[vertexDesc].setCoord(bestX, pos.y());
            }
            else {
                addVAL(currentX, grid, graph);
            }
            
            return;
        }
        else if (isOnVAL(vertexID, grid, graph)) {

            // BaseUGraphProperty::vertex_descriptor vertexDesc = getVertexDescriptor(vertexID);
            // auto out_ep = boost::out_edges(vertexDesc, graph);
            // for (BaseUGraphProperty::out_edge_iterator oeit = out_ep.first; oeit != out_ep.second; ++oeit) {
            //     std::cout << graph[*oeit].Source().getID() << ": (" << graph[*oeit].Source().getCoord().x() << ", " << graph[*oeit].Source().getCoord().y() << ")" << std::endl;
            //     std::cout << graph[*oeit].Target().getID() << ": (" << graph[*oeit].Target().getCoord().x() << ", " << graph[*oeit].Target().getCoord().y() << ")" << std::endl;
            //     std::cout << std::endl;
            // }

            // std::cout << "Maybe Here?" << std::endl;
            // std::cout << std::endl;

            double currentY = pos.y();
            std::vector<AuxiliaryLine> adjHALs = getAdjHALs(vertexID, grid, graph);
            double minDistance = std::numeric_limits<double>::max();
            double bestY = currentY;

            std::cout << adjHALs.size() << std::endl;

            bool flag = false;
            for (int i = 0; i < adjHALs.size(); ++i) {
                const AuxiliaryLine& adjHAL = adjHALs[i];
                std::cout << adjHAL.getPosition() << std::endl;
                Coord2 newPos = Coord2(pos.x(), adjHAL.getPosition());

                if (!overlapHappens(vertexID, newPos, graph)) { // there is no overlap!
                    flag = true;
                    double dist = std::abs(currentY - adjHAL.getPosition());
                    if (dist < minDistance) {
                        minDistance = dist;
                        bestY = adjHAL.getPosition();
                    }
                }
                else {
                    continue;
                }
            }
            
            // update the vertex position
            if (flag) {
                graph[vertexDesc].setCoord(pos.x(), bestY);
            }
            else {
                addHAL(currentY, grid, graph);
            }
            // !!! update the adjacent edges?
            
            return;
        }
        
    }

    void processFDV(int vertexID, DynamicGrid& grid, BaseUGraphProperty& graph) {

        BaseUGraphProperty::vertex_descriptor vertexDesc = getVertexDescriptor(vertexID);
        Coord2 pos = graph[vertexDesc].getCoord();

        std::cout << "Initial coordinates: X: " << pos.x() << " Y: " << pos.y() << std::endl;

        double currentX = pos.x();
        std::vector<AuxiliaryLine> adjVALs = getAdjVALs(vertexID, grid, graph);
        double minDistance = std::numeric_limits<double>::max();
        double bestX = currentX;

        bool flag = false;
        for (int i = 0; i < adjVALs.size(); ++i) {
            const AuxiliaryLine& adjVAL = adjVALs[i];
            Coord2 newPos = Coord2(adjVAL.getPosition(), pos.y());

            std::cout << "Now we have a try from West to East. New X: " << adjVAL.getPosition() << " Y: " << pos.y() << std::endl;

            if (!overlapHappens(vertexID, newPos, graph)) { // there is no overlap!
                flag = true;
                double dist = std::abs(currentX - adjVAL.getPosition());
                if (dist < minDistance) {
                    minDistance = dist;
                    bestX = adjVAL.getPosition();
                }
            }
            else {
                std::cout << "Holy Shit! We have an overlap!" << std::endl;
                continue;
            }
        }

        // update the vertex position
        if (flag) {
            graph[vertexDesc].setCoord(bestX, pos.y());
        }
        else {
            addVAL(currentX, grid, graph);
        }

        pos = graph[vertexDesc].getCoord();
        double currentY = pos.y();
        std::vector<AuxiliaryLine> adjHALs = getAdjHALs(vertexID, grid, graph);
        minDistance = std::numeric_limits<double>::max();
        double bestY = currentY;

        flag = false;
        for (int i = 0; i < adjHALs.size(); ++i) {
            const AuxiliaryLine& adjHAL = adjHALs[i];
            Coord2 newPos = Coord2(pos.x(), adjHAL.getPosition());
            std::cout << "Now we have a try from North to South. New X: " << pos.x() << " New Y: " << adjHAL.getPosition() << std::endl;

            if (!overlapHappens(vertexID, newPos, graph)) { // there is no overlap!
                flag = true;
                double dist = std::abs(currentY - adjHAL.getPosition());
                if (dist < minDistance) {
                    minDistance = dist;
                    bestY = adjHAL.getPosition();
                }
            }
            else {
                std::cout << "Holy Shit! We have an overlap!" << std::endl;
                continue;
            }
        }
        
        // update the vertex position
        if (flag) {
            graph[vertexDesc].setCoord(pos.x(), bestY);
        }
        else {
            addHAL(currentY, grid, graph);
        }
        // !!! update the adjacent edges?
        
        return;
    }
    
    //---------------------------------------------------------------------------------------------------------
    //  Main function to position all dangling vertices
    //---------------------------------------------------------------------------------------------------------

    int positionDanglingVertices(
        std::vector<BaseVertexProperty>& vertexList, 
        std::vector<BaseEdgeProperty>& edgeList, 
        BaseUGraphProperty& graph,
        DynamicGrid& grid,
        const std::string& testCaseName) {
        
        try {
            std::cout << "=== Starting Dangling Vertex Positioning ===" << std::endl;
            
            int vertexNum = vertexList.size();
            int edgeNum = edgeList.size();
            
            // Create vertex ID to index mapping (similar to optimizeEdgeOrientation and optimizeVertexAlignment)
            std::map<unsigned int, int> vertexID2Index = createVertexID2Index(vertexList);
            
            // Create visualization before positioning
            createVisualization(vertexList, edgeList, "before_dv.svg");
            std::cout << "Created visualization: before_dv.svg" << std::endl;
            
            int modifiedCount = 0;
            
            // Find all dangling vertices
            std::vector<int> danglingVertices = findDVs(grid, graph);
            
            std::cout << "Found " << danglingVertices.size() << " dangling vertices" << std::endl;
            
            // First process vertices that are partially aligned
            for (int vertexID : danglingVertices) {
                bool isOnHorizontal = isOnHAL(vertexID, grid, graph);
                bool isOnVertical = isOnVAL(vertexID, grid, graph);
                
                if ((isOnHorizontal && !isOnVertical) || (!isOnHorizontal && isOnVertical)) {
                    std::cout << std::endl;
                    std::cout << std::endl;
                    std::cout << "Processing partially aligned vertex " << vertexID << std::endl;
                    processPDV(vertexID, grid, graph);
                    modifiedCount++;
                    std::cout << std::endl;
                    std::cout << std::endl;
                }
            }
            
            // Then process fully dangling vertices
            for (int vertexID : danglingVertices) {
                bool isOnHorizontal = isOnHAL(vertexID, grid, graph);
                bool isOnVertical = isOnVAL(vertexID, grid, graph);
                
                if (!isOnHorizontal && !isOnVertical) {
                    std::cout << std::endl;
                    std::cout << std::endl;
                    std::cout << "Processing fully dangling vertex " << vertexID << std::endl;
                    processFDV(vertexID, grid, graph);
                    modifiedCount++;
                    std::cout << std::endl;
                    std::cout << std::endl;
                }
            }
            
            std::cout << "Positioned " << modifiedCount << " dangling vertices" << std::endl;
            
            if (modifiedCount == 0) {
                std::cout << "No vertices were modified, skipping updates." << std::endl;
                return 0;
            }
            
            // Update vertexList with new coordinates from graph (similar to optimizeEdgeOrientation)
            std::cout << "\n=== Updating vertexList ===" << std::endl;
            std::pair<BaseUGraphProperty::vertex_iterator, BaseUGraphProperty::vertex_iterator> vp = vertices(graph);
            for (BaseUGraphProperty::vertex_iterator vi = vp.first; vi != vp.second; ++vi) {
                const BaseVertexProperty& vertex = graph[*vi];
                auto it = vertexID2Index.find(vertex.getID());
                if (it != vertexID2Index.end()) {
                    int idx = it->second;
                    vertexList[idx].setCoord(vertex.getCoord().x(), vertex.getCoord().y());
                    std::cout << "Updated vertexList[" << idx << "] (ID: " << vertex.getID() 
                             << ") to (" << vertex.getCoord().x() << ", " << vertex.getCoord().y() << ")" << std::endl;
                }
            }
            
            // Update edge angles in both graph and edgeList (similar to optimizeEdgeOrientation)
            std::cout << "\n=== Updating edge angles ===" << std::endl;
            std::pair<BaseUGraphProperty::edge_iterator, BaseUGraphProperty::edge_iterator> ep = edges(graph);
            for (BaseUGraphProperty::edge_iterator ei = ep.first; ei != ep.second; ++ei) {
                BaseEdgeProperty& edge = graph[*ei];
                
                BaseUGraphProperty::vertex_descriptor source_desc = boost::source(*ei, graph);
                BaseUGraphProperty::vertex_descriptor target_desc = boost::target(*ei, graph);
                
                // Recalculate angle with updated coordinates
                double oldAngle = edge.Angle();
                double newAngle = calculateAngle(graph[source_desc], graph[target_desc]);
                
                // Update angle in graph edge
                edge.setAngle(newAngle);
                
                // Update angle in edgeList
                edgeList[edge.ID()].setAngle(newAngle);
                
                std::cout << "Edge " << edge.ID() << ": angle " << oldAngle << " -> " << newAngle << std::endl;
            }
            
            // Create visualization after positioning
            std::string outputFile = "output/" + testCaseName + "_4.svg";
            createVisualization(vertexList, edgeList, outputFile);
            std::cout << "\nCreated visualization: " << outputFile << std::endl;
            
            std::cout << "\n=== Dangling Vertex Positioning Completed Successfully ===" << std::endl;
            
            std::cout << std::endl;
            std::cout << "Current Graph:" << std::endl;
            std::cout << "Current Graph:" << std::endl;
            std::cout << "Current Graph:" << std::endl;
            std::cout << std::endl;
            for (BaseUGraphProperty::vertex_iterator vi = vp.first; vi != vp.second; ++vi) {
                std::cout << graph[*vi].getID() << ": (" << graph[*vi].getCoord().x() << ", " << graph[*vi].getCoord().y() << ")" << std::endl;
            }
            std::cout << std::endl;

            return modifiedCount;
            
        } catch (std::exception& e) {
            std::cerr << "Error in positionDanglingVertices: " << e.what() << std::endl;
            return -1;
        } catch (...) {
            std::cerr << "Unknown error occurred in positionDanglingVertices" << std::endl;
            return -1;
        }
    }

} // namespace Map