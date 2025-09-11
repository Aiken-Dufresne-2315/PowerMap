#include <iostream>
using namespace std;

class Person {
public:
    string name;
    int age;
    
    Person(string n, int a) : name(n), age(a) {}
};

// 使用指针的Edge类 - 拷贝赋值操作符可以正常工作
class EdgeWithPointer {
private:
    Person* start;  // 指针成员
    Person* end;    // 指针成员
    
public:
    EdgeWithPointer(Person* s, Person* e) : start(s), end(e) {
        cout << "指针版边: " << start->name << " -> " << end->name << endl;
    }
    
    // 拷贝赋值操作符 - 指针可以重新赋值
    EdgeWithPointer& operator=(const EdgeWithPointer& other) {
        if (this != &other) {
            cout << "重新分配指针..." << endl;
            start = other.start;  // 正常！指针可以指向不同对象
            end = other.end;      // 正常！指针可以指向不同对象
        }
        return *this;
    }
    
    void printEdge() {
        cout << "指针边: " << start->name << " -> " << end->name << endl;
    }
};

// 使用引用的Edge类 - 拷贝赋值操作符有问题
class EdgeWithReference {
private:
    Person& start;  // 引用成员
    Person& end;    // 引用成员
    
public:
    EdgeWithReference(Person& s, Person& e) : start(s), end(e) {
        cout << "引用版边: " << start.name << " -> " << end.name << endl;
    }
    
    // 如果我们试图实现拷贝赋值操作符...
    EdgeWithReference& operator=(const EdgeWithReference& other) {
        if (this != &other) {
            // 问题1: 无法重新绑定引用
            // start = other.start;  // 这不是重新绑定！
            // 实际上这等同于: start.operator=(other.start)
            // 即把 other.start 的内容复制给 start 引用的对象
            
            // 问题2: 如果我们写成这样：
            start = other.start;  // 实际上修改了原始Person对象！
            end = other.end;      // 实际上修改了原始Person对象！
        }
        return *this;
    }
    
    void printEdge() {
        cout << "引用边: " << start.name << " -> " << end.name << endl;
    }
};

int main() {
    cout << "=== 创建测试对象 ===" << endl;
    Person alice("Alice", 25);
    Person bob("Bob", 30);
    Person charlie("Charlie", 35);
    Person diana("Diana", 28);
    
    cout << "\n=== 指针版本的行为 ===" << endl;
    EdgeWithPointer pEdge1(&alice, &bob);
    EdgeWithPointer pEdge2(&charlie, &diana);
    
    cout << "赋值前:" << endl;
    pEdge1.printEdge();
    pEdge2.printEdge();
    
    pEdge1 = pEdge2;  // 指针可以重新指向不同对象
    
    cout << "赋值后:" << endl;
    pEdge1.printEdge();  // 现在指向charlie和diana
    pEdge2.printEdge();
    
    cout << "\n=== 引用版本的问题 ===" << endl;
    EdgeWithReference rEdge1(alice, bob);
    EdgeWithReference rEdge2(charlie, diana);
    
    cout << "赋值前的Person对象:" << endl;
    cout << "alice: " << alice.name << ", bob: " << bob.name << endl;
    cout << "charlie: " << charlie.name << ", diana: " << diana.name << endl;
    
    cout << "边的状态:" << endl;
    rEdge1.printEdge();
    rEdge2.printEdge();
    
    cout << "\n执行引用版本的赋值操作..." << endl;
    rEdge1 = rEdge2;  // 这会修改原始对象的内容！
    
    cout << "赋值后的Person对象:" << endl;
    cout << "alice: " << alice.name << ", bob: " << bob.name << endl;
    cout << "charlie: " << charlie.name << ", diana: " << diana.name << endl;
    
    cout << "边的状态:" << endl;
    rEdge1.printEdge();  // 仍然"连接"alice和bob，但它们的内容被改变了！
    rEdge2.printEdge();
    
    return 0;
}
