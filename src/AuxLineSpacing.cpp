//------------------------------------------------------------------------------
// AuxLineSpacing.cpp - Auxiliary Line Spacing Optimization Implementation
//------------------------------------------------------------------------------

#include "AuxLineSpacing.h"
#include "VisualizeSVG.h"
#include "MapFileReader.h"
#include "Commons.h"
#include "gurobi_c++.h"

#include <iostream>
#include <vector>
#include <algorithm>
#include <map>

namespace Map {

    int optimizeLineSpacing(
        std::vector<AuxiliaryLine>& auxLines,
        bool isHorizontal,
        double minSpacing,
        std::vector<double>& newPositions) {
        
        int lineCount = auxLines.size();
        
        if (lineCount < 2) {
            std::cout << "Too few auxiliary lines (" << lineCount << "), skipping optimization." << std::endl;
            return 0;
        }
        
        std::cout << "\n=== Optimizing " << (isHorizontal ? "Horizontal" : "Vertical") 
                  << " Auxiliary Line Spacing ===" << std::endl;
        std::cout << "Number of lines: " << lineCount << std::endl;
        
        // Sort auxiliary lines by position
        std::vector<std::pair<double, int>> sortedLines;
        for (int i = 0; i < lineCount; ++i) {
            sortedLines.push_back({auxLines[i].getPosition(), i});
        }
        std::sort(sortedLines.begin(), sortedLines.end());
        
        // Get original positions in sorted order
        std::vector<double> originalPositions;
        std::vector<int> originalIndices;
        for (const auto& pair : sortedLines) {
            originalPositions.push_back(pair.first);
            originalIndices.push_back(pair.second);
        }
        
        double firstPos = originalPositions.front();
        double lastPos = originalPositions.back();
        double totalRange = lastPos - firstPos;
        
        std::cout << "Position range: [" << firstPos << ", " << lastPos << "]" << std::endl;
        std::cout << "Total range: " << totalRange << std::endl;
        
        // Calculate target spacing
        double targetSpacing = totalRange / (lineCount - 1);
        std::cout << "Target spacing: " << targetSpacing << std::endl;
        
        if (targetSpacing < minSpacing) {
            std::cerr << "Warning: Target spacing (" << targetSpacing 
                     << ") is less than minimum spacing (" << minSpacing << ")" << std::endl;
            targetSpacing = minSpacing;
        }
        
        try {
            // Create Gurobi environment and model
            GRBEnv env(true);
            env.set("LogFile", "auxline_spacing_opt.log");
            env.set(GRB_IntParam_OutputFlag, 0);  // Suppress output
            env.start();
            GRBModel model(env);
            
            // Create decision variables for line positions
            std::vector<GRBVar> P(lineCount);
            for (int i = 0; i < lineCount; ++i) {
                P[i] = model.addVar(firstPos, lastPos, 0.0, GRB_CONTINUOUS, 
                                   "P_" + std::to_string(i));
                P[i].set(GRB_DoubleAttr_Start, originalPositions[i]);
            }
            
            // Fix first and last line positions to maintain overall range
            model.addConstr(P[0] == firstPos, "fix_first");
            model.addConstr(P[lineCount-1] == lastPos, "fix_last");
            
            // Add ordering constraints with minimum spacing
            for (int i = 0; i < lineCount - 1; ++i) {
                model.addConstr(P[i] + minSpacing <= P[i+1], 
                               "order_" + std::to_string(i));
            }
            
            // Objective: minimize deviation from uniform spacing
            GRBQuadExpr objective = 0;
            for (int i = 0; i < lineCount - 1; ++i) {
                GRBLinExpr spacing = P[i+1] - P[i];
                objective += (spacing - targetSpacing) * (spacing - targetSpacing);
            }
            
            model.setObjective(objective, GRB_MINIMIZE);
            
            // Solve the optimization problem
            std::cout << "Solving spacing optimization..." << std::endl;
            model.optimize();
            
            // Check optimization status
            int status = model.get(GRB_IntAttr_Status);
            if (status == GRB_OPTIMAL) {
                std::cout << "Optimization completed successfully!" << std::endl;
                std::cout << "Optimal objective value: " << model.get(GRB_DoubleAttr_ObjVal) << std::endl;
                
                // Extract optimized positions
                newPositions.resize(lineCount);
                std::cout << "\n=== Optimized line positions ===" << std::endl;
                for (int i = 0; i < lineCount; ++i) {
                    newPositions[originalIndices[i]] = P[i].get(GRB_DoubleAttr_X);
                    std::cout << "Line " << originalIndices[i] 
                             << ": " << originalPositions[i] 
                             << " -> " << P[i].get(GRB_DoubleAttr_X);
                    if (i > 0) {
                        double actualSpacing = P[i].get(GRB_DoubleAttr_X) - P[i-1].get(GRB_DoubleAttr_X);
                        std::cout << " (spacing: " << actualSpacing << ")";
                    }
                    std::cout << std::endl;
                }
                
                return 0;
                
            } else if (status == GRB_INFEASIBLE) {
                std::cerr << "Model is infeasible!" << std::endl;
                model.computeIIS();
                model.write("auxline_spacing_infeasible.ilp");
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
    }

    int uniformAuxLineSpacing(
        std::vector<BaseVertexProperty>& vertexList,
        std::vector<BaseEdgeProperty>& edgeList,
        BaseUGraphProperty& graph,
        DynamicGrid& grid,
        double minSpacing,
        const std::string& testCaseName) {
        
        std::cout << "\n========================================" << std::endl;
        std::cout << "=== Auxiliary Line Spacing Optimization ===" << std::endl;
        std::cout << "========================================" << std::endl;
        
        // IMPORTANT: Rebuild vertex-line mappings before optimization
        // This ensures vertexIDs are up-to-date after DVPositioning
        grid.rebuildVertexLineMappings(graph);
        grid.printAuxLineInfo();
        
        // Create vertex ID to index mapping
        std::map<unsigned int, int> vertexID2Index = createVertexID2Index(vertexList);
        
        // Get mutable copies of auxiliary lines
        std::vector<AuxiliaryLine> horizontalLines = grid.getHorizontalAuxLines();
        std::vector<AuxiliaryLine> verticalLines = grid.getVerticalAuxLines();
        
        // ==================== Step 1: Optimize Horizontal Auxiliary Lines ====================
        std::vector<double> newHorizontalPositions;
        if (horizontalLines.size() >= 2) {
            int result = optimizeLineSpacing(horizontalLines, true, minSpacing, newHorizontalPositions);
            if (result != 0) {
                std::cerr << "Failed to optimize horizontal line spacing!" << std::endl;
                return -1;
            }
            
            // Update horizontal auxiliary line positions in grid
            std::cout << "\n=== Updating horizontal auxiliary lines ===" << std::endl;
            
            const double EPSILON = 1e-2;
            
            // Build a map from old position to new position
            std::map<double, double> oldToNewY;
            for (size_t i = 0; i < horizontalLines.size(); ++i) {
                oldToNewY[horizontalLines[i].getPosition()] = newHorizontalPositions[i];
            }
            
            // Update all vertices by checking if they are on any horizontal line
            int updatedCount = 0;
            for (auto& vertex : vertexList) {
                double currentY = vertex.getCoord().y();
                
                // Check if vertex is on any horizontal auxiliary line
                for (const auto& pair : oldToNewY) {
                    double oldY = pair.first;
                    double newY = pair.second;
                    
                    if (std::abs(currentY - oldY) < EPSILON) {
                        // Update vertexList
                        vertex.setCoord(vertex.getCoord().x(), newY);
                        
                        // Update graph
                        try {
                            BaseUGraphProperty::vertex_descriptor vd = getVertexDescriptor(vertex.getID());
                            graph[vd].setCoord(graph[vd].getCoord().x(), newY);
                            updatedCount++;
                        } catch (const std::exception& e) {
                            std::cerr << "Error updating vertex " << vertex.getID() << ": " << e.what() << std::endl;
                        }
                        break;
                    }
                }
            }
            
            std::cout << "Updated " << updatedCount << " vertices for horizontal line repositioning" << std::endl;
            
            // Print summary
            for (size_t i = 0; i < horizontalLines.size(); ++i) {
                std::cout << "H-Line " << i << ": Y " 
                         << horizontalLines[i].getPosition() << " -> " 
                         << newHorizontalPositions[i] << std::endl;
            }
        } else {
            std::cout << "Skipping horizontal line optimization (insufficient lines)" << std::endl;
        }
        
        // ==================== Step 2: Optimize Vertical Auxiliary Lines ====================
        std::vector<double> newVerticalPositions;
        if (verticalLines.size() >= 2) {
            int result = optimizeLineSpacing(verticalLines, false, minSpacing, newVerticalPositions);
            if (result != 0) {
                std::cerr << "Failed to optimize vertical line spacing!" << std::endl;
                return -1;
            }
            
            // Update vertical auxiliary line positions in grid
            std::cout << "\n=== Updating vertical auxiliary lines ===" << std::endl;
            
            const double EPSILON = 1e-2;
            
            // Build a map from old position to new position
            std::map<double, double> oldToNewX;
            for (size_t i = 0; i < verticalLines.size(); ++i) {
                oldToNewX[verticalLines[i].getPosition()] = newVerticalPositions[i];
            }
            
            // Update all vertices by checking if they are on any vertical line
            int updatedCount = 0;
            for (auto& vertex : vertexList) {
                double currentX = vertex.getCoord().x();
                
                // Check if vertex is on any vertical auxiliary line
                for (const auto& pair : oldToNewX) {
                    double oldX = pair.first;
                    double newX = pair.second;
                    
                    if (std::abs(currentX - oldX) < EPSILON) {
                        // Update vertexList
                        vertex.setCoord(newX, vertex.getCoord().y());
                        
                        // Update graph
                        try {
                            BaseUGraphProperty::vertex_descriptor vd = getVertexDescriptor(vertex.getID());
                            graph[vd].setCoord(newX, graph[vd].getCoord().y());
                            updatedCount++;
                        } catch (const std::exception& e) {
                            std::cerr << "Error updating vertex " << vertex.getID() << ": " << e.what() << std::endl;
                        }
                        break;
                    }
                }
            }
            
            std::cout << "Updated " << updatedCount << " vertices for vertical line repositioning" << std::endl;
            
            // Print summary
            for (size_t i = 0; i < verticalLines.size(); ++i) {
                std::cout << "V-Line " << i << ": X " 
                         << verticalLines[i].getPosition() << " -> " 
                         << newVerticalPositions[i] << std::endl;
            }
        } else {
            std::cout << "Skipping vertical line optimization (insufficient lines)" << std::endl;
        }
        
        // ==================== Step 3: Update Edge Angles ====================
        std::cout << "\n=== Updating edge angles ===" << std::endl;
        auto ep = boost::edges(graph);
        for (auto ei = ep.first; ei != ep.second; ++ei) {
            BaseEdgeProperty& edge = graph[*ei];
            
            auto source_desc = boost::source(*ei, graph);
            auto target_desc = boost::target(*ei, graph);
            
            double oldAngle = edge.Angle();
            double newAngle = calculateAngle(graph[source_desc], graph[target_desc]);
            
            edge.setAngle(newAngle);
            edgeList[edge.ID()].setAngle(newAngle);
        }
        std::cout << "Updated " << edgeList.size() << " edge angles" << std::endl;
        
        // ==================== Step 4: Update Grid with new line positions ====================
        std::cout << "\n=== Updating dynamic grid with new line positions ===" << std::endl;
        
        // Update grid's internal auxiliary lines with new positions
        if (newHorizontalPositions.size() > 0) {
            grid.updateHorizontalLinePositions(newHorizontalPositions);
        }
        
        if (newVerticalPositions.size() > 0) {
            grid.updateVerticalLinePositions(newVerticalPositions);
        }
        
        // Rebuild vertex-line mappings again to ensure consistency
        grid.rebuildVertexLineMappings(graph);
        grid.printAuxLineInfo();
        
        // ==================== Step 5: Generate Visualization ====================
        std::string outputFile = "output/" + testCaseName + "_5.svg";
        createVisualization(vertexList, edgeList, outputFile);
        
        std::cout << "\n=== Auxiliary Line Spacing Optimization Completed Successfully! ===" << std::endl;
        return 0;
    }

} // namespace Map

