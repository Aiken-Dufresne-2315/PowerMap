#ifndef _Map_SpatialGrid_H
#define _Map_SpatialGrid_H

#include "BaseEdgeProperty.h"
#include "BaseVertexProperty.h"
#include "BaseUGraphProperty.h"
#include "Coord2.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <functional>

namespace Map {

/**
 * @brief 空间网格哈希结构，用于加速重叠检查
 * 
 * 将2D平面划分为均匀网格，每个网格单元存储其覆盖的顶点和边的ID
 * 时间复杂度：从 O(V*E) 降低到 O(k)，k为局部邻域的元素数量
 */
class SpatialGrid {
public:
    /**
     * @brief 构造函数
     * @param cellSize 网格单元大小（建议设置为平均边长的1-2倍）
     */
    explicit SpatialGrid(double cellSize = 1.0);

    /**
     * @brief 从图构建空间网格
     * @param graph 输入图
     */
    void buildFromGraph(const BaseUGraphProperty& graph);

    /**
     * @brief 清空网格
     */
    void clear();

    /**
     * @brief 获取指定位置附近的顶点ID集合
     * @param pos 查询位置
     * @param radius 查询半径（单位：格子数）
     * @return 顶点ID集合
     */
    std::vector<int> getNearbyVertices(const Coord2& pos, int radius = 1) const;

    /**
     * @brief 获取指定位置附近的边ID集合
     * @param pos 查询位置
     * @param radius 查询半径（单位：格子数）
     * @return 边ID集合
     */
    std::vector<int> getNearbyEdges(const Coord2& pos, int radius = 1) const;

    /**
     * @brief 获取线段覆盖的所有网格单元中的顶点ID
     * @param start 线段起点
     * @param end 线段终点
     * @return 顶点ID集合
     */
    std::vector<int> getVerticesAlongLine(const Coord2& start, const Coord2& end) const;

    /**
     * @brief 获取线段覆盖的所有网格单元中的边ID
     * @param start 线段起点
     * @param end 线段终点
     * @return 边ID集合
     */
    std::vector<int> getEdgesAlongLine(const Coord2& start, const Coord2& end) const;

    /**
     * @brief 设置网格单元大小
     * @param cellSize 新的网格单元大小
     */
    void setCellSize(double cellSize);

    /**
     * @brief 获取网格单元大小
     */
    double getCellSize() const { return m_cellSize; }

private:
    // 网格单元键类型（使用pair<int,int>表示网格坐标）
    using GridKey = std::pair<int, int>;

    // GridKey的哈希函数
    struct GridKeyHash {
        std::size_t operator()(const GridKey& k) const {
            return std::hash<int>()(k.first) ^ (std::hash<int>()(k.second) << 1);
        }
    };

    // 网格单元内容（存储顶点ID和边ID）
    struct GridCell {
        std::unordered_set<int> vertexIDs;
        std::unordered_set<int> edgeIDs;
    };

    /**
     * @brief 将世界坐标转换为网格坐标
     */
    GridKey worldToGrid(const Coord2& pos) const;

    /**
     * @brief 将世界坐标转换为网格坐标（分量）
     */
    GridKey worldToGrid(double x, double y) const;

    /**
     * @brief 向网格中插入顶点
     */
    void insertVertex(int vertexID, const Coord2& pos);

    /**
     * @brief 向网格中插入边（会插入到线段覆盖的所有网格单元）
     */
    void insertEdge(int edgeID, const Coord2& start, const Coord2& end);

    /**
     * @brief 获取线段经过的所有网格单元（Bresenham-like算法）
     */
    std::vector<GridKey> getGridCellsAlongLine(const Coord2& start, const Coord2& end) const;

    /**
     * @brief 获取指定网格单元及其周围radius范围内的所有网格单元
     */
    std::vector<GridKey> getNeighboringCells(const GridKey& center, int radius) const;

private:
    double m_cellSize;  // 网格单元大小
    std::unordered_map<GridKey, GridCell, GridKeyHash> m_grid;  // 空间网格
};

} // namespace Map

#endif // _Map_SpatialGrid_H


