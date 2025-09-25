#include "VisualizeSVG.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <vector>
#include <string>
#include "BaseVertexProperty.h"
#include "BaseEdgeProperty.h"


namespace Map {
    
    //------------------------------------------------------------------------------
    // create SVG visualization of the stations
    //------------------------------------------------------------------------------
    void createVisualization(
        const std::vector<BaseVertexProperty>& vertices, 
        const std::vector<BaseEdgeProperty>& edges,
        const std::string& filename) {
        
        if (vertices.empty()) {
            std::cerr << "no vertices to visualize!" << std::endl;
            return;
        }
        
        // calculate coordinate bounds
        double minX = vertices[0].getCoord().x();
        double maxX = vertices[0].getCoord().x();
        double minY = vertices[0].getCoord().y();
        double maxY = vertices[0].getCoord().y();
        
        for (const auto& vertex : vertices) {
            minX = std::min(minX, vertex.getCoord().x());
            maxX = std::max(maxX, vertex.getCoord().x());
            minY = std::min(minY, vertex.getCoord().y());
            maxY = std::max(maxY, vertex.getCoord().y());
        }
        
        // add padding
        double padding = 50;
        double width = (maxX - minX) + 2 * padding;
        double height = (maxY - minY) + 2 * padding;
        
        // create SVG file
        std::ofstream svgFile(filename);
        if (!svgFile.is_open()) {
            std::cerr << "error: cannot create SVG file " << filename << std::endl;
            return;
        }
        
        // SVG header
        svgFile << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        svgFile << "<svg width=\"" << width << "\" height=\"" << height << "\" ";
        svgFile << "viewBox=\"0 0 " << width << " " << height << "\" ";
        svgFile << "xmlns=\"http://www.w3.org/2000/svg\">\n";
        
        // add styles
        svgFile << "<defs>\n";
        svgFile << "  <style>\n";
        svgFile << "    .station { fill: #2563eb; stroke: #1e40af; stroke-width: 2; }\n";
        svgFile << "    .station-id { font-family: Arial, sans-serif; font-size: 12px; ";
        svgFile << "fill: white; text-anchor: middle; dominant-baseline: central; }\n";
        svgFile << "    .edge { stroke: #6b7280; stroke-width: 2; }\n";
        svgFile << "    .background { fill: #f8fafc; }\n";
        svgFile << "  </style>\n";
        svgFile << "</defs>\n";
        
        // background
        svgFile << "<rect class=\"background\" width=\"" << width << "\" height=\"" << height << "\"/>\n";
        
        // draw edges first (so they appear behind stations)
        svgFile << "<!-- Edges -->\n";
        for (const auto& edge : edges) {
            double sourceX = edge.Source().getCoord().x() - minX + padding;
            double sourceY = edge.Source().getCoord().y() - minY + padding;
            double targetX = edge.Target().getCoord().x() - minX + padding;
            double targetY = edge.Target().getCoord().y() - minY + padding;
            
            svgFile << "<line class=\"edge\" x1=\"" << sourceX << "\" y1=\"" << sourceY 
                    << "\" x2=\"" << targetX << "\" y2=\"" << targetY << "\"/>\n";
        }
        
        // draw stations
        svgFile << "<!-- Stations -->\n";
        for (const auto& vertex : vertices) {
            double x = vertex.getCoord().x() - minX + padding;
            double y = vertex.getCoord().y() - minY + padding;
            
            // draw station circle
            svgFile << "<circle class=\"station\" cx=\"" << x << "\" cy=\"" << y << "\" r=\"15\"/>\n";
            
            // draw station ID number
            svgFile << "<text class=\"station-id\" x=\"" << x << "\" y=\"" << y << "\">" << vertex.getID() << "</text>\n";
        }
        
        // add title
        svgFile << "<text x=\"" << width/2 << "\" y=\"25\" ";
        svgFile << "style=\"font-family: Arial, sans-serif; font-size: 18px; font-weight: bold; ";
        svgFile << "text-anchor: middle; fill: #1f2937;\">Metro Map Visualization</text>\n";
        
        // add coordinate info
        svgFile << "<text x=\"10\" y=\"" << height - 10 << "\" ";
        svgFile << "style=\"font-family: Arial, sans-serif; font-size: 10px; fill: #6b7280;\">";
        svgFile << "Coordinate range: X[" << minX << ", " << maxX << "], Y[" << minY << ", " << maxY << "]</text>\n";
        
        svgFile << "</svg>\n";
        svgFile.close();
        
        std::cout << "\nvisualization created successfully!" << std::endl;
        std::cout << "SVG file saved as: " << filename << std::endl;
        std::cout << "you can open this file in any web browser to view the visualization." << std::endl;
    }
}