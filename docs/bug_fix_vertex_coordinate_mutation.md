# Bug修复记录：顶点坐标意外被修改问题

## 日期
2025年10月10日

## 问题背景
在 `CheckOverlap.cpp` 中的 `overlapHappens` 函数中，需要检测当一个顶点移动到新位置后是否会产生重叠。为此，代码尝试创建临时的边对象，这些边对象的端点应该使用新的坐标，但**不应该修改原图中的顶点**。

---

## Bug表现

### 症状描述
在调试输出中发现，**同一个顶点在同一个函数内前后输出的坐标不一致**：

```
ATTENTION
58: (577.667, 1009)          <-- 第一次输出顶点58的坐标
577.667 1041.4               
57: (626.042, 958)
58: (577.667, 1041.4)        <-- 第二次输出顶点58的坐标（已被修改！）
```

### 问题关键
- **第一次输出**：顶点58的坐标是 `(577.667, 1009)`
- **第二次输出**：顶点58的坐标变成了 `(577.667, 1041.4)`
- **时间间隔**：两次输出之间只隔了几行代码，没有任何显式修改顶点58的操作
- **疑问**：为什么图中真实的顶点坐标会被意外修改？

---

## 问题原因分析

### 1. BaseEdgeProperty的设计：使用引用包装器

查看 `include/BaseEdgeProperty.h` 第30-31行：

```cpp
protected:
    std::reference_wrapper<BaseVertexProperty>  source;
    std::reference_wrapper<BaseVertexProperty>  target;
```

**关键设计**：
- `BaseEdgeProperty` 使用 `std::reference_wrapper` 存储顶点
- 这意味着边对象存储的是顶点的**引用**，而不是顶点的**副本**
- 这样设计是为了节省内存并确保边始终指向图中的真实顶点

### 2. 拷贝构造函数：浅拷贝引用

查看 `include/BaseEdgeProperty.h` 第71-76行：

```cpp
// copy constructor
BaseEdgeProperty( 
    const BaseEdgeProperty& e 
): source(e.source), target(e.target),   // <-- 只拷贝了引用！
id(e.id), angle(e.angle), weight(e.weight), 
visited(e.visited), visitedTimes(e.visitedTimes),
close2H(e.close2H), close2V(e.close2V), oriented2H(e.oriented2H), oriented2V(e.oriented2V) {}
```

**问题所在**：
- 拷贝构造函数执行的是**浅拷贝**
- `source(e.source)` 和 `target(e.target)` 只是拷贝了 `std::reference_wrapper`
- 拷贝后的边对象仍然引用**同一个顶点对象**

### 3. 问题代码：试图创建临时边对象

查看 `src/CheckOverlap.cpp` 原始代码（第101-113行）：

```cpp
std::vector<BaseEdgeProperty> newOutEdges;  
for(BaseUGraphProperty::out_edge_iterator oeit = out_ep.first; oeit != out_ep.second; ++oeit) {
    BaseEdgeProperty tempEdge = graph[*oeit];  // 拷贝边对象
    if (tempEdge.Source().getID() == vertexID) {
        // 试图修改"临时"边的目标顶点坐标
        tempEdge.Target().setCoord(newPos);    // ⚠️ 危险！
        newOutEdges.push_back(tempEdge);
    }
    else {
        tempEdge.Source().setCoord(newPos);    // ⚠️ 危险！
        newOutEdges.push_back(tempEdge);
    }
}
```

**问题分析**：
1. `BaseEdgeProperty tempEdge = graph[*oeit]` 创建了边的**副本**
2. 但由于拷贝构造函数只是浅拷贝引用，`tempEdge` 和 `graph[*oeit]` **共享同一个顶点引用**
3. 调用 `tempEdge.Target().setCoord(newPos)` 时，实际上修改了图中**真实的顶点**！
4. 这导致图的状态被意外改变

### 4. Bug的触发路径

假设某个顶点v正在被移动到位置 `(577.667, 1041.4)`，函数 `overlapHappens(v, newPos, graph)` 被调用：

```
1. 函数开始执行
   ├─ 第127-128行：输出顶点58的坐标 → (577.667, 1009) ✓ 正确
   │
2. 进入第102-113行的循环，处理顶点v的出边
   ├─ 假设某条出边连接 v → 顶点58
   ├─ 执行：tempEdge = graph[edge_v_to_58]
   ├─ tempEdge.source 引用顶点v（正在移动的顶点）
   ├─ tempEdge.target 引用顶点58（图中的真实顶点！）
   │
3. 判断：tempEdge.Source().getID() == v.getID() → true
   ├─ 执行：tempEdge.Target().setCoord(newPos)
   ├─ 等价于：顶点58.setCoord(577.667, 1041.4)
   └─ ⚠️ 图中真实的顶点58被修改了！
   │
4. 继续执行后续检查
   └─ 第137-138行：输出顶点58的坐标 → (577.667, 1041.4) ✗ 已被污染
```

**关键洞察**：
- 代码的**意图**：创建临时边对象进行模拟，不影响原图
- **实际效果**：由于引用共享，修改"临时"对象实际修改了原图
- **后果**：图的状态被破坏，后续的重叠检测基于错误的数据

---

## 解决方案

### 方案1：初始尝试（仍然有问题）

最初的修复尝试：

```cpp
std::vector<BaseEdgeProperty> newOutEdges;  
for(BaseUGraphProperty::out_edge_iterator oeit = out_ep.first; oeit != out_ep.second; ++oeit) {
    // 创建顶点的深拷贝
    BaseVertexProperty tempSource = graph[*oeit].Source();
    BaseVertexProperty tempTarget = graph[*oeit].Target();
    
    if (tempSource.getID() == vertexID) {
        tempSource.setCoord(newPos);
    }
    else {
        tempTarget.setCoord(newPos);
    }
    
    // 用临时顶点创建新边
    BaseEdgeProperty tempEdge(tempSource, tempTarget, 
                              graph[*oeit].ID(), 
                              graph[*oeit].Angle(), 
                              graph[*oeit].Weight());
    newOutEdges.push_back(tempEdge);
}
```

**问题**：这个方案看似正确，但仍有隐患！

### 问题：局部变量生命周期

```cpp
for(...) {
    BaseVertexProperty tempSource = ...;  // 局部变量
    BaseVertexProperty tempTarget = ...;  // 局部变量
    
    BaseEdgeProperty tempEdge(tempSource, tempTarget, ...);  // 边引用局部变量
    newOutEdges.push_back(tempEdge);  // 拷贝边对象，但引用的顶点即将销毁！
}
// tempSource 和 tempTarget 已被销毁
// newOutEdges 中的边引用了已销毁的对象！
```

**悬空引用问题**：
- 循环每次迭代创建局部顶点 `tempSource` 和 `tempTarget`
- 用这些局部顶点创建边 `tempEdge`
- 将边拷贝到 `newOutEdges` 中（浅拷贝引用）
- **循环结束时，局部顶点被销毁，但边仍然引用它们** → 悬空引用！

### 方案2：最终解决方案（正确）

使用持久化存储容器来保证顶点的生命周期：

```cpp
// 使用 deque 存储顶点副本，确保引用不会失效
// （deque 在扩容时不会移动已存在的元素，而 vector 会）
std::deque<BaseVertexProperty> vertexStorage;
std::vector<BaseEdgeProperty> newOutEdges;  

for(BaseUGraphProperty::out_edge_iterator oeit = oep.first; oeit != oep.second; ++oeit) {
    // 创建顶点的深拷贝
    BaseVertexProperty tempSource = graph[*oeit].Source();
    BaseVertexProperty tempTarget = graph[*oeit].Target();

    if (tempSource.getID() == vertexID) {
        tempSource.setCoord(newPos);
    }
    else {
        tempTarget.setCoord(newPos);
    }
    
    // 将顶点存储到持久化容器中
    vertexStorage.push_back(tempSource);
    vertexStorage.push_back(tempTarget);
    
    // 创建边，引用持久化存储中的顶点
    BaseEdgeProperty tempEdge(vertexStorage[vertexStorage.size()-2], 
                             vertexStorage[vertexStorage.size()-1], 
                             graph[*oeit].ID(), 
                             graph[*oeit].Angle());
    newOutEdges.push_back(tempEdge);
}
// vertexStorage 和 newOutEdges 同时存在
// 只要 newOutEdges 使用中，vertexStorage 就保持有效
```

**关键改进**：

1. **持久化存储**：
   - 使用 `std::deque<BaseVertexProperty>` 存储所有顶点副本
   - `deque` 在函数结束前一直存在，保证引用有效

2. **为什么用deque而不是vector**：
   - `std::vector` 在扩容时会**移动所有元素到新内存**，导致引用失效
   - `std::deque` 扩容时**不移动已存在的元素**，引用保持有效
   - 或者可以用 `std::vector` 并预先 `reserve` 足够的空间

3. **引用管理**：
   - 边对象引用 `vertexStorage` 中的顶点
   - 只要 `vertexStorage` 存在，这些引用就是有效的

---

## 修复验证

### 修复前的行为
```
ATTENTION
58: (577.667, 1009)          ← 初始值
577.667 1041.4
57: (626.042, 958)
58: (577.667, 1041.4)        ← 被意外修改！
VEOverlap(2) happens
```

### 修复后的预期行为
```
Checking overlap 1...
Checking overlap 2.1...
Checking overlap 2.2...
Checking overlap 3.1...
Checking overlap 3.2...
```
- 顶点坐标在整个检查过程中保持不变
- 图的状态不会被检查函数污染
- 重叠检测基于正确的数据

---

## 经验教训

### 1. 理解引用语义的陷阱

**问题核心**：当类使用 `std::reference_wrapper` 或原始引用作为成员时，拷贝对象并不会创建独立的副本。

```cpp
class Edge {
    std::reference_wrapper<Vertex> target;  // 引用
    
    Edge(const Edge& other) 
        : target(other.target) {}  // 浅拷贝引用，不是深拷贝对象
};

Edge e1 = originalEdge;  // e1 和 originalEdge 共享同一个 Vertex
e1.target.get().setX(100);  // 修改了共享的 Vertex！
```

### 2. 临时对象与引用的生命周期

**危险模式**：

```cpp
for (...) {
    Object temp = ...;           // 局部对象
    Container c(temp);           // 容器引用 temp
    containerList.push_back(c);  // 拷贝容器（引用仍指向 temp）
}  // temp 被销毁 → 悬空引用
```

**安全模式**：

```cpp
std::deque<Object> storage;  // 持久化存储
for (...) {
    Object temp = ...;
    storage.push_back(temp);     // 存储对象副本
    Container c(storage.back()); // 引用持久化的对象
    containerList.push_back(c);
}  // storage 仍然有效
```

### 3. const引用参数的误导

函数签名：
```cpp
bool overlapHappens(int vertexID, const Coord2& newPos, 
                   const BaseUGraphProperty& graph);
```

虽然 `graph` 是 `const` 引用，但这只保证：
- 不能调用 `graph` 的非const成员函数
- 不能修改 `graph` 的数据成员

**但不保证**：
- 通过 `graph[*eit].Target()` 获取的引用可以被修改
- `const` 的传递性在引用包装器中会断裂

### 4. 调试技巧

**发现问题的方法**：
1. 在函数开始和过程中多次输出同一个数据
2. 如果"不可能改变"的数据发生了变化，说明存在意外的引用共享
3. 检查拷贝构造函数是否正确实现深拷贝

**预防措施**：
1. 为使用引用的类添加明确的文档说明
2. 考虑提供深拷贝方法（如 `clone()`）
3. 在需要独立副本时，显式创建并持久化存储

---

## 相关文件

- `include/BaseEdgeProperty.h` - 边属性类定义（使用引用包装器）
- `src/BaseEdgeProperty.cpp` - 边属性类实现
- `src/CheckOverlap.cpp` - 重叠检测函数（问题发生位置）
- `include/BaseVertexProperty.h` - 顶点属性类定义

---

## 总结

这是一个典型的**引用共享导致的状态污染问题**：

1. **设计初衷**：使用引用包装器让边始终指向图中的真实顶点
2. **实现缺陷**：拷贝构造函数只做浅拷贝，导致副本之间共享引用
3. **使用错误**：在需要独立副本的场景中使用了拷贝，导致意外修改
4. **解决方案**：显式创建深拷贝并使用持久化容器管理生命周期

**核心启示**：在C++中处理引用和对象生命周期时必须格外小心，特别是：
- 理解浅拷贝 vs 深拷贝
- 注意局部对象的生命周期
- 容器扩容对引用的影响
- `const` 修饰符的局限性

通过这次bug修复，代码现在能够正确地模拟顶点移动而不污染原图状态，重叠检测逻辑也基于正确的数据运行。

