# Bug修复记录：引用包装器与容器的深拷贝问题

## 日期
2025年10月10日

## 问题背景
在 `CheckOverlap.cpp` 中实现 `checkOverlapAfterMove` 函数时，需要创建临时的边对象来检测顶点移动后是否会产生重叠。这些临时边对象需要使用修改后的顶点坐标，但不能影响原图中的顶点。

---

## Bug #1: 引用包装器导致的错误共享

### 问题表现
在循环中创建多条边后，输出所有边的端点信息时发现：
- **预期**：不同的边应该有不同的端点坐标
- **实际**：所有边的端点信息都相同，都是最后一次迭代的值

```cpp
// 循环中输出（正确）
Inner second check.
59: (577.667, 1041.4)    // 第一条边
60: (577.667, 1186)

Inner second check.
59: (577.667, 1041.4)    // 第二条边  
60: (577.667, 1186)

// 循环结束后输出（错误！）
59: (577.667, 1041.4)    // 第一条边 - 错误，应该是58
1980823552: (...)        // 垃圾值

59: (577.667, 1041.4)    // 第二条边 - 正确
60: (577.667, 1186)
```

### 问题原因

#### 1. `BaseEdgeProperty` 使用引用包装器存储顶点

查看 `BaseEdgeProperty.h` 第30-31行：

```cpp
std::reference_wrapper<BaseVertexProperty>  source;
std::reference_wrapper<BaseVertexProperty>  target;
```

**关键点**：`BaseEdgeProperty` 存储的是顶点的**引用**，而不是顶点的**副本**。

#### 2. 循环中局部变量的内存重用

原始错误代码：

```cpp
std::vector<BaseEdgeProperty> newOutEdges;  
for(BaseUGraphProperty::out_edge_iterator oeit = oep.first; oeit != oep.second; ++oeit) {
    // 创建局部变量
    BaseVertexProperty tempSource = graph[*oeit].Source();
    BaseVertexProperty tempTarget = graph[*oeit].Target();
    
    // 修改坐标...
    if (tempSource.getID() == vertexID) {
        tempSource.setCoord(newPos);
    } else {
        tempTarget.setCoord(newPos);
    }
    
    // 用局部变量创建边
    BaseEdgeProperty tempEdge(tempSource, tempTarget, ...);
    newOutEdges.push_back(tempEdge);
}
```

**问题分析**：
1. 每次循环迭代，`tempSource` 和 `tempTarget` 是栈上的局部变量
2. 编译器可能在每次迭代中**重用相同的栈地址**
3. `BaseEdgeProperty` 构造函数通过 `reference_wrapper` 存储对这些局部变量的引用
4. 循环结束后，所有 `BaseEdgeProperty` 对象的 `source` 和 `target` 都指向**相同的内存地址**
5. 该地址上的内容是最后一次迭代的值

### 第一次尝试的解决方案

使用 `vector` 持久化存储顶点副本：

```cpp
std::vector<BaseVertexProperty> vertexStorage;
std::vector<BaseEdgeProperty> newOutEdges;  

for(...) {
    BaseVertexProperty tempSource = graph[*oeit].Source();
    BaseVertexProperty tempTarget = graph[*oeit].Target();
    
    // 修改坐标...
    
    // 存储到vector中
    vertexStorage.push_back(tempSource);
    vertexStorage.push_back(tempTarget);
    
    // 使用vector中持久化的元素创建引用
    BaseEdgeProperty tempEdge(vertexStorage[vertexStorage.size()-2], 
                             vertexStorage[vertexStorage.size()-1], 
                             ...);
    newOutEdges.push_back(tempEdge);
}
```

---

## Bug #2: Vector扩容导致引用失效

### 问题表现
使用上述 `vector` 方案后，输出仍然出现问题：
- 某些顶点ID变成了**垃圾值**（如 `858397504`），而不是预期的值（如 `58`）

```
Glory is mine!
858397504: (577.667, 1009)    // 应该是 58，却是垃圾值
59: (577.667, 1041.4)

59: (577.667, 1041.4)
60: (577.667, 1186)
```

### 问题原因

#### `std::vector` 的扩容机制

当 `vector` 需要扩容时（例如从容量2增长到4）：

1. **分配新的更大的内存块**
2. **将旧元素移动/复制到新位置**
3. **释放旧内存**

**关键问题**：
- 在 `push_back` 过程中，如果触发扩容，所有已存储元素的内存地址会改变
- 但之前创建的 `BaseEdgeProperty` 对象中的 `reference_wrapper` 仍然指向**旧的（已释放的）内存地址**
- 访问这些引用时就会得到**未定义行为**，表现为垃圾值

#### 示例场景

```cpp
std::vector<BaseVertexProperty> vertexStorage;  // 初始容量可能是0或1

// 第一次迭代
vertexStorage.push_back(vertex1);  // 地址：0x1000
vertexStorage.push_back(vertex2);  // 地址：0x1008
BaseEdgeProperty edge1(vertexStorage[0], vertexStorage[1], ...);
// edge1 的 source 引用地址 0x1000

// 第二次迭代
vertexStorage.push_back(vertex3);  // 触发扩容！
// 扩容后：vertex1 移动到 0x2000，vertex2 移动到 0x2008，vertex3 在 0x2010
// 但 edge1.source 仍然引用 0x1000（已释放的内存）

vertexStorage.push_back(vertex4);
BaseEdgeProperty edge2(vertexStorage[2], vertexStorage[3], ...);
// edge2 正常，因为创建时引用的是新地址

// 访问 edge1.Source() 时，读取 0x1000 的内容 -> 垃圾值！
```

### 最终解决方案

使用 `std::deque` 替代 `std::vector`：

```cpp
#include <deque>

// ...

// 使用 deque 而非 vector，避免扩容时引用失效
std::deque<BaseVertexProperty> vertexStorage;
std::vector<BaseEdgeProperty> newOutEdges;  

for(BaseUGraphProperty::out_edge_iterator oeit = oep.first; oeit != oep.second; ++oeit) {
    BaseVertexProperty tempSource = graph[*oeit].Source();
    BaseVertexProperty tempTarget = graph[*oeit].Target();
    
    if (tempSource.getID() == vertexID) {
        tempSource.setCoord(newPos);
    } else {
        tempTarget.setCoord(newPos);
    }
    
    // 存储到 deque 中（关键：deque 不会使已有元素的引用失效）
    vertexStorage.push_back(tempSource);
    vertexStorage.push_back(tempTarget);
    
    BaseEdgeProperty tempEdge(vertexStorage[vertexStorage.size()-2], 
                             vertexStorage[vertexStorage.size()-1], 
                             graph[*oeit].ID(), 
                             graph[*oeit].Angle());
    newOutEdges.push_back(tempEdge);
}
```

### 为什么 `deque` 可以解决问题

#### `std::deque` 的内存模型

- `deque` 内部使用**多个不连续的内存块**存储元素
- 添加新元素时，只需要分配新的内存块，**不会移动已有元素**
- 因此，**已有元素的地址保持不变**，引用和指针保持有效

#### `std::vector` vs `std::deque` 引用稳定性对比

| 容器 | 添加元素 | 引用稳定性 | 适用场景 |
|------|----------|------------|----------|
| `std::vector` | `push_back` 可能触发扩容和元素移动 | ❌ 引用可能失效 | 需要连续内存、随机访问性能优先 |
| `std::deque` | 分配新内存块，不移动已有元素 | ✅ 引用保持有效 | 需要引用稳定性、两端插入 |

---

## 核心教训

### 1. 理解引用包装器的语义
`std::reference_wrapper` 存储的是**引用**，不是**副本**。如果被引用的对象被销毁或移动，引用会失效。

### 2. 注意容器与引用的交互
当容器可能会重新分配内存时（如 `vector` 的 `push_back`），存储在其他地方的引用/指针会失效。

### 3. 选择合适的容器
- 需要引用稳定性 → `std::deque`、`std::list`
- 需要连续内存和高性能随机访问 → `std::vector`（但注意引用失效问题）

### 4. 对象生命周期管理
在使用引用（或指针）时，必须确保被引用对象的生命周期**长于**引用的使用期。

---

## 代码对比

### 错误代码（Bug #1）

```cpp
std::vector<BaseEdgeProperty> newOutEdges;  
for(...) {
    BaseVertexProperty tempSource = ...;  // 局部变量
    BaseVertexProperty tempTarget = ...;  // 局部变量
    
    BaseEdgeProperty tempEdge(tempSource, tempTarget, ...);  // 引用局部变量
    newOutEdges.push_back(tempEdge);
}
// tempSource 和 tempTarget 被销毁，所有边的引用都失效
```

### 部分正确但有隐患的代码（Bug #2）

```cpp
std::vector<BaseVertexProperty> vertexStorage;  // ❌ vector 扩容会使引用失效
std::vector<BaseEdgeProperty> newOutEdges;  
for(...) {
    BaseVertexProperty tempSource = ...;
    BaseVertexProperty tempTarget = ...;
    
    vertexStorage.push_back(tempSource);
    vertexStorage.push_back(tempTarget);  // 可能触发扩容！
    
    BaseEdgeProperty tempEdge(
        vertexStorage[vertexStorage.size()-2],  // 引用可能在下次扩容时失效
        vertexStorage[vertexStorage.size()-1], 
        ...
    );
    newOutEdges.push_back(tempEdge);
}
```

### 正确代码（最终方案）

```cpp
std::deque<BaseVertexProperty> vertexStorage;  // ✅ deque 不会使引用失效
std::vector<BaseEdgeProperty> newOutEdges;  
for(...) {
    BaseVertexProperty tempSource = ...;
    BaseVertexProperty tempTarget = ...;
    
    vertexStorage.push_back(tempSource);
    vertexStorage.push_back(tempTarget);  // 引用保持有效
    
    BaseEdgeProperty tempEdge(
        vertexStorage[vertexStorage.size()-2],  // 引用始终有效
        vertexStorage[vertexStorage.size()-1], 
        ...
    );
    newOutEdges.push_back(tempEdge);
}
```

---

## 相关文件

- `src/CheckOverlap.cpp`：修复位置（第105-141行）
- `include/BaseEdgeProperty.h`：引用包装器定义（第30-31行）

## 参考资料

- C++ Reference: [`std::reference_wrapper`](https://en.cppreference.com/w/cpp/utility/functional/reference_wrapper)
- C++ Reference: [`std::deque`](https://en.cppreference.com/w/cpp/container/deque)
- C++ Reference: [Iterator invalidation rules](https://en.cppreference.com/w/cpp/container)



