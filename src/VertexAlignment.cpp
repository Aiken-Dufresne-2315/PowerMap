#include "VertexAlignment.h"
#include "VisualizeSVG.h"
#define _USE_MATH_DEFINES  // Enable M_PI and other math constants
#include "BaseUGraphProperty.h"
#include "gurobi_c++.h"
#include <vector>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <map>
#include <numeric>

#ifndef M_PI
#define M_PI 3.14159265358979323846  // Fallback definition for M_PI
#endif

#include <boost/config.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/iteration_macros.hpp>
#include "MapFileReader.h"

namespace Map {

    // Parameter settings for vertex alignment optimization
    // !!! too many parameters depend on pre-set values !!!
    const double            ALIGNMENT_TOLERANCE = 20.0;                 // Tolerance for considering vertices "close" to a line
    const int               MAX_HORIZONTAL_LINES = -1;                  // -1 means auto-determine
    const int               MAX_VERTICAL_LINES = -1;                    // -1 means auto-determine  
    const double            CLUSTERING_BANDWIDTH = 15.0;                // Bandwidth for density estimation
    const AlignmentMethod   ALIGNMENT_METHOD = CLUSTERING_BASED;        // Current method
    const double            BIG_M = 1000.0;                             // Big M parameter for constraints
    const double            MIN_CLUSTER_SIZE = 3;                       // Minimum vertices needed to form a cluster
    const double            DEFAULT_VERTEX_WEIGHT = 1.0;                // Default weight for vertices
    const bool              USE_DEGREE_BASED_WEIGHT = false;            // Enable degree-based weighting (future extension)

    // Helper function to calculate vertex weight (reserved for future extensions)
    double calculateVWeight(const BaseVertexProperty& vertex, const BaseUGraphProperty& graph) {
        if (!USE_DEGREE_BASED_WEIGHT) {
            return DEFAULT_VERTEX_WEIGHT;
        }
        
        // TODO: Implement degree-based weighting in future
        // For now, return default weight
        return DEFAULT_VERTEX_WEIGHT;
    }

    // K-means clustering for 1D coordinates
    std::vector<double> clusterCoordinates1D(const std::vector<double>& coords, int& optimalK) {
        if (coords.size() < MIN_CLUSTER_SIZE) {
            optimalK = 0;
            return {};
        }

        std::vector<double> sortedCoords = coords;
        std::sort(sortedCoords.begin(), sortedCoords.end());
        
        // Remove duplicates
        sortedCoords.erase(std::unique(sortedCoords.begin(), sortedCoords.end()), sortedCoords.end());
        
        if (sortedCoords.size() < MIN_CLUSTER_SIZE) {
            optimalK = 0;
            return {};
        }

        // Auto-determine optimal k using simplified elbow method
        int maxK = std::min(static_cast<int>(sortedCoords.size() / MIN_CLUSTER_SIZE), static_cast<int>(std::sqrt(sortedCoords.size())) + 2);
        
        double bestScore = std::numeric_limits<double>::max();
        int bestK = 1;
        std::vector<double> bestCenters;

        for (int k = 1; k <= maxK; ++k) {
            // Initialize centroids evenly spaced
            std::vector<double> centroids(k);
            for (int i = 0; i < k; ++i) {
                int idx;
                if (k == 1) {
                    idx = sortedCoords.size() / 2;
                } else {
                    // !!! thinking !!!
                    idx = i * (sortedCoords.size() - 1) / (k - 1);
                }
                centroids[i] = sortedCoords[idx];
            }

            // K-means iterations
            bool converged = false;
            for (int iter = 0; iter < 50 && !converged; ++iter) {
                std::vector<std::vector<double>> clusters(k);
                
                // !!! Assign points to nearest centroid
                for (double coord : sortedCoords) {
                    int nearestCluster = 0;
                    double minDist = std::abs(coord - centroids[0]);
                    
                    // !!! k centroids
                    for (int i = 1; i < k; ++i) {
                        double dist = std::abs(coord - centroids[i]);
                        if (dist < minDist) {
                            minDist = dist;
                            nearestCluster = i;
                        }
                    }
                    // the current coord is closet to the nearestCluster
                    clusters[nearestCluster].push_back(coord);
                }

                // !!! Update centroids
                converged = true;
                for (int i = 0; i < k; ++i) {
                    if (!clusters[i].empty()) {
                        double newCentroid = std::accumulate(clusters[i].begin(), clusters[i].end(), 0.0) / clusters[i].size();
                        if (std::abs(newCentroid - centroids[i]) > 1e-6) {
                            converged = false;
                        }
                        centroids[i] = newCentroid;
                    }
                }
            }

            // Calculate within-cluster sum of squares (WCSS)
            double wcss = 0.0;
            for (double coord : sortedCoords) {
                double minDist = std::numeric_limits<double>::max();
                for (double centroid : centroids) {
                    minDist = std::min(minDist, std::abs(coord - centroid));
                }
                wcss += minDist * minDist;
            }

            // Simple elbow method: prefer fewer clusters with reasonable WCSS
            double score = wcss + k * 50.0;  // Penalty for more clusters
            
            if (score < bestScore) {
                bestScore = score;
                bestK = k;
                bestCenters = centroids;
            }
        }

        optimalK = bestK;
        std::sort(bestCenters.begin(), bestCenters.end());
        return bestCenters;
    }

    // Structure for vertex-line candidate assignments
    struct VertexLineCandidate {
        int vertexIdx;
        int lineIdx;
        bool isHorizontal;
        double distance;
        double linePosition;
    };

    // Pre-select vertices for alignment based on closest line within tolerance
    std::vector<VertexLineCandidate> preSelect(
        const std::vector<BaseVertexProperty>& vertexList,
        const std::vector<double>& hLines,
        const std::vector<double>& vLines) {
        
        std::vector<VertexLineCandidate> candidates;
        
        for (int i = 0; i < vertexList.size(); ++i) {
            double minHDist = std::numeric_limits<double>::max();
            double minVDist = std::numeric_limits<double>::max();
            int bestHLine = -1, bestVLine = -1;
            
            // Find closest horizontal line within tolerance
            for (int h = 0; h < hLines.size(); ++h) {
                double dist = std::abs(vertexList[i].getCoord().y() - hLines[h]);
                if (dist < minHDist && dist <= ALIGNMENT_TOLERANCE) {
                    minHDist = dist;
                    bestHLine = h;
                }
            }
            
            // Find closest vertical line within tolerance
            for (int v = 0; v < vLines.size(); ++v) {
                double dist = std::abs(vertexList[i].getCoord().x() - vLines[v]);
                if (dist < minVDist && dist <= ALIGNMENT_TOLERANCE) {
                    minVDist = dist;
                    bestVLine = v;
                }
            }
            
            // Allow both horizontal and vertical alignment if within tolerance
            if (bestHLine != -1) {
                candidates.push_back({i, bestHLine, true, minHDist, hLines[bestHLine]});
            }
            if (bestVLine != -1) {
                candidates.push_back({i, bestVLine, false, minVDist, vLines[bestVLine]});
            }
        }
        
        return candidates;
    }

    int optimizeVertexAlignment(
        std::vector<BaseVertexProperty>& vertexList, 
        std::vector<BaseEdgeProperty>& edgeList, 
        BaseUGraphProperty& graph,
        const std::string& testCaseName) {
        try {
            std::cout << "=== Starting Vertex Alignment Optimization ===" << std::endl;
        
            int vertexNum = vertexList.size();
            
            if (vertexNum < MIN_CLUSTER_SIZE) {
                std::cout << "Too few vertices for alignment. Skipping optimization." << std::endl;
                return 0;
            }
            
            // Create vertex ID to index mapping
            std::map<unsigned int, int> vertexID2Index = createVertexID2Index(vertexList);
        
            // Calculate coordinate boundaries
            double x_min = vertexList[0].getCoord().x();
            double x_max = vertexList[0].getCoord().x();
            double y_min = vertexList[0].getCoord().y();
            double y_max = vertexList[0].getCoord().y();
        
            std::vector<double> xCoords, yCoords;
            for (const auto& vertex : vertexList) {
                double x = vertex.getCoord().x();
                double y = vertex.getCoord().y();
                x_min = std::min(x_min, x);
                x_max = std::max(x_max, x);
                y_min = std::min(y_min, y);
                y_max = std::max(y_max, y);
                xCoords.push_back(x);
                yCoords.push_back(y);
            }
        
            std::cout << "Coordinate range: X[" << x_min << ", " << x_max << "], Y[" << y_min << ", " << y_max << "]" << std::endl;

            // Phase 1: Detect alignment lines using clustering
            std::cout << "\n=== Phase 1: Line Detection ===" << std::endl;
            
            int optimalHorizontalK, optimalVerticalK;
            std::vector<double> hLines = clusterCoordinates1D(yCoords, optimalHorizontalK);
            std::vector<double> vLines = clusterCoordinates1D(xCoords, optimalVerticalK);
            
            std::cout << "Detected " << hLines.size() << " horizontal alignment lines" << std::endl;
            for (int i = 0; i < hLines.size(); ++i) {
                std::cout << "  H-Line " << i << ": y = " << hLines[i] << std::endl;
            }
            
            std::cout << "Detected " << vLines.size() << " vertical alignment lines" << std::endl;
            for (int i = 0; i < vLines.size(); ++i) {
                std::cout << "  V-Line " << i << ": x = " << vLines[i] << std::endl;
            }

            // If no lines detected, skip optimization
            if (hLines.empty() && vLines.empty()) {
                std::cout << "No alignment lines detected. Skipping optimization." << std::endl;
                return 0;
            }

            // Phase 2: Pre-select vertices for alignment
            std::cout << "\n=== Phase 2: Pre-selection ===" << std::endl;
            
            std::vector<VertexLineCandidate> alignmentCandidates = preSelect(vertexList, hLines, vLines);
            
            std::cout << "Selected " << alignmentCandidates.size() << " alignment constraints:" << std::endl;
            
            // Group candidates by vertex for better display
            std::map<int, std::vector<VertexLineCandidate>> vertexToCandidates;
            for (const auto& candidate : alignmentCandidates) {
                vertexToCandidates[candidate.vertexIdx].push_back(candidate);
            }
            
            for (const auto& pair : vertexToCandidates) {
                int vertexIdx = pair.first;
                const auto& candidates = pair.second;
                
                std::cout << "  Vertex " << vertexList[vertexIdx].getID() << " -> ";
                for (int i = 0; i < candidates.size(); ++i) {
                    if (i > 0) std::cout << " and ";
                    const auto& candidate = candidates[i];
                    std::cout << (candidate.isHorizontal ? "H-Line" : "V-Line") 
                             << " " << candidate.lineIdx 
                             << " (pos=" << candidate.linePosition 
                             << ", dist=" << candidate.distance << ")";
                }
                std::cout << std::endl;
            }

            if (alignmentCandidates.empty()) {
                std::cout << "No vertices selected for alignment. Skipping optimization." << std::endl;
                return 0;
            }

            // Phase 3: Gurobi optimization
            std::cout << "\n=== Phase 3: Optimization ===" << std::endl;
            
            // Create Gurobi environment and model
            GRBEnv env(true);
            env.set("LogFile", "vertex_alignment_opt.log");
            env.start();
            GRBModel model(env);

            // Create decision variables for new coordinates
            std::vector<GRBVar> X(vertexNum), Y(vertexNum);
            for (int i = 0; i < vertexNum; ++i) {
                X[i] = model.addVar(x_min, x_max, 0.0, GRB_CONTINUOUS, "X_" + std::to_string(i));
                Y[i] = model.addVar(y_min, y_max, 0.0, GRB_CONTINUOUS, "Y_" + std::to_string(i));
                
                // Set initial values to original coordinates
                X[i].set(GRB_DoubleAttr_Start, vertexList[i].getCoord().x());
                Y[i].set(GRB_DoubleAttr_Start, vertexList[i].getCoord().y());
            }

            // Add forced alignment constraints for pre-selected vertices
            for (const auto& candidate : alignmentCandidates) {
                if (candidate.isHorizontal) {
                    model.addConstr(Y[candidate.vertexIdx] == candidate.linePosition, 
                                   "force_h_align_" + std::to_string(candidate.vertexIdx));
                } else {
                    model.addConstr(X[candidate.vertexIdx] == candidate.linePosition, 
                                   "force_v_align_" + std::to_string(candidate.vertexIdx));
                }
            }

            // Set objective: minimize coordinate displacement (same as EdgeOrientation)
            
            GRBQuadExpr objective = 0;
            for (int i = 0; i < vertexNum; ++i) {
                double weight = calculateVWeight(vertexList[i], graph);
                objective += weight * ((X[i] - vertexList[i].getCoord().x()) * (X[i] - vertexList[i].getCoord().x()) + 
                            (Y[i] - vertexList[i].getCoord().y()) * (Y[i] - vertexList[i].getCoord().y()));
            }
            
            model.setObjective(objective, GRB_MINIMIZE);
            
            // Solve the optimization problem
            std::cout << "Solving optimization problem..." << std::endl;
            model.optimize();
            
            // Check optimization status and update coordinates
            int status = model.get(GRB_IntAttr_Status);
            if (status == GRB_OPTIMAL) {
                std::cout << "\n=== Optimization completed successfully! ===" << std::endl;
                std::cout << "Optimal objective value: " << model.get(GRB_DoubleAttr_ObjVal) << std::endl;
                
                // Print results and update coordinates
                std::cout << "\n=== Aligned coordinates ===" << std::endl;
                
                // Group alignment candidates by vertex index for display
                std::map<int, std::vector<VertexLineCandidate>> vertexToCandidates;
                for (const auto& candidate : alignmentCandidates) {
                    vertexToCandidates[candidate.vertexIdx].push_back(candidate);
                }
                
                for (int i = 0; i < vertexNum; ++i) {
                    double newX = X[i].get(GRB_DoubleAttr_X);
                    double newY = Y[i].get(GRB_DoubleAttr_X);
                    
                    // Check if this vertex was in the alignment candidates
                    auto it = vertexToCandidates.find(i);
                    if (it != vertexToCandidates.end()) {
                        const auto& candidates = it->second;
                        std::cout << "Vertex " << vertexList[i].getID() << " aligned to ";
                        
                        for (int j = 0; j < candidates.size(); ++j) {
                            if (j > 0) std::cout << " and ";
                            const auto& candidate = candidates[j];
                            std::cout << (candidate.isHorizontal ? "H-line y=" : "V-line x=") 
                                     << candidate.linePosition;
                        }
                        std::cout << " (" << newX << ", " << newY << ")" << std::endl;
                    }
                    
                    // Update vertices in graph and vertexList
                    vertexList[i].setCoord(newX, newY);
                }

                std::cout << "Total aligned vertices: " << vertexToCandidates.size() << "/" << vertexNum << std::endl;

                // Update graph vertices (similar to EdgeOrientation)
                std::pair<BaseUGraphProperty::vertex_iterator, BaseUGraphProperty::vertex_iterator> vp = vertices(graph);
                for (BaseUGraphProperty::vertex_iterator vi = vp.first; vi != vp.second; ++vi) {
                    BaseVertexProperty& vertex = graph[*vi];
                    auto it = vertexID2Index.find(vertex.getID());
                    if (it != vertexID2Index.end()) {
                        int idx = it->second;
                        double newX = X[idx].get(GRB_DoubleAttr_X);
                        double newY = Y[idx].get(GRB_DoubleAttr_X);
                        vertex.setCoord(newX, newY);

                    }
                }

                // Update edge angles (similar to EdgeOrientation)
                std::pair<BaseUGraphProperty::edge_iterator, BaseUGraphProperty::edge_iterator> ep = edges(graph);
                for (BaseUGraphProperty::edge_iterator ei = ep.first; ei != ep.second; ++ei) {
                    BaseEdgeProperty& edge = graph[*ei];
                    
                    BaseUGraphProperty::vertex_descriptor source_desc = boost::source(*ei, graph);
                    BaseUGraphProperty::vertex_descriptor target_desc = boost::target(*ei, graph);
                    
                    // Recalculate angle with updated coordinates
                    double newAngle = calculateAngle(graph[source_desc], graph[target_desc]);
                    edge.setAngle(newAngle);
                    edgeList[edge.ID()].setAngle(newAngle);
                }

                std::string outputFile = "output/" + testCaseName + "_3.svg";
                createVisualization(vertexList, edgeList, outputFile);

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