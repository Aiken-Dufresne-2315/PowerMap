#include <iostream>
using namespace std;

class Person {
public:
    string name;
    int age;
    
    Person(string n, int a) : name(n), age(a) {}
};

// 问题类：包含引用成员
class Edge {
private:
    Person& start;  // 引用成员
    Person& end;    // 引用成员
    
public:
    // 构造函数 - 正常工作
    Edge(Person& s, Person& e) : start(s), end(e) {
        cout << "创建边: " << start.name << " -> " << end.name << endl;
    }
    
    // 拷贝构造函数 - 正常工作
    Edge(const Edge& other) : start(other.start), end(other.end) {
        cout << "拷贝构造边: " << start.name << " -> " << end.name << endl;
    }
    
    // 拷贝赋值操作符 - 这里有问题！
    Edge& operator=(const Edge& other) {
        if (this != &other) {
            // 错误！无法重新赋值引用
            // start = other.start;  // 编译错误！
            // end = other.end;      // 编译错误！
            
            // 实际上这样写会被理解为：
            // start.operator=(other.start); // 调用Person的赋值操作符
            // 这不是重新绑定引用，而是修改引用所指向对象的内容
        }
        return *this;
    }
    
    void printEdge() {
        cout << "边: " << start.name << "(" << start.age << ") -> " 
             << end.name << "(" << end.age << ")" << endl;
    }
};

int main() {
    // 创建一些Person对象
    Person alice("Alice", 25);
    Person bob("Bob", 30);
    Person charlie("Charlie", 35);
    Person diana("Diana", 28);
    
    cout << "=== 引用的基本行为 ===" << endl;
    
    // 普通引用示例
    Person& ref1 = alice;  // ref1 引用 alice
    cout << "ref1 指向: " << ref1.name << endl;
    
    // 引用无法重新绑定到其他对象
    // Person& ref1 = bob;  // 错误！不能重新声明
    // ref1 = bob;          // 这不是重新绑定，而是将bob的内容复制给alice
    
    cout << "\n=== 演示引用赋值的实际含义 ===" << endl;
    cout << "赋值前 - alice: " << alice.name << ", bob: " << bob.name << endl;
    
    ref1 = bob;  // 这实际上是 alice = bob，即把bob的值复制给alice
    
    cout << "赋值后 - alice: " << alice.name << ", bob: " << bob.name << endl;
    cout << "ref1 仍然指向alice: " << ref1.name << endl;
    
    cout << "\n=== Edge类的问题 ===" << endl;
    
    // 创建两条边
    Edge edge1(alice, bob);
    Edge edge2(charlie, diana);
    
    edge1.printEdge();
    edge2.printEdge();
    
    cout << "\n尝试赋值操作..." << endl;
    // edge1 = edge2;  // 如果没有删除赋值操作符，这会有意想不到的行为
    
    return 0;
}