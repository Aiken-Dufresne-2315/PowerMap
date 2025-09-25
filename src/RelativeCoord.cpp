#define _USE_MATH_DEFINES  // Enable M_PI and other math constants
#include "BaseUGraphProperty.h"
#include "boost/graph/iteration_macros.hpp"
#include "gurobi_c++.h"
#include <vector>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <set>

#ifndef M_PI
#define M_PI 3.14159265358979323846  // Fallback definition for M_PI
#endif

#include <boost/config.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/iteration_macros.hpp>
#include "MapFileReader.h" 

using namespace Map;

// Parameter settings
const double lambda_x = 1.0;    // Weight for X-direction relative position preservation
const double lambda_y = 1.0;    // Weight for Y-direction relative position preservation
const double s = 23.15;         // Threshold
const double epsilon = 0.01;    // Small value to handle strict inequality

// Global variables
double x_min, x_max, y_min, y_max;
double Mx, My;

int optimizeRelativeCoord() {
    try {
        std::cout << "=== Starting Relative Position Optimization ===" << std::endl;
        
        BaseUGraphProperty graph;
        std::vector<BaseVertexProperty> vertexList;
        std::vector<BaseEdgeProperty> edgeList;
        
        // Read map data
        if (!readMapFile("local_map.txt", vertexList, edgeList)) {
            std::cerr << "Unable to read map file!" << std::endl;
            return -1;
        }
        
        const int vertex_num = vertexList.size();
        std::cout << "Successfully read " << vertexList.size() << " vertices" << std::endl;
        
        // Calculate coordinate boundaries
        x_min = x_max = vertexList[0].getCoord().x();
        y_min = y_max = vertexList[0].getCoord().y();
        
        for(int i = 0; i < vertex_num; ++i) {
            double x = vertexList[i].getCoord().x();
            double y = vertexList[i].getCoord().y();
            x_min = std::min(x_min, x);
            x_max = std::max(x_max, x);
            y_min = std::min(y_min, y);
            y_max = std::max(y_max, y);
        }
        
        // Extend boundaries
        Mx = x_max - x_min + s;
        My = y_max - y_min + s;
        
        std::cout << "Coordinate range: X[" << x_min << ", " << x_max << "], Y[" << y_min << ", " << y_max << "]" << std::endl;
        
        // Relative position relationship matrix
        std::vector<std::vector<int>> sx(vertex_num, std::vector<int>(vertex_num));
        std::vector<std::vector<int>> sy(vertex_num, std::vector<int>(vertex_num));

        // Calculate relative position relationship matrix
        for(int i = 0; i < vertex_num; ++i) {
            for(int j = 0; j < vertex_num && j != i; ++j) {  // Fixed loop variable error
                sx[i][j] = (vertexList[i].getCoord().x() <= vertexList[j].getCoord().x()) ? 1 : -1;
                sy[i][j] = (vertexList[i].getCoord().y() <= vertexList[j].getCoord().y()) ? 1 : -1;
            }
        }
        
        // Create Gurobi environment and model
        GRBEnv env(true);
        env.set("LogFile", "relative_coord_opt.log");
        env.start();
        GRBModel model(env);

        // Create optimization variables - new coordinates
        std::vector<GRBVar> X(vertex_num), Y(vertex_num);
        for(int i = 0; i < vertex_num; ++i) {
            X[i] = model.addVar(x_min, x_max, 0.0, GRB_CONTINUOUS, "X_" + std::to_string(i));
            Y[i] = model.addVar(y_min, y_max, 0.0, GRB_CONTINUOUS, "Y_" + std::to_string(i));
            
            // Set initial values
            X[i].set(GRB_DoubleAttr_Start, vertexList[i].getCoord().x());
            Y[i].set(GRB_DoubleAttr_Start, vertexList[i].getCoord().y());
        }
        
        // Create binary variables for relative position constraints
        std::vector<std::vector<GRBVar>> z_x(vertex_num, std::vector<GRBVar>(vertex_num));
        std::vector<std::vector<GRBVar>> z_y(vertex_num, std::vector<GRBVar>(vertex_num));
        
        // Big M value for linearization
        double M = Mx + My + 1000;
        
        for(int i = 0; i < vertex_num; ++i) {
            for(int j = 0; j < vertex_num && j != i; ++j) {
                z_x[i][j] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY, 
                    "z_x_" + std::to_string(i) + "_" + std::to_string(j));
                z_y[i][j] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY, 
                    "z_y_" + std::to_string(i) + "_" + std::to_string(j));
            }
        }
        
        // Add logical constraints: z^x_ij = 1 if X_i <= X_j, z^y_ij = 1 if Y_i <= Y_j
        // This is NOT a constraint, but the DEFINITION of z variables!
        std::cout << "Adding logical constraints for z variables..." << std::endl;
        
        for(int i = 0; i < vertex_num; ++i) {
            for(int j = 0; j < vertex_num && j != i; ++j) {
                // Constraint 1: If z^x_ij = 1, then X_i <= X_j
                // X_i - X_j <= M * (1 - z^x_ij)
                model.addConstr(X[i] - X[j] <= M * (1 - z_x[i][j]),
                    "logic_x1_" + std::to_string(i) + "_" + std::to_string(j));
                
                // Constraint 2: If z^x_ij = 0, then X_i > X_j (i.e., X_j - X_i < 0)
                // X_j - X_i <= M * z^x_ij - epsilon
                model.addConstr(X[j] - X[i] <= M * z_x[i][j] - epsilon,
                    "logic_x2_" + std::to_string(i) + "_" + std::to_string(j));
                
                // Same constraints for Y coordinates
                // Constraint 3: If z^y_ij = 1, then Y_i <= Y_j
                model.addConstr(Y[i] - Y[j] <= M * (1 - z_y[i][j]),
                    "logic_y1_" + std::to_string(i) + "_" + std::to_string(j));
                
                // Constraint 4: If z^y_ij = 0, then Y_i > Y_j
                model.addConstr(Y[j] - Y[i] <= M * z_y[i][j] - epsilon,
                    "logic_y2_" + std::to_string(i) + "_" + std::to_string(j));
            }
        }
        
        // Add relative position preservation constraints (MIP formulation)
        std::cout << "Adding MIP relative position constraints..." << std::endl;
        
        for(int i = 0; i < vertex_num; ++i) {
            for(int j = 0; j < vertex_num && j != i; ++j) {
                model.addConstr(sx[i][j] * (X[j] - X[i]) >= s - M * (1 - z_x[i][j]),
                    "rel_x_" + std::to_string(i) + "_" + std::to_string(j));
                model.addConstr(sy[i][j] * (Y[j] - Y[i]) >= s - M * (1 - z_y[i][j]),
                    "rel_y_" + std::to_string(i) + "_" + std::to_string(j));
            }
        }
        
        // Construct neighborhood relationship for computational efficiency
        std::cout << "Constructing neighborhood relationships..." << std::endl;
        const int K = 6;  // Number of nearest neighbors per vertex
        std::vector<std::vector<int>> neighbors(vertex_num);
        
        // Calculate all pairwise distances and find k-nearest neighbors
        for(int i = 0; i < vertex_num; ++i) {
            std::vector<std::pair<double, int>> distances;
            
            for(int j = 0; j < vertex_num; ++j) {
                if(i != j) {
                    double dx = vertexList[i].getCoord().x() - vertexList[j].getCoord().x();
                    double dy = vertexList[i].getCoord().y() - vertexList[j].getCoord().y();
                    double dist = std::sqrt(pow(dx, 2) + pow(dy, 2));
                    distances.push_back({dist, j});
                }
            }
            
            // Sort by distance and take K nearest
            std::sort(distances.begin(), distances.end());
            int actual_k = std::min(K, (int)distances.size());
            
            for(int k = 0; k < actual_k; ++k) {
                neighbors[i].push_back(distances[k].second);
            }
            
            std::cout << "Vertex " << i << " has " << neighbors[i].size() << " neighbors" << std::endl;
        }
        
        // Create neighborhood pair set P (undirected, avoid duplicates)
        std::set<std::pair<int, int>> neighborhood_pairs;
        for(int i = 0; i < vertex_num; ++i) {
            for(int neighbor : neighbors[i]) {
                int j = neighbor;
                // Add both directions but avoid duplicates by ensuring i < j
                if(i < j) {
                    neighborhood_pairs.insert({i, j});
                } else {
                    neighborhood_pairs.insert({j, i});
                }
            }
        }
        
        std::cout << "Total neighborhood pairs: " << neighborhood_pairs.size() << " (vs " << (vertex_num*(vertex_num-1)/2) << " total pairs)" << std::endl;
        
        // Add constraint: z^x_ij + z^y_ij >= 1 (at least preserve one direction)
        // Apply only to neighborhood pairs P
        std::cout << "Adding at-least-one constraint for neighborhood pairs..." << std::endl;
        for(const auto& pair : neighborhood_pairs) {
            int i = pair.first;
            int j = pair.second;
            model.addConstr(z_x[i][j] + z_y[i][j] >= 1,
                "at_least_one_" + std::to_string(i) + "_" + std::to_string(j));
        }
        
        // Set objective function: minimize violations only for neighborhood pairs
        // Objective: λ_x * ∑(1 - z^x_ij) + λ_y * ∑(1 - z^y_ij) for (i,j) ∈ P
        // Note: Only consider one direction per pair to avoid redundancy
        GRBLinExpr objective = 0;
        
        for(const auto& pair : neighborhood_pairs) {
            int i = pair.first;
            int j = pair.second;
            
            // Only add one direction since z^x_ij and z^x_ji are complementary
            // We choose the canonical direction i < j (already ensured in pair construction)
            objective += lambda_x * (1 - z_x[i][j]);
            objective += lambda_y * (1 - z_y[i][j]);
        }
        
        std::cout << "Objective function includes " << (2 * neighborhood_pairs.size()) << " terms (vs " << (2 * vertex_num * (vertex_num-1)) << " in full version)" << std::endl;
        
        model.setObjective(objective, GRB_MINIMIZE);
        
        // Set MIP solver parameters
        model.set(GRB_DoubleParam_TimeLimit,    300.0);  // 5-minute time limit
        model.set(GRB_IntParam_OutputFlag,      0    );
        model.set(GRB_DoubleParam_MIPGap,       0.01 );  // 1% optimality gap
        model.set(GRB_IntParam_MIPFocus,        1    );  // Focus on finding feasible solutions
        
        // Solve
        std::cout << "Starting optimization..." << std::endl;
        model.optimize();

        // Check solution status
        int status = model.get(GRB_IntAttr_Status);
        if (status == GRB_OPTIMAL || status == GRB_TIME_LIMIT) {
            std::cout << "\n=== Optimization Results ===" << std::endl;
            std::cout << "Objective value: " << model.get(GRB_DoubleAttr_ObjVal) << std::endl;
            
            // Output optimized coordinates
            std::cout << "\nOptimized coordinates:" << std::endl;
            for(int i = 0; i < vertex_num; ++i) {
                double new_x = X[i].get(GRB_DoubleAttr_X);
                double new_y = Y[i].get(GRB_DoubleAttr_X);  // Fix: should get Y coordinate
                double orig_x = vertexList[i].getCoord().x();
                double orig_y = vertexList[i].getCoord().y();
                
                std::cout << "Station " << vertexList[i].getID() << " (" << vertexList[i].getName() << "): "
                         << "(" << orig_x << ", " << orig_y << ") -> "
                         << "(" << new_x << ", " << new_y << ")" << std::endl;
            }
            
            // Analyze MIP solution: count preserved and violated relations (neighborhood pairs only)
            int x_preserved = 0, y_preserved = 0;
            int x_violated = 0, y_violated = 0;
            int total_neighbor_relations = neighborhood_pairs.size();  // One direction per pair
            
            for(const auto& pair : neighborhood_pairs) {
                int i = pair.first;
                int j = pair.second;
                
                // Check only the canonical direction i -> j (i < j)
                if(z_x[i][j].get(GRB_DoubleAttr_X) > 0.5) {
                    x_preserved++;
                } else {
                    x_violated++;
                }
                
                if(z_y[i][j].get(GRB_DoubleAttr_X) > 0.5) {
                    y_preserved++;
                } else {
                    y_violated++;
                }
            }
            
            std::cout << "\nMIP Solution Analysis (Neighborhood pairs only):" << std::endl;
            std::cout << "X-direction: " << x_preserved << " preserved, " << x_violated << " violated (total: " << total_neighbor_relations << ")" << std::endl;
            std::cout << "Y-direction: " << y_preserved << " preserved, " << y_violated << " violated (total: " << total_neighbor_relations << ")" << std::endl;
            std::cout << "Preservation rate: X=" << (100.0 * x_preserved / total_neighbor_relations) << "%, Y=" << (100.0 * y_preserved / total_neighbor_relations) << "%" << std::endl;
            std::cout << "Computational savings: " << neighborhood_pairs.size() << " pairs vs " << (vertex_num*(vertex_num-1)/2) << " total pairs (" 
                     << (100.0 * neighborhood_pairs.size() / (vertex_num*(vertex_num-1)/2)) << "%)" << std::endl;
            
        } else {
            std::cerr << "Optimization failed, status: " << status << std::endl;
            return -1;
        }
        
        return 0;
        
    } catch (GRBException& e) {
        std::cerr << "Gurobi error " << e.getErrorCode() << ": " << e.getMessage() << std::endl;
        return -1;
    } catch (std::exception& e) {
        std::cerr << "Standard error: " << e.what() << std::endl;
        return -1;
    }
}

// Test function
int testRelativeCoordOptimization() {
    std::cout << "=== Testing Relative Position Optimization Function ===" << std::endl;
    return optimizeRelativeCoord();
}