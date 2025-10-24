#include "CheckOverlap.h"
#include "BaseEdgeProperty.h"
#include "BaseVertexProperty.h"
#include "BaseUGraphProperty.h"
#include "Commons.h"
#include "SpatialGrid.h"

#include <cmath>
#include <deque>
#include <memory>
#include <boost/config.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/iteration_macros.hpp>

namespace Map {
    const double EPSILON = 1e-3;
    
    bool isCollinear(double x, double y, double x_A, double y_A, double x_B, double y_B){
        double cross_product = (y - y_A) * (x_B - x_A) - (x - x_A) * (y_B - y_A);
        return std::abs(cross_product) < EPSILON;
    }   

    bool VVOverlap(const BaseVertexProperty& vertex_1, const BaseVertexProperty& vertex_2) {
        return (std::abs(vertex_1.getCoord().x() - vertex_2.getCoord().x()) < EPSILON) &&
               (std::abs(vertex_1.getCoord().y() - vertex_2.getCoord().y()) < EPSILON);
    }

    bool VEOverlap(const BaseVertexProperty& vertex, const BaseEdgeProperty& edge) {
        double x_A = edge.Source().getCoord().x();
        double y_A = edge.Source().getCoord().y();
        double x_B = edge.Target().getCoord().x();
        double y_B = edge.Target().getCoord().y();

        double x_P = vertex.getCoord().x();
        double y_P = vertex.getCoord().y();

        if (isCollinear(x_P, y_P, x_A, y_A, x_B, y_B)) {
            if (std::abs(x_A - x_B) < EPSILON) { // vertical line
                return (y_P > std::min(y_A, y_B)) && (y_P < std::max(y_A, y_B));
            }     
            else {
                return (x_P > std::min(x_A, x_B)) && (x_P < std::max(x_A, x_B));
            }
        }
        else {
            return false;
        }
    }

    // check if two edges overlap
    bool EEOverlap(const BaseEdgeProperty& edge_1, const BaseEdgeProperty& edge_2) {
        double x_A = edge_1.Source().getCoord().x();
        double y_A = edge_1.Source().getCoord().y();
        double x_B = edge_1.Target().getCoord().x();
        double y_B = edge_1.Target().getCoord().y();

        double x_C = edge_2.Source().getCoord().x();
        double y_C = edge_2.Source().getCoord().y();
        double x_D = edge_2.Target().getCoord().x();
        double y_D = edge_2.Target().getCoord().y();

        if (isCollinear(x_C, y_C, x_A, y_A, x_B, y_B) && isCollinear(x_D, y_D, x_A, y_A, x_B, y_B)) {
            if (std::abs(x_A - x_B) < EPSILON) { 
                // A, B, C, D are on the same vertical line.
                double a = std::min(y_A, y_B);
                double b = std::max(y_A, y_B);
                double c = std::min(y_C, y_D);
                double d = std::max(y_C, y_D); 
                return !(c >= b || d <= a);
            }     
            else {
                double a = std::min(x_A, x_B);
                double b = std::max(x_A, x_B);
                double c = std::min(x_C, x_D);
                double d = std::max(x_C, x_D);    
                return !(c >= b || d <= a);
            }
        }
        else {
            return false;
        }
    }

    bool overlapHappens(int vertexID, const Coord2& newPos, const BaseUGraphProperty& graph) {
        // current vertex descriptor
        BaseUGraphProperty::vertex_descriptor VD = getVertexDescriptor(vertexID);

        auto vp = boost::vertices(graph);
        auto ep = boost::edges(graph);

        auto oep = boost::out_edges(VD, graph);

        // !!! Remember: check overlap with the new vertex position and the corresponding edges!

        BaseVertexProperty tempNewV = BaseVertexProperty(graph[VD]);
        tempNewV.setCoord(newPos);

        // std::cout << tempNewV.getID() << ": (" << tempNewV.getCoord().x() << ", " << tempNewV.getCoord().y() << ")" << std::endl;

        std::set<int> outVertexIDs;
        std::set<int> outEdgeIDs;

        // // !!! confusing checking result here !!!
        // if (vertexID == 17) {
        //     std::cout << std::endl;
        //     BaseUGraphProperty::vertex_descriptor tempVD = getVertexDescriptor(10);
        //     std::cout << graph[tempVD].getID() << " " << graph[tempVD].getCoord().x() << " " << graph[tempVD].getCoord().y() << std::endl;
        //     std::cout << "All hail Lelouch!" << std::endl;
        //     for (BaseUGraphProperty::out_edge_iterator oeit = oep.first; oeit != oep.second; ++oeit) {
        //         std::cout << graph[*oeit].Source().getID() << " " << graph[*oeit].Source().getCoord().x() << " " << graph[*oeit].Source().getCoord().y() << std::endl;
        //         std::cout << graph[*oeit].Target().getID() << " " << graph[*oeit].Target().getCoord().x() << " " << graph[*oeit].Target().getCoord().y() << std::endl;
        //         std::cout << std::endl;
        //     }
        // }

        for (BaseUGraphProperty::out_edge_iterator oeit = oep.first; oeit != oep.second; ++oeit) {
            outEdgeIDs.insert(graph[*oeit].ID());
            if (graph[*oeit].Source().getID() == vertexID) {
                outVertexIDs.insert(graph[*oeit].Target().getID());
            }
            else {
                outVertexIDs.insert(graph[*oeit].Source().getID());
            }
        }

        // Store vertices persistently so references remain valid
        // Use deque instead of vector to avoid reference invalidation on reallocation
        std::deque<BaseVertexProperty> vertexStorage;
        std::vector<BaseEdgeProperty> newOutEdges;  
        
        for(BaseUGraphProperty::out_edge_iterator oeit = oep.first; oeit != oep.second; ++oeit) {
            // Create deep copies of vertices and store them persistently
            BaseVertexProperty tempSource = graph[*oeit].Source();
            BaseVertexProperty tempTarget = graph[*oeit].Target();

            if (tempSource.getID() == vertexID) {
                tempSource.setCoord(newPos);
            }
            else {
                tempTarget.setCoord(newPos);
            }
            
            // Store vertices in vector so they persist (important: references in BaseEdgeProperty must remain valid!)
            vertexStorage.push_back(tempSource);
            vertexStorage.push_back(tempTarget);
            
            // Create a new edge with references to the persistent vertex copies
            BaseEdgeProperty tempEdge(vertexStorage[vertexStorage.size()-2], 
                                     vertexStorage[vertexStorage.size()-1], 
                                     graph[*oeit].ID(), 
                                     graph[*oeit].Angle());
            newOutEdges.push_back(tempEdge);
        } 

        // // !!! output something confusing here !!!
        // std::cout << "Glory is mine!" << std::endl;
        // for (auto newOutEdge: newOutEdges) {
        //     std::cout << newOutEdge.Source().getID() << ": (" << newOutEdge.Source().getCoord().x() << ", " << newOutEdge.Source().getCoord().y() << ")" << std::endl;
        //     std::cout << newOutEdge.Target().getID() << ": (" << newOutEdge.Target().getCoord().x() << ", " << newOutEdge.Target().getCoord().y() << ")" << std::endl;
        //     std::cout << std::endl;
        // }

        // 1. V-V checking
        std::cout << "Checking overlap 1..." << std::endl;
        for (BaseUGraphProperty::vertex_iterator vit = vp.first; vit != vp.second; ++vit) {
            BaseVertexProperty tempV = graph[*vit];
            if (vertexID != tempV.getID() && VVOverlap(tempNewV, tempV)) {
                std::cout << std::endl;
                std::cout << "Checked vertex " << tempV.getID() << ": (" << tempV.getCoord().x() << ", " << tempV.getCoord().y() << ")" << std::endl;
                std::cout << "Current vertex " << tempNewV.getID() << ": (" << tempNewV.getCoord().x() << ", " << tempNewV.getCoord().y() << ")" << std::endl;
                std::cout << "VVOverlap(1) happens" << std::endl;
                return true;
            }
        }

        // 2. V-E checking

        std::cout << "Checking overlap 2.1..." << std::endl;
        for (BaseUGraphProperty::edge_iterator eit = ep.first; eit != ep.second; ++eit) {
            int tempID = graph[*eit].ID();
            if (outEdgeIDs.find(tempID) == outEdgeIDs.end()) { 
                // graph[*eit] is not an out-edge of v

                if (VEOverlap(tempNewV, graph[*eit])) {
                    std::cout << std::endl;
                    std::cout << "Current vertex: " << tempNewV.getCoord().x() << " " << tempNewV.getCoord().y() << std::endl;
                    std::cout << "Checked edge source " << graph[*eit].Source().getID() << ": (" << graph[*eit].Source().getCoord().x() << ", " << graph[*eit].Source().getCoord().y() << ")" << std::endl;
                    std::cout << "Checked edge target " << graph[*eit].Target().getID() << ": (" << graph[*eit].Target().getCoord().x() << ", " << graph[*eit].Target().getCoord().y() << ")" << std::endl;
                    std::cout << "VEOverlap(2.1) happens" << std::endl;
                    return true;
                }
            }
        }

        std::cout << "Checking overlap 2.2..." << std::endl;
        for (auto newOutEdge: newOutEdges) {
            for (BaseUGraphProperty::vertex_iterator vit = vp.first; vit != vp.second; ++vit) {
                if (graph[*vit].getID() != vertexID && 
                    outVertexIDs.find(graph[*vit].getID()) == outVertexIDs.end() && 
                    VEOverlap(graph[*vit], newOutEdge)) {
                    // an out-edge of v overlaps with an irrelevant vertex
                    std::cout << std::endl;
                    std::cout << "Checked vertex " << graph[*vit].getID() << ": (" << graph[*vit].getCoord().x() << ", " << graph[*vit].getCoord().y() << ")" << std::endl;
                    std::cout << "Current edge source " << newOutEdge.Source().getID() << ": (" << newOutEdge.Source().getCoord().x() << ", " << newOutEdge.Source().getCoord().y() << ")" << std::endl;
                    std::cout << "Current edge target " << newOutEdge.Target().getID() << ": (" << newOutEdge.Target().getCoord().x() << ", " << newOutEdge.Target().getCoord().y() << ")" << std::endl;
                    std::cout << "VEOverlap(2.2) happens" << std::endl;
                    return true;
                }
            }
        }

        // 3.1. check: OUT(v) and E-OUT(v)
        std::cout << "Checking overlap 3.1..." << std::endl;
        for (BaseUGraphProperty::edge_iterator eit = ep.first; eit != ep.second; ++eit) {
            if(outEdgeIDs.find(graph[*eit].ID()) != outEdgeIDs.end()) {
                // graph[*eit] is an out-edge of v
                continue;
            }
            
            // graph[*eit] is not an out-edge of v
            for (BaseEdgeProperty newOutEdge: newOutEdges) {
                if (EEOverlap(graph[*eit], newOutEdge)) {
                    std::cout << std::endl;
                    std::cout << "Checked edge source " << graph[*eit].Source().getID() << ": (" << graph[*eit].Source().getCoord().x() << ", " << graph[*eit].Source().getCoord().y() << ")" << std::endl;
                    std::cout << "Checked edge target " << graph[*eit].Target().getID() << ": (" << graph[*eit].Target().getCoord().x() << ", " << graph[*eit].Target().getCoord().y() << ")" << std::endl;
                    std::cout << "Current edge source " << newOutEdge.Source().getID() << ": (" << newOutEdge.Source().getCoord().x() << ", " << newOutEdge.Source().getCoord().y() << ")" << std::endl;
                    std::cout << "Current edge target " << newOutEdge.Target().getID() << ": (" << newOutEdge.Target().getCoord().x() << ", " << newOutEdge.Target().getCoord().y() << ")" << std::endl;
                    std::cout << "EEOverlap(3.1) happens" << std::endl;
                    return true;
                }
            }
        }

        // 3.2. check: OUT(v) and OUT(v)
        std::cout << "Checking overlap 3.2..." << std::endl;
        for (BaseEdgeProperty newOutEdge1: newOutEdges) {
            for (BaseEdgeProperty newOutEdge2: newOutEdges) {
                if (newOutEdge1.ID() != newOutEdge2.ID() && 
                    EEOverlap(newOutEdge1, newOutEdge2)) {
                    std::cout << std::endl;
                    std::cout << newOutEdge1.Source().getID() << ": (" << newOutEdge1.Source().getCoord().x() << ", " << newOutEdge1.Source().getCoord().y() << ")" << std::endl;
                    std::cout << newOutEdge1.Target().getID() << ": (" << newOutEdge1.Target().getCoord().x() << ", " << newOutEdge1.Target().getCoord().y() << ")" << std::endl;
                    std::cout << newOutEdge2.Source().getID() << ": (" << newOutEdge2.Source().getCoord().x() << ", " << newOutEdge2.Source().getCoord().y() << ")" << std::endl;
                    std::cout << newOutEdge2.Target().getID() << ": (" << newOutEdge2.Target().getCoord().x() << ", " << newOutEdge2.Target().getCoord().y() << ")" << std::endl;
                    std::cout << "EEOverlap(3.2) happens" << std::endl;
                    return true;
                }
            }
        }
        
        return false;
    }

    bool overlapHappensOptimized(int vertexID, const Coord2& newPos, 
                                  const BaseUGraphProperty& graph,
                                  SpatialGrid* spatialGrid) {
        // 如果没有提供空间网格，创建一个临时的
        std::unique_ptr<SpatialGrid> tempGrid;
        if (spatialGrid == nullptr) {
            // 自动估算合适的网格大小（可以根据图的边长分布调整）
            tempGrid = std::make_unique<SpatialGrid>(2.0);
            tempGrid->buildFromGraph(graph);
            spatialGrid = tempGrid.get();
        }

        // 获取当前顶点描述符
        BaseUGraphProperty::vertex_descriptor VD = getVertexDescriptor(vertexID);

        // 收集出边信息
        auto oep = boost::out_edges(VD, graph);
        std::set<int> outVertexIDs;
        std::set<int> outEdgeIDs;

        for (BaseUGraphProperty::out_edge_iterator oeit = oep.first; oeit != oep.second; ++oeit) {
            outEdgeIDs.insert(graph[*oeit].ID());
            if (graph[*oeit].Source().getID() == vertexID) {
                outVertexIDs.insert(graph[*oeit].Target().getID());
            }
            else {
                outVertexIDs.insert(graph[*oeit].Source().getID());
            }
        }

        // 创建临时顶点（新位置）
        BaseVertexProperty tempNewV = BaseVertexProperty(graph[VD]);
        tempNewV.setCoord(newPos);

        // 存储更新后的出边（用于检查）
        std::deque<BaseVertexProperty> vertexStorage;
        std::vector<BaseEdgeProperty> newOutEdges;
        
        for(BaseUGraphProperty::out_edge_iterator oeit = oep.first; oeit != oep.second; ++oeit) {
            BaseVertexProperty tempSource = graph[*oeit].Source();
            BaseVertexProperty tempTarget = graph[*oeit].Target();

            if (tempSource.getID() == vertexID) {
                tempSource.setCoord(newPos);
            }
            else {
                tempTarget.setCoord(newPos);
            }
            
            vertexStorage.push_back(tempSource);
            vertexStorage.push_back(tempTarget);
            
            BaseEdgeProperty tempEdge(vertexStorage[vertexStorage.size()-2], 
                                     vertexStorage[vertexStorage.size()-1], 
                                     graph[*oeit].ID(), 
                                     graph[*oeit].Angle());
            newOutEdges.push_back(tempEdge);
        }

        // ============ 优化的重叠检查 ============
        
        // 1. V-V检查：只检查新位置附近的顶点
        std::cout << "Checking overlap 1 (optimized)..." << std::endl;
        std::vector<int> nearbyVertexIDs = spatialGrid->getNearbyVertices(newPos, 1);
        
        for (int nearbyVertexID : nearbyVertexIDs) {
            if (nearbyVertexID == vertexID) continue;
            
            BaseUGraphProperty::vertex_descriptor nearbyVD = getVertexDescriptor(nearbyVertexID);
            const BaseVertexProperty& nearbyVertex = graph[nearbyVD];
            
            if (VVOverlap(tempNewV, nearbyVertex)) {
                std::cout << std::endl;
                std::cout << "Checked vertex " << nearbyVertex.getID() << ": (" 
                         << nearbyVertex.getCoord().x() << ", " << nearbyVertex.getCoord().y() << ")" << std::endl;
                std::cout << "Current vertex " << tempNewV.getID() << ": (" 
                         << tempNewV.getCoord().x() << ", " << tempNewV.getCoord().y() << ")" << std::endl;
                std::cout << "VVOverlap(1) happens" << std::endl;
                return true;
            }
        }

        // 2.1. V-E检查：新顶点与附近的边
        std::cout << "Checking overlap 2.1 (optimized)..." << std::endl;
        std::vector<int> nearbyEdgeIDs = spatialGrid->getNearbyEdges(newPos, 1);
        
        for (int nearbyEdgeID : nearbyEdgeIDs) {
            if (outEdgeIDs.find(nearbyEdgeID) != outEdgeIDs.end()) {
                continue; // 跳过出边
            }
            
            BaseUGraphProperty::edge_descriptor nearbyED = getEdgeDescriptor(nearbyEdgeID);
            const BaseEdgeProperty& nearbyEdge = graph[nearbyED];
            
            if (VEOverlap(tempNewV, nearbyEdge)) {
                std::cout << std::endl;
                std::cout << "Current vertex: " << tempNewV.getCoord().x() << " " 
                         << tempNewV.getCoord().y() << std::endl;
                std::cout << "Checked edge source " << nearbyEdge.Source().getID() << ": (" 
                         << nearbyEdge.Source().getCoord().x() << ", " 
                         << nearbyEdge.Source().getCoord().y() << ")" << std::endl;
                std::cout << "Checked edge target " << nearbyEdge.Target().getID() << ": (" 
                         << nearbyEdge.Target().getCoord().x() << ", " 
                         << nearbyEdge.Target().getCoord().y() << ")" << std::endl;
                std::cout << "VEOverlap(2.1) happens" << std::endl;
                return true;
            }
        }

        // 2.2. V-E检查：附近的顶点与新的出边
        std::cout << "Checking overlap 2.2 (optimized)..." << std::endl;
        for (const auto& newOutEdge : newOutEdges) {
            // 获取这条边覆盖路径上的所有顶点
            std::vector<int> verticesAlongEdge = spatialGrid->getVerticesAlongLine(
                newOutEdge.Source().getCoord(), 
                newOutEdge.Target().getCoord()
            );
            
            for (int vertexAlongID : verticesAlongEdge) {
                if (vertexAlongID == vertexID || 
                    outVertexIDs.find(vertexAlongID) != outVertexIDs.end()) {
                    continue; // 跳过当前顶点和出边相连的顶点
                }
                
                BaseUGraphProperty::vertex_descriptor vAlongD = getVertexDescriptor(vertexAlongID);
                const BaseVertexProperty& vertexAlong = graph[vAlongD];
                
                if (VEOverlap(vertexAlong, newOutEdge)) {
                    std::cout << std::endl;
                    std::cout << "Checked vertex " << vertexAlong.getID() << ": (" 
                             << vertexAlong.getCoord().x() << ", " << vertexAlong.getCoord().y() << ")" << std::endl;
                    std::cout << "Current edge source " << newOutEdge.Source().getID() << ": (" 
                             << newOutEdge.Source().getCoord().x() << ", " 
                             << newOutEdge.Source().getCoord().y() << ")" << std::endl;
                    std::cout << "Current edge target " << newOutEdge.Target().getID() << ": (" 
                             << newOutEdge.Target().getCoord().x() << ", " 
                             << newOutEdge.Target().getCoord().y() << ")" << std::endl;
                    std::cout << "VEOverlap(2.2) happens" << std::endl;
                    return true;
                }
            }
        }

        // 3.1. E-E检查：新的出边与附近的其他边
        std::cout << "Checking overlap 3.1 (optimized)..." << std::endl;
        for (const BaseEdgeProperty& newOutEdge : newOutEdges) {
            // 获取这条边路径上的所有边
            std::vector<int> edgesAlongLine = spatialGrid->getEdgesAlongLine(
                newOutEdge.Source().getCoord(),
                newOutEdge.Target().getCoord()
            );
            
            for (int edgeAlongID : edgesAlongLine) {
                if (outEdgeIDs.find(edgeAlongID) != outEdgeIDs.end()) {
                    continue; // 跳过出边
                }
                
                BaseUGraphProperty::edge_descriptor eAlongD = getEdgeDescriptor(edgeAlongID);
                const BaseEdgeProperty& edgeAlong = graph[eAlongD];
                
                if (EEOverlap(edgeAlong, newOutEdge)) {
                    std::cout << std::endl;
                    std::cout << "Checked edge source " << edgeAlong.Source().getID() << ": (" 
                             << edgeAlong.Source().getCoord().x() << ", " 
                             << edgeAlong.Source().getCoord().y() << ")" << std::endl;
                    std::cout << "Checked edge target " << edgeAlong.Target().getID() << ": (" 
                             << edgeAlong.Target().getCoord().x() << ", " 
                             << edgeAlong.Target().getCoord().y() << ")" << std::endl;
                    std::cout << "Current edge source " << newOutEdge.Source().getID() << ": (" 
                             << newOutEdge.Source().getCoord().x() << ", " 
                             << newOutEdge.Source().getCoord().y() << ")" << std::endl;
                    std::cout << "Current edge target " << newOutEdge.Target().getID() << ": (" 
                             << newOutEdge.Target().getCoord().x() << ", " 
                             << newOutEdge.Target().getCoord().y() << ")" << std::endl;
                    std::cout << "EEOverlap(3.1) happens" << std::endl;
                    return true;
                }
            }
        }

        // 3.2. E-E检查：新的出边之间的重叠
        std::cout << "Checking overlap 3.2 (optimized)..." << std::endl;
        for (size_t i = 0; i < newOutEdges.size(); ++i) {
            for (size_t j = i + 1; j < newOutEdges.size(); ++j) {
                if (EEOverlap(newOutEdges[i], newOutEdges[j])) {
                    std::cout << std::endl;
                    std::cout << newOutEdges[i].Source().getID() << ": (" 
                             << newOutEdges[i].Source().getCoord().x() << ", " 
                             << newOutEdges[i].Source().getCoord().y() << ")" << std::endl;
                    std::cout << newOutEdges[i].Target().getID() << ": (" 
                             << newOutEdges[i].Target().getCoord().x() << ", " 
                             << newOutEdges[i].Target().getCoord().y() << ")" << std::endl;
                    std::cout << newOutEdges[j].Source().getID() << ": (" 
                             << newOutEdges[j].Source().getCoord().x() << ", " 
                             << newOutEdges[j].Source().getCoord().y() << ")" << std::endl;
                    std::cout << newOutEdges[j].Target().getID() << ": (" 
                             << newOutEdges[j].Target().getCoord().x() << ", " 
                             << newOutEdges[j].Target().getCoord().y() << ")" << std::endl;
                    std::cout << "EEOverlap(3.2) happens" << std::endl;
                    return true;
                }
            }
        }
        
        return false;
    }
}