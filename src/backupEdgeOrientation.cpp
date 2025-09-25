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
    const double TOLERANCE_EPSILON = 0;                                                         // Tolerance parameter ε
    const double ANGLE_THRESHOLD_DEG = 22.5;                                                    // Angle threshold δ = 22.5°
    const double TAN_THRESHOLD = std::tan(ANGLE_THRESHOLD_DEG * M_PI / 180.0);                
    const double LAMBDA_H = 1.0;                                                                // Weight for horizontal edges
    const double LAMBDA_V = 1.0;                                                                // Weight for vertical edges
    const double BIG_M = 1000.0;                                                                // Big M parameter
    const double EDGE_LENGTH_THRESHOLD = 23.15;                                                 // Edge length threshold

    int optimizeEdgeOrientation(std::vector<BaseVertexProperty>& vertexList, std::vector<BaseEdgeProperty>& edgeList, BaseUGraphProperty& graph) {
        try {
            std::cout << "=== Starting Edge Orientation Optimization ===" << std::endl;
        
            int vertexNum = vertexList.size();
            int edgeNum = edgeList.size();
            
            // Create vertex ID to index mapping
            std::map<unsigned int, int> vertexId2Index = createVertexID2Index(vertexList);
        
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
            // Add constraints
            // ---------------------------------------------------------------------------------------------------------
            
            

            for (int e = 0; e < edgeNum; ++e) {
                const BaseEdgeProperty& edge = edgeList[e];
                
                // Find vertex indices
                auto sourceIt = vertexId2Index.find(edge.Source().getID());
                auto targetIt = vertexId2Index.find(edge.Target().getID());
                
                if (sourceIt == vertexId2Index.end() || targetIt == vertexId2Index.end()) {
                    std::cerr << "Error: vertex not found for edge " << edge.Source().getID() 
                            << " - " << edge.Target().getID() << std::endl;
                    continue;
                }
    
                int i = sourceIt->second;
                int j = targetIt->second;

                bool v = edge.Close2V();
                bool h = edge.Close2H();

                model.addConstr(X[i] - X[j] <= TOLERANCE_EPSILON + BIG_M * (1 - v), "enforce_v_1_" + std::to_string(e));
                model.addConstr(X[j] - X[i] <= TOLERANCE_EPSILON + BIG_M * (1 - v), "enforce_v_2_" + std::to_string(e));
                model.addConstr(Y[i] - Y[j] <= TOLERANCE_EPSILON + BIG_M * (1 - h), "enforce_h_1_" + std::to_string(e));
                model.addConstr(Y[j] - Y[i] <= TOLERANCE_EPSILON + BIG_M * (1 - h), "enforce_h_2_" + std::to_string(e));
            }
            
            createVisualization(vertexList, edgeList, "before_2.svg");

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
                    auto it = vertexId2Index.find(vertex.getID());
                    if (it != vertexId2Index.end()) {
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
                // !!! close2XXX not updated
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

                createVisualization(vertexList, edgeList, "after_2.svg");
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