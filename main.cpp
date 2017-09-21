#include "mainwindow.h"
#include <QApplication>
#include <QtCore>

#include <cstdlib>     /* srand, rand */
#include <ctime>       /* time */
#include <flow.h>
#include <iostream>

#define concurrent(a) std::cout << a << "concurrent {\n"; for(bool m=true; m; m=false, (std::cout << a << "}\n"))
#define pipeline(a) std::cout << a << "pipeline {\n"; for(bool m=true; m; m=false, (std::cout << a << "}\n"))

#include <functional>
#include <windows.h>

class B;
QVector<B*> list;

class B {
public:
    B(std::function<void(void)> f=[](){}) {
        this->destroy = f;
        list << this;
    }
    std::function<void(void)> destroy;

    int test(float a) {
        qDebug() << (uint64_t)(this) << __FUNCTION__;
    }

    ~B() {
        qDebug() << __PRETTY_FUNCTION__;
        destroy();
    }
};

template<typename T>
class A {
public:
    A() {
        qDebug() << "instanciated";
    }

    A(const T &other) {
        value[0] = other;
        qDebug() << "instanciated";
    }

    // Getter
    inline T& get() {
        qDebug() << "read get()";
        return value[0];
    }

    // Getter
    inline T* operator->() {
        qDebug() << "read ->";
        return &value[0];
    }

    // Getter
    inline T& operator()() {
        qDebug() << "read ()";
        return value[0];
    }

    // Getter
    inline operator T&() {
        qDebug() << "read";
        return value[0];
    }

    // Setter
    inline T& set() {
        qDebug() << "write set()";
        return value[0];
    }

    // Setter
    inline A<T>& operator()(T const& newValue) {
        value[0] = newValue;
        qDebug() << "write ()";
        return *this;
    }

    // Setter
    inline A<T>& operator=(const T &newValue) {
        value[0] = newValue;
        qDebug() << "write";
        return *this;
    }

    int f() {
        qDebug() << "constructed " << typeid(T).name();
    }
    int d() {
        qDebug() << "destroyed " << typeid(T).name();
    }

    static T type;
    T value[100];
};


template<typename T>
class AssignAccessor {
public:
    AssignAccessor(A<T> &var) : var(&var) {
        this->var->f();
    }
    ~AssignAccessor() {
        var->d();
    }
private:
    A<T> *var;
};

int sum(int a, int b)
{
    qDebug() << __PRETTY_FUNCTION__;
    B *bl = new B([a,b](){
        qDebug() << "destroy " << a << b;
    });
    bl->test(3);
    if(b < 10) return sum(a, b+1);
    return a+b;
}

int main(int argc, char *argv[])
{


    concurrent("") {
        std::cout << "   cbody\n";
        concurrent("   ") {
            std::cout << "     cbody\n";
        }
        pipeline("   ") {
            std::cout << "     sbody\n";
        }
    }


    srand(time(NULL));
    QApplication::setStyle("Fusion");
    QApplication a(argc, argv);
    MainWindow w;

//    QPriorityQueue<int> l1;
//    QMap<int, char> l2;

//    LARGE_INTEGER frequency;        // ticks per second
//    LARGE_INTEGER t1, t2;           // ticks
//    double elapsedTime;

//    QueryPerformanceFrequency(&frequency);
//    QueryPerformanceCounter(&t1);
//    for(int i=0; i<1000; i++) {
//        l1.push(rand());
//    }
//    for(int i=0; i<1000; i++) {
//        l1.pop();
//    }
//    QueryPerformanceCounter(&t2);
//    elapsedTime = (t2.QuadPart - t1.QuadPart) * 1000.0 / frequency.QuadPart;
//    qDebug() << "QPriorityQueue" << elapsedTime;

//    QueryPerformanceCounter(&t1);
//    for(int i=0; i<1000; i++) {
//        l2[rand()] = 'c';
//    }
//    for(int i=0; i<1000; i++) {
//        l2.remove(l2.firstKey());
//    }
//    QueryPerformanceCounter(&t2);
//    elapsedTime = (t2.QuadPart - t1.QuadPart) * 1000.0 / frequency.QuadPart;
//    qDebug() << "QQueue" << elapsedTime;


//    struct Test {
//        int a, b;
//    };
//    typedef Test atr;

//    Test t = atr{.a=12};
//    qDebug() << t.a;


    struct test {
        int a;
    };

    A<test> t = test{.a=0};
    t.set().a = 4;

    A<std::string> str[3];
    str[0] = "my test";
    qDebug() << str[0]().c_str();

    A<char[20]> str2;
    sprintf(str2.set(), "My test");
    printf(str2);

//    A<int> aa;
//    aa = 12;
//    int b = aa;
//    A<double> bb;
//    float k = (float)(aa = 87);
//    {
//        AssignAccessor<typeof(aa.type)> b1(aa);
//        AssignAccessor<typeof(bb.type)> b2(bb);
//    }

//    A<char> c1;
//    A<long[50]> c2;
//    A<char> *c3 = (A<char> *)(&c2);

//    qDebug() << "((A<uint64_t>*) c1)->type " << typeid(((A<long long int>*) &c1)->type).name() << sizeof(c1);
//    qDebug() << "((A<int>*) c2)->type " << typeid(((A<int>*) &c2)->type).name() << sizeof(c2);


//    class foo {
//    public:
//        foo(int a=0, char b=0) : a(a), b(b) {sum=[](){return 0;};}
//        int a;
//        char b;
//        bool operator ==(const foo &other) {
//            return a == other.a;
//        }
//        QVector<int> list;
//        std::function<int(void)> sum;
//        void setSum() {
//            sum = [this](){
//                return a+b;
//            };
//        }
//    };

//    foo f1, f2;

//    qDebug() << "f1(0,0) == f2(0,0) is" << (f1 == f2);
//    f1.a = 8;
//    qDebug() << "f1(8,0) == f2(0,0) is" << (f1 == f2);
//    f2.a = 8;
//    qDebug() << "f1(8,0) == f2(8,0) is" << (f1 == f2);
//    f1.b = 1;
//    qDebug() << "f1(8,1) == f2(8,0) is" << (f1 == f2);
//    f2.b = 1;
//    qDebug() << "f1(8,1) == f2(8,1) is" << (f1 == f2);
//    qDebug() << f1.sum() << f2.sum();
//    f1.setSum();
//    qDebug() << f1.sum() << f2.sum();
//    f2.setSum();
//    qDebug() << f1.sum() << f2.sum();
//    std::function<int(void)> sum=0;
//    for(int i=0; i<100; i++) {
//        int a = rand();
//        char b = rand();
//        qDebug() << i << a << b << a+b;
//        foo *ff = new foo(a, b);
//        ff->setSum();
//        if(i == 97) sum = ff->sum;
//        delete ff;
//        ff = new foo(19, 85);
//    }
//    qDebug() << sum();

//    qDebug() << sum(1,1) << sum(4,8);
//    for(int i=0; i<list.size(); i++)
//        delete list[i];
//    list.clear();

    {
        int i=0;
        B();
        {
loop:
            B();
        }
        if(i++ == 0) goto loop;
    }
label1:


    QPriorityQueue<int> q;

    int *v = new int(12);
    q.push(*v);
    qDebug() << q.top();
    *v = 15;
    qDebug() << q.top();

    w.show();


    return a.exec();
}
