/**
 * @file spatial_grid_usage.cpp
 * @brief SpatialGrid 使用示例
 * 
 * 展示如何在实际优化算法中使用空间网格加速重叠检查
 */

#include "CheckOverlap.h"
#include "SpatialGrid.h"
#include "MapFileReader.h"
#include "PowerMap.h"
#include <iostream>

using namespace Map;

// 示例1：基本使用 - 单次检查
void example1_basic_usage() {
    std::cout << "========================================" << std::endl;
    std::cout << "示例1：基本使用 - 单次重叠检查" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // 加载地图
    MapFileReader reader("input/test5.txt");
    reader.read();
    BaseUGraphProperty graph = reader.getUGraph();
    
    // 选择一个顶点进行测试
    int testVertexID = 1;
    Coord2 newPos(10.0, 5.0);
    
    std::cout << "检查顶点 " << testVertexID 
              << " 移动到 (" << newPos.x() << ", " << newPos.y() << ")" << std::endl;
    
    // 方法1：使用原始实现
    bool overlap1 = overlapHappens(testVertexID, newPos, graph);
    std::cout << "原始实现结果: " << (overlap1 ? "有重叠" : "无重叠") << std::endl;
    
    // 方法2：使用优化实现（自动创建网格）
    bool overlap2 = overlapHappensOptimized(testVertexID, newPos, graph);
    std::cout << "优化实现结果: " << (overlap2 ? "有重叠" : "无重叠") << std::endl;
    
    std::cout << std::endl;
}

// 示例2：批量检查 - 显式管理网格
void example2_batch_checks() {
    std::cout << "========================================" << std::endl;
    std::cout << "示例2：批量检查 - 显式管理空间网格" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // 加载地图
    MapFileReader reader("input/test5.txt");
    reader.read();
    BaseUGraphProperty graph = reader.getUGraph();
    
    // 创建空间网格（推荐：cellSize = 1-2倍平均边长）
    SpatialGrid spatialGrid(2.0);
    spatialGrid.buildFromGraph(graph);
    
    std::cout << "空间网格已构建，单元大小: " << spatialGrid.getCellSize() << std::endl;
    
    // 测试多个候选位置
    int testVertexID = 1;
    std::vector<Coord2> candidatePositions = {
        Coord2(5.0, 5.0),
        Coord2(10.0, 10.0),
        Coord2(15.0, 5.0),
        Coord2(8.0, 12.0)
    };
    
    std::cout << "\n测试 " << candidatePositions.size() << " 个候选位置:" << std::endl;
    
    for (size_t i = 0; i < candidatePositions.size(); ++i) {
        const auto& pos = candidatePositions[i];
        bool hasOverlap = overlapHappensOptimized(testVertexID, pos, graph, &spatialGrid);
        
        std::cout << "  位置 " << i + 1 << " (" << pos.x() << ", " << pos.y() << "): "
                  << (hasOverlap ? "❌ 有重叠" : "✓ 无重叠") << std::endl;
    }
    
    std::cout << std::endl;
}

// 示例3：模拟优化迭代
void example3_optimization_loop() {
    std::cout << "========================================" << std::endl;
    std::cout << "示例3：模拟优化迭代过程" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // 加载地图
    MapFileReader reader("input/test5.txt");
    reader.read();
    BaseUGraphProperty graph = reader.getUGraph();
    
    // 创建空间网格
    SpatialGrid spatialGrid(2.0);
    spatialGrid.buildFromGraph(graph);
    
    // 模拟优化算法的迭代过程
    int maxIterations = 5;
    int testVertexID = 2;
    
    std::cout << "模拟优化顶点 " << testVertexID << " 的位置" << std::endl;
    std::cout << "最大迭代次数: " << maxIterations << std::endl << std::endl;
    
    // 获取初始位置
    BaseUGraphProperty::vertex_descriptor vd = getVertexDescriptor(testVertexID);
    Coord2 currentPos = graph[vd].getCoord();
    
    std::cout << "初始位置: (" << currentPos.x() << ", " << currentPos.y() << ")" << std::endl;
    
    for (int iter = 0; iter < maxIterations; ++iter) {
        std::cout << "\n--- 迭代 " << iter + 1 << " ---" << std::endl;
        
        // 生成候选新位置（这里简单地随机移动）
        double dx = (rand() % 3 - 1) * 0.5;  // -0.5, 0, 0.5
        double dy = (rand() % 3 - 1) * 0.5;
        Coord2 newPos(currentPos.x() + dx, currentPos.y() + dy);
        
        std::cout << "尝试新位置: (" << newPos.x() << ", " << newPos.y() << ")" << std::endl;
        
        // 使用优化的重叠检查
        bool hasOverlap = overlapHappensOptimized(testVertexID, newPos, graph, &spatialGrid);
        
        if (!hasOverlap) {
            std::cout << "✓ 无重叠，接受新位置" << std::endl;
            
            // 更新顶点位置
            graph[vd].setCoord(newPos);
            currentPos = newPos;
            
            // 重要：位置变化后重建空间网格
            spatialGrid.clear();
            spatialGrid.buildFromGraph(graph);
            std::cout << "  空间网格已更新" << std::endl;
        } else {
            std::cout << "❌ 有重叠，拒绝新位置" << std::endl;
        }
    }
    
    std::cout << "\n最终位置: (" << currentPos.x() << ", " << currentPos.y() << ")" << std::endl;
    std::cout << std::endl;
}

// 示例4：空间查询功能
void example4_spatial_queries() {
    std::cout << "========================================" << std::endl;
    std::cout << "示例4：空间查询功能展示" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // 加载地图
    MapFileReader reader("input/test5.txt");
    reader.read();
    BaseUGraphProperty graph = reader.getUGraph();
    
    // 创建空间网格
    SpatialGrid spatialGrid(2.0);
    spatialGrid.buildFromGraph(graph);
    
    // 查询点
    Coord2 queryPos(10.0, 10.0);
    
    std::cout << "查询位置: (" << queryPos.x() << ", " << queryPos.y() << ")" << std::endl << std::endl;
    
    // 1. 查询附近的顶点
    std::vector<int> nearbyVertices = spatialGrid.getNearbyVertices(queryPos, 1);
    std::cout << "附近的顶点 (半径=1格):" << std::endl;
    std::cout << "  找到 " << nearbyVertices.size() << " 个顶点: ";
    for (int vid : nearbyVertices) {
        std::cout << vid << " ";
    }
    std::cout << std::endl << std::endl;
    
    // 2. 查询附近的边
    std::vector<int> nearbyEdges = spatialGrid.getNearbyEdges(queryPos, 1);
    std::cout << "附近的边 (半径=1格):" << std::endl;
    std::cout << "  找到 " << nearbyEdges.size() << " 条边: ";
    for (int eid : nearbyEdges) {
        std::cout << eid << " ";
    }
    std::cout << std::endl << std::endl;
    
    // 3. 查询线段路径上的元素
    Coord2 lineStart(5.0, 5.0);
    Coord2 lineEnd(15.0, 15.0);
    
    std::cout << "线段查询: (" << lineStart.x() << "," << lineStart.y() << ") -> ("
              << lineEnd.x() << "," << lineEnd.y() << ")" << std::endl;
    
    std::vector<int> verticesAlongLine = spatialGrid.getVerticesAlongLine(lineStart, lineEnd);
    std::cout << "  线段路径上的顶点: " << verticesAlongLine.size() << " 个" << std::endl;
    
    std::vector<int> edgesAlongLine = spatialGrid.getEdgesAlongLine(lineStart, lineEnd);
    std::cout << "  线段路径上的边: " << edgesAlongLine.size() << " 条" << std::endl;
    
    std::cout << std::endl;
}

int main() {
    std::cout << "空间网格优化 - 使用示例集" << std::endl;
    std::cout << "======================================" << std::endl << std::endl;
    
    try {
        example1_basic_usage();
        example2_batch_checks();
        example3_optimization_loop();
        example4_spatial_queries();
        
        std::cout << "所有示例运行完成！" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}


