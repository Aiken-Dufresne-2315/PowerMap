/**
 * @file overlap_check_benchmark.cpp
 * @brief 重叠检查性能基准测试
 * 
 * 对比原始实现和空间网格优化实现的性能差异
 */

#include "CheckOverlap.h"
#include "SpatialGrid.h"
#include "MapFileReader.h"
#include "PowerMap.h"
#include <chrono>
#include <iostream>
#include <iomanip>
#include <random>

using namespace Map;

// 性能计时器
class Timer {
private:
    std::chrono::high_resolution_clock::time_point m_start;
    
public:
    Timer() : m_start(std::chrono::high_resolution_clock::now()) {}
    
    double elapsed_ms() const {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(end - m_start).count();
    }
    
    void reset() {
        m_start = std::chrono::high_resolution_clock::now();
    }
};

// 生成随机测试位置
std::vector<Coord2> generateRandomPositions(const BaseUGraphProperty& graph, int count) {
    std::vector<Coord2> positions;
    std::random_device rd;
    std::mt19937 gen(rd());
    
    // 计算地图边界
    auto vp = boost::vertices(graph);
    double minX = 1e9, maxX = -1e9, minY = 1e9, maxY = -1e9;
    
    for (auto vit = vp.first; vit != vp.second; ++vit) {
        double x = graph[*vit].getCoord().x();
        double y = graph[*vit].getCoord().y();
        minX = std::min(minX, x);
        maxX = std::max(maxX, x);
        minY = std::min(minY, y);
        maxY = std::max(maxY, y);
    }
    
    std::uniform_real_distribution<> distX(minX, maxX);
    std::uniform_real_distribution<> distY(minY, maxY);
    
    for (int i = 0; i < count; ++i) {
        positions.emplace_back(distX(gen), distY(gen));
    }
    
    return positions;
}

// 计算平均边长
double computeAverageEdgeLength(const BaseUGraphProperty& graph) {
    auto ep = boost::edges(graph);
    double totalLength = 0.0;
    int edgeCount = 0;
    
    for (auto eit = ep.first; eit != ep.second; ++eit) {
        const auto& edge = graph[*eit];
        double dx = edge.Source().getCoord().x() - edge.Target().getCoord().x();
        double dy = edge.Source().getCoord().y() - edge.Target().getCoord().y();
        totalLength += std::sqrt(dx * dx + dy * dy);
        edgeCount++;
    }
    
    return edgeCount > 0 ? totalLength / edgeCount : 1.0;
}

// 基准测试
void runBenchmark(const std::string& inputFile, int testCount = 100) {
    std::cout << "========================================" << std::endl;
    std::cout << "重叠检查性能基准测试" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "输入文件: " << inputFile << std::endl;
    std::cout << "测试次数: " << testCount << std::endl << std::endl;
    
    // 加载地图
    MapFileReader reader(inputFile);
    reader.read();
    
    BaseUGraphProperty graph = reader.getUGraph();
    int vertexCount = boost::num_vertices(graph);
    int edgeCount = boost::num_edges(graph);
    
    std::cout << "地图规模:" << std::endl;
    std::cout << "  顶点数: " << vertexCount << std::endl;
    std::cout << "  边数:   " << edgeCount << std::endl;
    
    double avgEdgeLength = computeAverageEdgeLength(graph);
    std::cout << "  平均边长: " << std::fixed << std::setprecision(2) 
              << avgEdgeLength << std::endl << std::endl;
    
    // 选择一个测试顶点（度数较大的）
    int testVertexID = 0;
    int maxDegree = 0;
    auto vp = boost::vertices(graph);
    
    for (auto vit = vp.first; vit != vp.second; ++vit) {
        int degree = boost::out_degree(*vit, graph);
        if (degree > maxDegree) {
            maxDegree = degree;
            testVertexID = graph[*vit].getID();
        }
    }
    
    std::cout << "测试顶点: ID=" << testVertexID << ", 度数=" << maxDegree << std::endl << std::endl;
    
    // 生成随机测试位置
    std::vector<Coord2> testPositions = generateRandomPositions(graph, testCount);
    
    // ======== 测试1: 原始实现 ========
    std::cout << "[1] 测试原始实现 (overlapHappens)..." << std::endl;
    
    Timer timer;
    int overlapCount1 = 0;
    
    for (const auto& pos : testPositions) {
        if (overlapHappens(testVertexID, pos, graph)) {
            overlapCount1++;
        }
    }
    
    double time1 = timer.elapsed_ms();
    double avgTime1 = time1 / testCount;
    
    std::cout << "  总耗时: " << std::fixed << std::setprecision(2) 
              << time1 << " ms" << std::endl;
    std::cout << "  平均耗时: " << avgTime1 << " ms/次" << std::endl;
    std::cout << "  重叠数量: " << overlapCount1 << std::endl << std::endl;
    
    // ======== 测试2: 优化实现（每次重建网格） ========
    std::cout << "[2] 测试优化实现 - 自动模式 (每次重建)..." << std::endl;
    
    timer.reset();
    int overlapCount2 = 0;
    
    for (const auto& pos : testPositions) {
        if (overlapHappensOptimized(testVertexID, pos, graph)) {
            overlapCount2++;
        }
    }
    
    double time2 = timer.elapsed_ms();
    double avgTime2 = time2 / testCount;
    
    std::cout << "  总耗时: " << time2 << " ms" << std::endl;
    std::cout << "  平均耗时: " << avgTime2 << " ms/次" << std::endl;
    std::cout << "  重叠数量: " << overlapCount2 << std::endl;
    std::cout << "  加速比: " << std::fixed << std::setprecision(1) 
              << time1 / time2 << "x" << std::endl << std::endl;
    
    // ======== 测试3: 优化实现（重用网格） ========
    std::cout << "[3] 测试优化实现 - 显式管理 (重用网格)..." << std::endl;
    
    // 构建空间网格
    double cellSize = avgEdgeLength * 1.5;
    SpatialGrid spatialGrid(cellSize);
    
    Timer buildTimer;
    spatialGrid.buildFromGraph(graph);
    double buildTime = buildTimer.elapsed_ms();
    
    std::cout << "  网格构建耗时: " << buildTime << " ms" << std::endl;
    std::cout << "  网格单元大小: " << cellSize << std::endl;
    
    timer.reset();
    int overlapCount3 = 0;
    
    for (const auto& pos : testPositions) {
        if (overlapHappensOptimized(testVertexID, pos, graph, &spatialGrid)) {
            overlapCount3++;
        }
    }
    
    double time3 = timer.elapsed_ms();
    double avgTime3 = time3 / testCount;
    
    std::cout << "  查询总耗时: " << time3 << " ms" << std::endl;
    std::cout << "  平均耗时: " << avgTime3 << " ms/次" << std::endl;
    std::cout << "  重叠数量: " << overlapCount3 << std::endl;
    std::cout << "  加速比: " << time1 / time3 << "x" << std::endl << std::endl;
    
    // ======== 结果汇总 ========
    std::cout << "========================================" << std::endl;
    std::cout << "性能对比总结" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::left << std::setw(25) << "方法" 
              << std::right << std::setw(12) << "总耗时(ms)" 
              << std::setw(15) << "平均耗时(ms)" 
              << std::setw(10) << "加速比" << std::endl;
    std::cout << std::string(62, '-') << std::endl;
    
    std::cout << std::left << std::setw(25) << "原始实现" 
              << std::right << std::setw(12) << std::fixed << std::setprecision(2) << time1
              << std::setw(15) << avgTime1
              << std::setw(10) << "1.0x" << std::endl;
              
    std::cout << std::left << std::setw(25) << "优化实现(自动)" 
              << std::right << std::setw(12) << time2
              << std::setw(15) << avgTime2
              << std::setw(10) << std::setprecision(1) << (time1/time2) << "x" << std::endl;
              
    std::cout << std::left << std::setw(25) << "优化实现(重用网格)" 
              << std::right << std::setw(12) << std::setprecision(2) << time3
              << std::setw(15) << avgTime3
              << std::setw(10) << std::setprecision(1) << (time1/time3) << "x" << std::endl;
    
    std::cout << std::string(62, '-') << std::endl;
    
    // 验证正确性
    if (overlapCount1 == overlapCount2 && overlapCount2 == overlapCount3) {
        std::cout << "✓ 正确性验证通过：所有方法结果一致" << std::endl;
    } else {
        std::cout << "✗ 警告：不同方法的结果不一致！" << std::endl;
        std::cout << "  原始: " << overlapCount1 
                  << ", 自动: " << overlapCount2 
                  << ", 重用: " << overlapCount3 << std::endl;
    }
    
    std::cout << std::endl;
}

int main(int argc, char* argv[]) {
    std::string inputFile = "input/test10.txt";
    int testCount = 100;
    
    if (argc >= 2) {
        inputFile = argv[1];
    }
    if (argc >= 3) {
        testCount = std::stoi(argv[2]);
    }
    
    try {
        runBenchmark(inputFile, testCount);
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}


