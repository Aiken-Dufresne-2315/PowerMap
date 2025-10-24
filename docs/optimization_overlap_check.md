# 重叠检查优化方案

## 问题分析

### 原始实现的性能瓶颈

原始的 `overlapHappens` 函数通过简单遍历实现重叠检查，存在严重的性能问题：

```cpp
// 原始实现的时间复杂度分析：
// 1. V-V 检查: O(V) - 遍历所有顶点
// 2. V-E 检查: O(E) + O(d(v)·V) - 遍历所有边 + 每条出边遍历所有顶点
// 3. E-E 检查: O(d(v)·E) + O(d(v)²) - 每条出边遍历所有边
// 总复杂度: O(d(v)·E + d(v)·V) ≈ O(V·E) 在密集图中
```

其中：
- `V` = 顶点数量
- `E` = 边数量
- `d(v)` = 顶点v的度数

在大规模电网拓扑中（例如 V=500个节点, E=800条线路），每次重叠检查可能需要数十万次比较操作，严重影响优化算法的收敛速度。

---

## 优化方案：空间网格哈希（Spatial Grid Hashing）

### 核心思想

将 2D 平面划分为均匀网格，每个网格单元存储其覆盖的顶点和边的ID。检查重叠时，只需查询相关网格单元及其邻居，而不是遍历所有元素。

### 算法原理

1. **空间划分**：将平面划分为大小为 `cellSize × cellSize` 的网格
2. **索引构建**：
   - 顶点：插入到其所在的网格单元
   - 边：插入到其线段经过的所有网格单元（使用DDA算法）
3. **查询优化**：
   - V-V检查：只查询目标位置周围 3×3 网格内的顶点
   - V-E检查：只查询目标位置周围网格内的边
   - E-E检查：只查询边路径覆盖的网格内的其他边

### 时间复杂度分析

| 操作 | 原始实现 | 优化实现 | 改进 |
|------|---------|---------|------|
| 索引构建 | N/A | O(V + E·L) | 一次性成本 |
| V-V 检查 | O(V) | O(k_v) | k_v ≈ 9个单元的顶点数 |
| V-E 检查 | O(E + d(v)·V) | O(k_e + d(v)·k_v) | k_e ≈ 局部边数 |
| E-E 检查 | O(d(v)·E) | O(d(v)·k_e) | k_e << E |
| **总计** | **O(V·E)** | **O(k)** | **k << V+E** |

其中：
- `L` = 平均边长覆盖的网格数（通常 < 10）
- `k_v` = 局部网格内的顶点数（通常 < 10）
- `k_e` = 局部网格内的边数（通常 < 20）

**性能提升估算**：
- 对于 500 节点、800 条线路的电网：从 **O(400,000)** 降至 **O(100)** 
- **理论加速比：400x - 1000x**

---

## 使用指南

### 1. 基本用法（自动模式）

最简单的使用方式是直接调用优化函数，它会自动创建临时网格：

```cpp
#include "CheckOverlap.h"

// 在你的优化循环中
bool hasOverlap = Map::overlapHappensOptimized(vertexID, newPos, graph);
```

**优点**：无需管理网格生命周期  
**缺点**：每次调用都重新构建网格（适合单次检查）

---

### 2. 高级用法（显式网格管理）

对于需要多次重叠检查的场景（如优化算法的迭代），建议显式创建和管理 `SpatialGrid`：

```cpp
#include "CheckOverlap.h"
#include "SpatialGrid.h"

// 1. 创建空间网格（一次性）
Map::SpatialGrid spatialGrid(2.0);  // 网格单元大小=2.0
spatialGrid.buildFromGraph(graph);

// 2. 在优化循环中重复使用
for (int iter = 0; iter < maxIterations; ++iter) {
    // ... 生成新位置 ...
    
    bool hasOverlap = Map::overlapHappensOptimized(
        vertexID, newPos, graph, &spatialGrid
    );
    
    if (!hasOverlap) {
        // 更新顶点位置
        // ...
        
        // 重要：位置变化后需要重建网格
        spatialGrid.clear();
        spatialGrid.buildFromGraph(graph);
    }
}
```

**关键点**：
- 图结构变化后必须调用 `spatialGrid.buildFromGraph(graph)` 重建索引
- 网格构建成本为 O(V+E)，但远小于重复的 O(V·E) 检查

---

### 3. 网格大小参数调优

网格单元大小（`cellSize`）是影响性能的关键参数：

```cpp
// 根据地图特征选择合适的 cellSize
double avgEdgeLength = computeAverageEdgeLength(graph);
double optimalCellSize = avgEdgeLength * 1.5;  // 经验值：1-2倍边长

Map::SpatialGrid spatialGrid(optimalCellSize);
```

**选择建议**：
- **太小**：每条边跨越多个网格，查询开销增大
- **太大**：每个网格包含太多元素，退化为线性查询
- **推荐**：设置为平均边长的 1-2 倍

---

### 4. 完整示例：集成到优化算法

以下是在 Gurobi 优化回调中使用的示例：

```cpp
#include "CheckOverlap.h"
#include "SpatialGrid.h"
#include "gurobi_c++.h"

class OptimizationCallback : public GRBCallback {
private:
    Map::SpatialGrid m_spatialGrid;
    BaseUGraphProperty& m_graph;
    
public:
    OptimizationCallback(BaseUGraphProperty& graph, double cellSize) 
        : m_graph(graph), m_spatialGrid(cellSize) {
        m_spatialGrid.buildFromGraph(graph);
    }
    
protected:
    void callback() override {
        if (where == GRB_CB_MIPSOL) {
            // 获取候选解
            int vertexID = /* ... */;
            double newX = getSolution(xVars[vertexID]);
            double newY = getSolution(yVars[vertexID]);
            Coord2 newPos(newX, newY);
            
            // 使用优化的重叠检查
            if (Map::overlapHappensOptimized(vertexID, newPos, m_graph, &m_spatialGrid)) {
                // 添加懒惰约束排除这个解
                // ...
            }
        }
    }
};
```

---

## 实现细节

### SpatialGrid 类接口

```cpp
class SpatialGrid {
public:
    // 构造函数
    explicit SpatialGrid(double cellSize = 1.0);
    
    // 从图构建索引
    void buildFromGraph(const BaseUGraphProperty& graph);
    
    // 清空网格
    void clear();
    
    // 查询操作
    std::vector<int> getNearbyVertices(const Coord2& pos, int radius = 1) const;
    std::vector<int> getNearbyEdges(const Coord2& pos, int radius = 1) const;
    std::vector<int> getVerticesAlongLine(const Coord2& start, const Coord2& end) const;
    std::vector<int> getEdgesAlongLine(const Coord2& start, const Coord2& end) const;
    
    // 参数调整
    void setCellSize(double cellSize);
    double getCellSize() const;
};
```

### 网格单元键值

```cpp
// 使用 (gridX, gridY) 作为哈希键
GridKey worldToGrid(const Coord2& pos) const {
    int gridX = static_cast<int>(std::floor(pos.x() / m_cellSize));
    int gridY = static_cast<int>(std::floor(pos.y() / m_cellSize));
    return {gridX, gridY};
}
```

### 线段网格遍历

使用改进的DDA算法计算线段经过的所有网格单元：

```cpp
std::vector<GridKey> getGridCellsAlongLine(const Coord2& start, const Coord2& end) const {
    // 参数方程遍历：P(t) = start + t * (end - start), t ∈ [0, 1]
    // 同时检查端点邻近单元处理边界情况
}
```

---

## 性能测试建议

### 测试脚本示例

```cpp
#include <chrono>

void benchmarkOverlapCheck() {
    // 加载测试数据
    BaseUGraphProperty graph = loadTestGraph("input/test50.txt");
    
    // 测试原始方法
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        overlapHappens(testVertexID, testPos, graph);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration_old = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // 测试优化方法
    Map::SpatialGrid spatialGrid(2.0);
    spatialGrid.buildFromGraph(graph);
    
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        overlapHappensOptimized(testVertexID, testPos, graph, &spatialGrid);
    }
    end = std::chrono::high_resolution_clock::now();
    auto duration_new = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "原始方法: " << duration_old.count() << " ms" << std::endl;
    std::cout << "优化方法: " << duration_new.count() << " ms" << std::endl;
    std::cout << "加速比: " << (double)duration_old.count() / duration_new.count() << "x" << std::endl;
}
```

---

## 注意事项

### 1. 网格更新策略

- **增量更新**：如果只有少量顶点移动，可以只更新相关网格单元
- **批量重建**：如果变化较多，直接 `clear()` + `buildFromGraph()` 更高效

### 2. 内存占用

```
内存占用 ≈ (V + E·L) × sizeof(int) + 网格单元数 × sizeof(GridCell)
```

对于典型地铁网络（V=500, E=800, L=5）：
- 元素存储：~20 KB
- 网格单元：取决于地图范围和 cellSize

### 3. 边界情况处理

- 网格边界处的线段需要特别处理（已在实现中通过邻近单元检查解决）
- 极短边（< cellSize）确保至少插入到起点和终点的网格

### 4. 线程安全

当前实现不是线程安全的。如需并行优化，建议：
- 每个线程维护独立的 `SpatialGrid` 实例
- 或者使用读写锁保护共享网格

---

## 进一步优化方向

### 1. 自适应网格

根据地图的密度分布使用不同的网格大小：

```cpp
// 密集区域使用较小的网格，稀疏区域使用较大的网格
class AdaptiveSpatialGrid : public SpatialGrid {
    std::unordered_map<GridKey, double> m_adaptiveCellSizes;
};
```

### 2. 层次化网格（Hierarchical Grid）

类似四叉树的多层次结构：

```
Level 0: cellSize = 8.0  (粗粒度，快速剔除)
Level 1: cellSize = 4.0
Level 2: cellSize = 2.0  (细粒度，精确查询)
```

### 3. R-tree 索引

对于非均匀分布的数据，R-tree 可能更优：

```cpp
#include <boost/geometry/index/rtree.hpp>

// 使用 Boost.Geometry 的 R-tree
using RTree = boost::geometry::index::rtree<...>;
```

### 4. GPU 加速

对于超大规模网络（V > 10000），可以考虑 CUDA 并行：

```cpp
// 在 GPU 上并行检查多个候选位置
__global__ void checkOverlapKernel(/* ... */);
```

---

## 常见问题

**Q1: 何时使用优化版本？**  
A: 当图规模 > 100 顶点，或需要频繁重叠检查时（如优化迭代）。

**Q2: cellSize 如何选择？**  
A: 推荐设置为平均边长的 1-2 倍。可以通过性能测试找到最优值。

**Q3: 如何验证正确性？**  
A: 在小规模测试集上对比原始和优化版本的结果，确保一致。

**Q4: 能否增量更新网格？**  
A: 可以，但需要额外实现 `updateVertex()` 和 `updateEdge()` 方法。对于当前场景，批量重建已足够高效。

---

## 总结

通过引入空间网格哈希，我们将重叠检查的时间复杂度从 **O(V·E)** 降低到 **O(k)**，在典型地铁网络中可实现 **100-1000 倍的性能提升**。

**推荐使用策略**：
- 小规模图（< 50 顶点）：使用原始 `overlapHappens`
- 中大规模图（> 50 顶点）：使用 `overlapHappensOptimized`
- 频繁检查场景：显式管理 `SpatialGrid` 实例

这个优化是后续优化算法（如 VertexAlignment、AuxLineSpacing）的性能基础。

