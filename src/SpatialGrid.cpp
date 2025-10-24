#include "SpatialGrid.h"
#include <cmath>
#include <algorithm>
#include <boost/graph/graph_traits.hpp>

namespace Map {

SpatialGrid::SpatialGrid(double cellSize) 
    : m_cellSize(cellSize) {
    if (m_cellSize <= 0) {
        m_cellSize = 1.0;
    }
}

void SpatialGrid::buildFromGraph(const BaseUGraphProperty& graph) {
    clear();

    // 插入所有顶点
    auto vp = boost::vertices(graph);
    for (auto vit = vp.first; vit != vp.second; ++vit) {
        const auto& vertex = graph[*vit];
        insertVertex(vertex.getID(), vertex.getCoord());
    }

    // 插入所有边
    auto ep = boost::edges(graph);
    for (auto eit = ep.first; eit != ep.second; ++eit) {
        const auto& edge = graph[*eit];
        insertEdge(edge.ID(), 
                   edge.Source().getCoord(), 
                   edge.Target().getCoord());
    }
}

void SpatialGrid::clear() {
    m_grid.clear();
}

SpatialGrid::GridKey SpatialGrid::worldToGrid(const Coord2& pos) const {
    return worldToGrid(pos.x(), pos.y());
}

SpatialGrid::GridKey SpatialGrid::worldToGrid(double x, double y) const {
    int gridX = static_cast<int>(std::floor(x / m_cellSize));
    int gridY = static_cast<int>(std::floor(y / m_cellSize));
    return {gridX, gridY};
}

void SpatialGrid::insertVertex(int vertexID, const Coord2& pos) {
    GridKey key = worldToGrid(pos);
    m_grid[key].vertexIDs.insert(vertexID);
}

void SpatialGrid::insertEdge(int edgeID, const Coord2& start, const Coord2& end) {
    // 获取线段经过的所有网格单元
    std::vector<GridKey> cells = getGridCellsAlongLine(start, end);
    
    // 将边ID添加到所有经过的网格单元
    for (const auto& cell : cells) {
        m_grid[cell].edgeIDs.insert(edgeID);
    }
}

std::vector<SpatialGrid::GridKey> SpatialGrid::getGridCellsAlongLine(
    const Coord2& start, const Coord2& end) const {
    
    std::vector<GridKey> cells;
    
    GridKey startCell = worldToGrid(start);
    GridKey endCell = worldToGrid(end);
    
    int x0 = startCell.first;
    int y0 = startCell.second;
    int x1 = endCell.first;
    int y1 = endCell.second;
    
    // 使用DDA算法（Digital Differential Analyzer）遍历线段经过的网格
    int dx = std::abs(x1 - x0);
    int dy = std::abs(y1 - y0);
    int steps = std::max(dx, dy) + 1;
    
    // 为了保证覆盖，额外检查线段实际经过的单元
    // 使用参数方程遍历
    for (int i = 0; i <= steps; ++i) {
        double t = (steps > 0) ? static_cast<double>(i) / steps : 0.0;
        double x = start.x() + t * (end.x() - start.x());
        double y = start.y() + t * (end.y() - start.y());
        
        GridKey cell = worldToGrid(x, y);
        
        // 避免重复添加
        if (cells.empty() || cells.back() != cell) {
            cells.push_back(cell);
        }
    }
    
    // 额外检查线段端点的邻近单元（为了处理边界情况）
    // 这确保了线段与网格边界相交的情况也能被正确处理
    std::unordered_set<GridKey, GridKeyHash> cellSet(cells.begin(), cells.end());
    
    // 检查起点和终点周围的单元
    for (int dx = -1; dx <= 1; ++dx) {
        for (int dy = -1; dy <= 1; ++dy) {
            GridKey startNeighbor = {startCell.first + dx, startCell.second + dy};
            GridKey endNeighbor = {endCell.first + dx, endCell.second + dy};
            
            // 简单的边界框检查：只添加可能与线段相交的邻近单元
            double startX = start.x();
            double startY = start.y();
            double endX = end.x();
            double endY = end.y();
            
            double minX = std::min(startX, endX) - m_cellSize;
            double maxX = std::max(startX, endX) + m_cellSize;
            double minY = std::min(startY, endY) - m_cellSize;
            double maxY = std::max(startY, endY) + m_cellSize;
            
            // 检查邻近单元是否在边界框内
            double neighborMinX = startNeighbor.first * m_cellSize;
            double neighborMaxX = (startNeighbor.first + 1) * m_cellSize;
            double neighborMinY = startNeighbor.second * m_cellSize;
            double neighborMaxY = (startNeighbor.second + 1) * m_cellSize;
            
            if (neighborMaxX >= minX && neighborMinX <= maxX &&
                neighborMaxY >= minY && neighborMinY <= maxY) {
                cellSet.insert(startNeighbor);
            }
            
            neighborMinX = endNeighbor.first * m_cellSize;
            neighborMaxX = (endNeighbor.first + 1) * m_cellSize;
            neighborMinY = endNeighbor.second * m_cellSize;
            neighborMaxY = (endNeighbor.second + 1) * m_cellSize;
            
            if (neighborMaxX >= minX && neighborMinX <= maxX &&
                neighborMaxY >= minY && neighborMinY <= maxY) {
                cellSet.insert(endNeighbor);
            }
        }
    }
    
    cells.assign(cellSet.begin(), cellSet.end());
    return cells;
}

std::vector<SpatialGrid::GridKey> SpatialGrid::getNeighboringCells(
    const GridKey& center, int radius) const {
    
    std::vector<GridKey> neighbors;
    
    for (int dx = -radius; dx <= radius; ++dx) {
        for (int dy = -radius; dy <= radius; ++dy) {
            neighbors.push_back({center.first + dx, center.second + dy});
        }
    }
    
    return neighbors;
}

std::vector<int> SpatialGrid::getNearbyVertices(const Coord2& pos, int radius) const {
    GridKey center = worldToGrid(pos);
    std::vector<GridKey> cells = getNeighboringCells(center, radius);
    
    std::unordered_set<int> vertexIDSet;
    
    for (const auto& cell : cells) {
        auto it = m_grid.find(cell);
        if (it != m_grid.end()) {
            const auto& cellData = it->second;
            vertexIDSet.insert(cellData.vertexIDs.begin(), cellData.vertexIDs.end());
        }
    }
    
    return std::vector<int>(vertexIDSet.begin(), vertexIDSet.end());
}

std::vector<int> SpatialGrid::getNearbyEdges(const Coord2& pos, int radius) const {
    GridKey center = worldToGrid(pos);
    std::vector<GridKey> cells = getNeighboringCells(center, radius);
    
    std::unordered_set<int> edgeIDSet;
    
    for (const auto& cell : cells) {
        auto it = m_grid.find(cell);
        if (it != m_grid.end()) {
            const auto& cellData = it->second;
            edgeIDSet.insert(cellData.edgeIDs.begin(), cellData.edgeIDs.end());
        }
    }
    
    return std::vector<int>(edgeIDSet.begin(), edgeIDSet.end());
}

std::vector<int> SpatialGrid::getVerticesAlongLine(
    const Coord2& start, const Coord2& end) const {
    
    std::vector<GridKey> cells = getGridCellsAlongLine(start, end);
    std::unordered_set<int> vertexIDSet;
    
    for (const auto& cell : cells) {
        auto it = m_grid.find(cell);
        if (it != m_grid.end()) {
            const auto& cellData = it->second;
            vertexIDSet.insert(cellData.vertexIDs.begin(), cellData.vertexIDs.end());
        }
    }
    
    return std::vector<int>(vertexIDSet.begin(), vertexIDSet.end());
}

std::vector<int> SpatialGrid::getEdgesAlongLine(
    const Coord2& start, const Coord2& end) const {
    
    std::vector<GridKey> cells = getGridCellsAlongLine(start, end);
    std::unordered_set<int> edgeIDSet;
    
    for (const auto& cell : cells) {
        auto it = m_grid.find(cell);
        if (it != m_grid.end()) {
            const auto& cellData = it->second;
            edgeIDSet.insert(cellData.edgeIDs.begin(), cellData.edgeIDs.end());
        }
    }
    
    return std::vector<int>(edgeIDSet.begin(), edgeIDSet.end());
}

void SpatialGrid::setCellSize(double cellSize) {
    if (cellSize > 0) {
        m_cellSize = cellSize;
    }
}

} // namespace Map


