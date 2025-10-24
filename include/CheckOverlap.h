#ifndef _Map_CheckOverlap_H
#define _Map_CheckOverlap_H
#include "BaseEdgeProperty.h"
#include "BaseUGraphProperty.h"
#include "SpatialGrid.h"

namespace Map {

    bool VVOverlap(const BaseVertexProperty& vertex_1, const BaseVertexProperty& vertex_2);

    bool VEOverlap(const BaseVertexProperty& vertex, const BaseEdgeProperty& edge);

    bool EEOverlap(const BaseEdgeProperty& edge_1, const BaseEdgeProperty& edge_2);

    // 原始实现（O(V*E)复杂度）
    bool overlapHappens(int vertexID, const Coord2& newPos, const BaseUGraphProperty& graph);

    /**
     * @brief 优化的重叠检查函数（使用空间网格加速）
     * @param vertexID 要移动的顶点ID
     * @param newPos 新位置
     * @param graph 图结构
     * @param spatialGrid 空间网格索引（如果为nullptr则自动创建）
     * @return 是否发生重叠
     * 
     * 时间复杂度：O(k) 其中k是局部邻域的元素数量，通常 k << V+E
     * 相比原始实现的 O(V*E) 有显著提升
     */
    bool overlapHappensOptimized(int vertexID, const Coord2& newPos, 
                                  const BaseUGraphProperty& graph,
                                  SpatialGrid* spatialGrid = nullptr);
}

#endif // _Map_CheckOverlap_H