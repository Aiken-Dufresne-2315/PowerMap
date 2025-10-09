#include "VisualizeSVG.h"
#define _USE_MATH_DEFINES  // Enable M_PI and other math constants
#include "BaseUGraphProperty.h"
#include "gurobi_c++.h"
#include <vector>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <map>

#ifndef M_PI
#define M_PI 3.14159265358979323846  // Fallback definition for M_PI
#endif

#include <boost/config.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/iteration_macros.hpp>
#include "MapFileReader.h"
#include "EdgeOrientation.h"

namespace Map {

    // Parameter settings for edge orientation optimization
    const double TOLERANCE_EPSILON = 0;                                             // Tolerance parameter ε
    const double ANGLE_THRESHOLD_DEG = 30;                                          // Angle threshold δ = 22.5°
    const double TAN_THRESHOLD = std::tan(ANGLE_THRESHOLD_DEG * M_PI / 180.0);                
    const double LAMBDA_H = 1.0;                                                    // Weight for horizontal edges
    const double LAMBDA_V = 1.0;                                                    // Weight for vertical edges
    const double BIG_M = 1000.0;                                                    // Big M parameter
    const double EDGE_LENGTH_THRESHOLD = 23.15;                                     // Edge length threshold

    int optimizeEdgeOrientation(
        std::vector<BaseVertexProperty>& vertexList, 
        std::vector<BaseEdgeProperty>& edgeList, 
        BaseUGraphProperty& graph,
        const std::string& testCaseName) {
        try {
            std::cout << "=== Starting Edge Orientation Optimization ===" << std::endl;
        
            int vertexNum = vertexList.size();
            int edgeNum = edgeList.size();
            
            // Create vertex ID to index mapping
            std::map<unsigned int, int> vertexID2Index = createVertexID2Index(vertexList);
        
            // Calculate coordinate boundaries
            double x_min = vertexList[0].getCoord().x();
            double x_max = vertexList[0].getCoord().x();
            double y_min = vertexList[0].getCoord().y();
            double y_max = vertexList[0].getCoord().y();
        
            for (const auto& vertex : vertexList) {
                double x = vertex.getCoord().x();
                double y = vertex.getCoord().y();
                x_min = std::min(x_min, x);
                x_max = std::max(x_max, x);
                y_min = std::min(y_min, y);
                y_max = std::max(y_max, y);
            }
        
            std::cout << "Coordinate range: X[" << x_min << ", " << x_max << "], Y[" << y_min << ", " << y_max << "]" << std::endl;
        
            // Create Gurobi environment and model
            GRBEnv env(true);
            env.set("LogFile", "edge_orientation_opt.log");
            env.start();
            GRBModel model(env);
        
            // ---------------------------------------------------------------------------------------------------------
            // Create decision variables
            // ---------------------------------------------------------------------------------------------------------
            std::vector<GRBVar> X(vertexNum), Y(vertexNum);
            for (int i = 0; i < vertexNum; ++i) {
                X[i] = model.addVar(x_min, x_max, 0.0, GRB_CONTINUOUS, "X_" + std::to_string(i));
                Y[i] = model.addVar(y_min, y_max, 0.0, GRB_CONTINUOUS, "Y_" + std::to_string(i));
                
                // Set initial values to original coordinates
                X[i].set(GRB_DoubleAttr_Start, vertexList[i].getCoord().x());
                Y[i].set(GRB_DoubleAttr_Start, vertexList[i].getCoord().y());
            }
            
            // ---------------------------------------------------------------------------------------------------------
            // 2025.09.15 Handling Edge Overlapping.
            // ---------------------------------------------------------------------------------------------------------
            
            std::cout << "=== Anti-overlap Processing ===" << std::endl;

            std::vector<std::pair<int, size_t>> degVertIdxPairs;
            // !!! vertex iterator
            std::pair<BaseUGraphProperty::vertex_iterator, BaseUGraphProperty::vertex_iterator> vp = vertices(graph);

            int vertexIndex = 0;
            for (BaseUGraphProperty::vertex_iterator vi = vp.first; vi != vp.second; ++vi, ++vertexIndex) {
                size_t out_degree = boost::out_degree(*vi, graph);
                degVertIdxPairs.push_back({out_degree, vertexIndex});
            }

            std::sort(
                degVertIdxPairs.begin(),
                degVertIdxPairs.end(),
                [](const auto& a, const auto& b) { return a.first > b.first; } );

            std::cout << "Vertex processing order (by out-degree):" << std::endl;
            for (const auto& pair : degVertIdxPairs) {
                std::cout << "Vertex " << vertexList[pair.second].getID() << " (degree: " << pair.first << ")" << std::endl;
            }

            // Initialize edge orientation states to -1 (unprocessed)
            // -1: unprocessed, 0: explicitly not aligned, 1: aligned
            std::vector<int> edgeOriented2V(edgeNum, -1);
            std::vector<int> edgeOriented2H(edgeNum, -1);

            const double ANGLE_CENTERS[2] = {M_PI / 2, 0.0};  // Vertical (UP/DOWN), Horizontal (LEFT/RIGHT)
            const std::string AXIS_NAMES[2] = {"VERTICAL", "HORIZONTAL"};

            // angle normalization function -> [0, 2*PI)
            auto normalizeAngle = [](double angle) -> double {
                while(angle < 0) {
                    angle += 2 * M_PI;
                }
                while(angle >= 2 * M_PI) {
                    angle -= 2 * M_PI;
                }
                return angle;
            };

            // Calculate minimum offset to an axis (vertical or horizontal)
            auto calculateAxisOffset = [&](double edge_angle, int axis) -> double {
                double norm_angle = normalizeAngle(edge_angle);
                if (axis == 0) {  // Vertical: UP (90°) or DOWN (270°)
                    double offsetUp = std::abs(norm_angle - M_PI / 2);
                    double offsetDown = std::abs(norm_angle - 3 * M_PI / 2);
                    return std::min(offsetUp, offsetDown);
                } else {  // Horizontal: RIGHT (0°) or LEFT (180°)
                    double offsetRight = std::min(std::abs(norm_angle), std::abs(norm_angle - 2 * M_PI));
                    double offsetLeft = std::abs(norm_angle - M_PI);
                    return std::min(offsetRight, offsetLeft);
                }
            };
            
            // Check if edge is within axis neighborhood
            auto inAxisNeighborhood = [&](double edge_angle, int axis) -> bool {
                double offset = calculateAxisOffset(edge_angle, axis);
                double threshold_rad = ANGLE_THRESHOLD_DEG * M_PI / 180.0;
                return offset <= threshold_rad;
            };
            
            // Get the other vertex of an edge given current vertex
            auto theOtherVertexDesc = [&](BaseUGraphProperty::edge_descriptor edge_desc, BaseUGraphProperty::vertex_descriptor current) -> BaseUGraphProperty::vertex_descriptor {
                BaseUGraphProperty::vertex_descriptor source = boost::source(edge_desc, graph);
                BaseUGraphProperty::vertex_descriptor target = boost::target(edge_desc, graph);
                return (source == current) ? target : source;
            };

            // !!! Main loop: for each vertex (sorted by out-degree)
            for (const auto& pair : degVertIdxPairs) {
                size_t currentVertexIndex = pair.second;
                
                // Get corresponding vertex descriptor
                BaseUGraphProperty::vertex_iterator vi = vp.first;
                std::advance(vi, currentVertexIndex);
                BaseUGraphProperty::vertex_descriptor current_vertex = *vi;
                unsigned int currentVertexID = graph[current_vertex].getID();
                
                std::cout << "\n=== Processing vertex " << currentVertexIndex << " (ID: " << currentVertexID << ") ===" << std::endl;
                
                // Process both axes: 0=Vertical, 1=Horizontal
                for (int axis = 0; axis < 2; ++axis) {
                    std::vector<int>& edgeOrientedMarks = (axis == 0) ? edgeOriented2V : edgeOriented2H;
                    bool flag = false;  // Flag to track if other vertex already has aligned edge
                    
                    std::cout << "\n  Processing " << AXIS_NAMES[axis] << " axis:" << std::endl;
                    
                    // First pass: mark edges based on conflict detection
                    std::pair<BaseUGraphProperty::out_edge_iterator, BaseUGraphProperty::out_edge_iterator> oep = boost::out_edges(current_vertex, graph);
                    
                    // !!! Traverse out-edges of the current vertex
                    for (BaseUGraphProperty::out_edge_iterator eit = oep.first; eit != oep.second; ++eit) {
                        int edgeIndex = graph[*eit].ID();
                        
                        // Skip if already processed
                        if (edgeOrientedMarks[edgeIndex] != -1) {
                            std::cout << "    Edge " << edgeIndex << " already processed (state=" << edgeOrientedMarks[edgeIndex] << "), skip" << std::endl;
                            continue;
                        }
                        
                        // edgeID is equivalent to edgeIndex
                        const BaseEdgeProperty& edge = edgeList[edgeIndex];
                        double edge_angle = edge.Angle();
                        
                        // Check if edge is within axis neighborhood
                        if (inAxisNeighborhood(edge_angle, axis)) {
                            std::cout << "    Edge " << edgeIndex << " is in " << AXIS_NAMES[axis] << " neighborhood (angle=" << edge_angle * 180.0 / M_PI << "°)" << std::endl;
                            
                            // Get the other vertex of this edge
                            BaseUGraphProperty::vertex_descriptor oVertex = theOtherVertexDesc(*eit, current_vertex);
                            
                            // Check if other vertex already has an aligned edge in this axis
                            flag = false;
                            std::pair<BaseUGraphProperty::out_edge_iterator, BaseUGraphProperty::out_edge_iterator> ooep = boost::out_edges(oVertex, graph);
                            
                            for (BaseUGraphProperty::out_edge_iterator oeit = ooep.first; oeit != ooep.second; ++oeit) {
                                int otherEdgeIndex = graph[*oeit].ID();
                                if (edgeOrientedMarks[otherEdgeIndex] == 1) {
                                    flag = true;
                                    std::cout << "      Conflict detected! Other vertex has aligned edge " << otherEdgeIndex << std::endl;
                                    break;
                                }
                            }
                            
                            // Mark the edge based on conflict status
                            if (!flag) {
                                edgeOrientedMarks[edgeIndex] = 1;
                                std::cout << "      Set edge " << edgeIndex << " " << AXIS_NAMES[axis] << " = 1 (aligned)" << std::endl;
                                flag = false;  // Reset for next edge
                            } 
                            else {
                                edgeOrientedMarks[edgeIndex] = 0;
                                std::cout << "      Set edge " << edgeIndex << " " << AXIS_NAMES[axis] << " = 0 (conflict)" << std::endl;
                            }
                        } 
                        else {
                            // Not in neighborhood
                            edgeOrientedMarks[edgeIndex] = 0;
                            std::cout << "    Edge " << edgeIndex << " not in " << AXIS_NAMES[axis] << " neighborhood, set to 0" << std::endl;
                        }
                    }
                    
                    // Second pass: keep only the closest edge among all aligned edges (oriented=1)
                    std::vector<int> candEdges;
                    oep = boost::out_edges(current_vertex, graph);
                    
                    for (BaseUGraphProperty::out_edge_iterator eit = oep.first; eit != oep.second; ++eit) {
                        int edgeIndex = graph[*eit].ID();
                        if (edgeOrientedMarks[edgeIndex] == 1) {
                            candEdges.push_back(edgeIndex);
                        }
                    }
                    
                    if (candEdges.size() > 1) {
                        std::cout << "  Multiple aligned edges detected (" << candEdges.size() << "), selecting closest to " << AXIS_NAMES[axis] << " axis" << std::endl;
                        
                        // Find the edge closest to the axis
                        int bestEdge = candEdges[0];
                        double minOffset = calculateAxisOffset(edgeList[bestEdge].Angle(), axis);
                        
                        for (int edgeIndex : candEdges) {
                            double offset = calculateAxisOffset(edgeList[edgeIndex].Angle(), axis);
                            std::cout << "    Edge " << edgeIndex << " offset: " << offset * 180.0 / M_PI << "°" << std::endl;
                            if (offset < minOffset) {
                                minOffset = offset;
                                bestEdge = edgeIndex;
                            }
                        }
                        
                        // Set all other edges back to 0
                        for (int edgeIndex : candEdges) {
                            if (edgeIndex != bestEdge) {
                                edgeOrientedMarks[edgeIndex] = 0;
                                std::cout << "    Unmark edge " << edgeIndex << " (not the closest)" << std::endl;
                            }
                        }
                        
                        std::cout << "  Final choice: Edge " << bestEdge << " (offset: " << minOffset * 180.0 / M_PI << "°)" << std::endl;
                    }
                }
            }

            std::cout << "\n=== Anti-overlap Processing Completed ===" << std::endl;
            
            // Apply final orientation states to edgeList
            std::cout << "\n=== Final Edge Orientations ===" << std::endl;
            for (int e = 0; e < edgeNum; ++e) {
                if (edgeOriented2V[e] == 1) {
                    edgeList[e].setOriented2V(true);
                    std::cout << "Edge " << e << ": Oriented2V = true" << std::endl;
                } else {
                    edgeList[e].setOriented2V(false);
                }
                
                if (edgeOriented2H[e] == 1) {
                    edgeList[e].setOriented2H(true);
                    std::cout << "Edge " << e << ": Oriented2H = true" << std::endl;
                } else {
                    edgeList[e].setOriented2H(false);
                }
            }

            // ---------------------------------------------------------------------------------------------------------
            // Add constraints
            // ---------------------------------------------------------------------------------------------------------

            for (int e = 0; e < edgeNum; ++e) {
                const BaseEdgeProperty& edge = edgeList[e];
                
                // Find vertex indices
                auto sourceIt = vertexID2Index.find(edge.Source().getID());
                auto targetIt = vertexID2Index.find(edge.Target().getID());
                
                if (sourceIt == vertexID2Index.end() || targetIt == vertexID2Index.end()) {
                    std::cerr << "Error: vertex not found for edge " << edge.Source().getID() 
                            << " - " << edge.Target().getID() << std::endl;
                    continue;
                }
    
                int i = sourceIt->second;
                int j = targetIt->second;

                bool v = edge.Oriented2V();
                bool h = edge.Oriented2H();

                model.addConstr(X[i] - X[j] <= TOLERANCE_EPSILON + BIG_M * (1 - v), "enforce_v_1_" + std::to_string(e));
                model.addConstr(X[j] - X[i] <= TOLERANCE_EPSILON + BIG_M * (1 - v), "enforce_v_2_" + std::to_string(e));
                model.addConstr(Y[i] - Y[j] <= TOLERANCE_EPSILON + BIG_M * (1 - h), "enforce_h_1_" + std::to_string(e));
                model.addConstr(Y[j] - Y[i] <= TOLERANCE_EPSILON + BIG_M * (1 - h), "enforce_h_2_" + std::to_string(e));
            }
            
            GRBQuadExpr objective = 0;
            for (int i = 0; i < vertexNum; ++i) {
                objective += (X[i] - vertexList[i].getCoord().x()) * (X[i] - vertexList[i].getCoord().x()) + 
                            (Y[i] - vertexList[i].getCoord().y()) * (Y[i] - vertexList[i].getCoord().y());
            }
            
            model.setObjective(objective, GRB_MINIMIZE);
            
            // Solve the optimization problem
            std::cout << "Solving optimization problem..." << std::endl;
            model.optimize();
            
            // Check optimization status
            int status = model.get(GRB_IntAttr_Status);
            if (status == GRB_OPTIMAL) {
                std::cout << "\n=== Optimization completed successfully! ===" << std::endl;
                std::cout << "Optimal objective value: " << model.get(GRB_DoubleAttr_ObjVal) << std::endl;
                
                // Print results and update coordinates
                std::cout << "\n=== Optimized coordinates ===" << std::endl;
                for (int i = 0; i < vertexNum; ++i) {
                    double newX = X[i].get(GRB_DoubleAttr_X);
                    double newY = Y[i].get(GRB_DoubleAttr_X);
                    std::cout << "Vertex " << vertexList[i].getID() << " (" << vertexList[i].getName() << "): (" << newX << ", " << newY << ")" << std::endl;
                    
                    // !!! Update vertices in graph  
                    vertexList[i].setCoord(newX, newY);
                    std::cout << "Vertex " << vertexList[i].getID() <<  "(" << newX << ", " << newY << ")" << std::endl;
                }

                // Update graph vertices
                std::pair<BaseUGraphProperty::vertex_iterator, BaseUGraphProperty::vertex_iterator> vp = vertices(graph);
                for (BaseUGraphProperty::vertex_iterator vi = vp.first; vi != vp.second; ++vi) {
                    // !!! reference type
                    BaseVertexProperty& vertex = graph[*vi];
                    auto it = vertexID2Index.find(vertex.getID());
                    if (it != vertexID2Index.end()) {
                        // !!! index in vertexList
                        int idx = it->second;
                        double newX = X[idx].get(GRB_DoubleAttr_X);
                        double newY = Y[idx].get(GRB_DoubleAttr_X);

                        std::cout << "Graph vertex " << vertex.getID() << " to (" << vertex.getCoord().x() << ", " << vertex.getCoord().y() << ")" << std::endl;
                        vertex.setCoord(newX, newY);
                        std::cout << "Updated graph vertex " << vertex.getID() << " to (" << vertex.getCoord().x() << ", " << vertex.getCoord().y() << ")" << std::endl;
                    }
                }
                std::cout << edgeList[0].Source().getCoord().x() << " " << edgeList[0].Source().getCoord().y() << " " << edgeList[0].Angle() << std::endl;

                // Update (angles and xxx of) graph edges and edgeList
                std::pair<BaseUGraphProperty::edge_iterator, BaseUGraphProperty::edge_iterator> ep = edges(graph);
                for (BaseUGraphProperty::edge_iterator ei = ep.first; ei != ep.second; ++ei) {
                    BaseEdgeProperty& edge = graph[*ei];
                    std::cout << edge.ID() << " " << edge.Source().getID() << " " << edge.Target().getID() << std::endl;

                    BaseUGraphProperty::vertex_descriptor source_desc = boost::source(*ei, graph);
                    BaseUGraphProperty::vertex_descriptor target_desc = boost::target(*ei, graph);
                    
                    // Recalculate angle with updated coordinates
                    double oldAngle = edge.Angle();
                    double newAngle = calculateAngle(graph[source_desc], graph[target_desc]);
                    
                    // Replace the edge in the graph
                    edge.setAngle(newAngle);
                    
                    std::cout << "  Angle: " << oldAngle << " -> " << newAngle << std::endl;
                    std::cout << "  Edge updated successfully!" << std::endl << std::endl;

                    edgeList[edge.ID()].setAngle(newAngle);
                }

                std::string outputFile = "output/" + testCaseName + "_2.svg";
                createVisualization(vertexList, edgeList, outputFile);
                // Update edges in edgeList with new vertex coordinates

                // !!! not updated completely
                std::cout << edgeList[0].Source().getCoord().x() << " " << edgeList[0].Source().getCoord().y() << " " << edgeList[0].Angle() << std::endl;

            } else if (status == GRB_INFEASIBLE) {
                std::cerr << "Model is infeasible!" << std::endl;
                return -1;
            } else if (status == GRB_UNBOUNDED) {
                std::cerr << "Model is unbounded!" << std::endl;
                return -1;
            } else {
                std::cerr << "Optimization ended with status " << status << std::endl;
                return -1;
            }
            
        } catch (GRBException e) {
            std::cerr << "Gurobi error code: " << e.getErrorCode() << std::endl;
            std::cerr << e.getMessage() << std::endl;
            return -1;
        } catch (...) {
            std::cerr << "Unknown error occurred" << std::endl;
            return -1;
        }
        
        return 0;
    }

} // namespace Map