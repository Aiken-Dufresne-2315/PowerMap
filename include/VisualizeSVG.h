#ifndef _Map_VisualizeSVG_H
#define _Map_VisualizeSVG_H

#include <vector>
#include <string>
#include "BaseVertexProperty.h"
#include "BaseEdgeProperty.h"

namespace Map {

    // Visualization functions
    void createVisualization(
        const std::vector<BaseVertexProperty>& vertices, 
        const std::vector<BaseEdgeProperty>& edges,
        const std::string& filename = "map_visualization.svg");
}

#endif // _Map_VisualizeSVG_H