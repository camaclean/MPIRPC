#include "../manager.hpp"
#include "../parameterstream.hpp"

#include <map>
#include <iostream>
#include <string>

void f1() { std::cout << "Running function f1()" << std::endl; }

int f2() { std::cout << "Running function f2()" << std::endl; return 2; }

double f3(double p) { std::cout << "Running function f3(double)" << std::endl; return p; }

struct Foo
{
    void bar1() { std::cout << "Ran Foo::bar()" << std::endl; }
    std::string bar2() { std::cout << "Ran Foo::bar2()" << std::endl; return std::string("Have a string!"); }
    virtual int bar3(int p, double v) { std::cout << "Ran Foo::bar3(int,double)" << std::endl; val = v; return p; }
    template <typename T1, typename T2>
    void bar4(const std::map<T1,T2>& m) {
        std::cout << "Ran Foo::bar4(std::map)" << std::endl;
        for (auto i : m) {
            std::cout << i.first << " " << i.second << std::endl;
        }
    }
    double val;
};

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);

    mpirpc::Manager* manager = new mpirpc::Manager();

    std::cout << "Running example main. Rank: " << manager->rank() << std::endl;

    bool test = false;
    int test2;
    int procsToGo = manager->numProcs();

    manager->registerType<Foo>();

    mpirpc::FunctionHandle simple_lambda = manager->registerLambda([]() { std::cout << "Running simple lambda" << std::endl; });

    mpirpc::FunctionHandle reply = manager->registerLambda([](int r, const std::string& s) { std::cout << "Rank " << r << " replied with a message: " << s << std::endl; });

    mpirpc::FunctionHandle set_test = manager->registerLambda([&]() { std::cout << "Setting test to true" << std::endl; test = true; return 2.0f; });

    mpirpc::FunctionHandle set_test2 = manager->registerLambda([&](int p) { std::cout << "Setting test2 to " << p << std::endl; test2 = p; manager->invokeFunction(p, reply, manager->rank(), std::string("Hello there!")); });

    mpirpc::FunctionHandle done = manager->registerLambda([&](int r) { --procsToGo; std::cout << "Done on rank " << r << ". Left: " << procsToGo << std::endl; });

    mpirpc::FunctionHandle hf1 = manager->registerFunction<decltype(&f1),&f1>();

    mpirpc::FunctionHandle hf2 = manager->registerFunction<decltype(&f2),&f2>();

    mpirpc::FunctionHandle hf3 = manager->registerFunction<decltype(&f3),&f3>();
    
    mpirpc::FunctionHandle fooBar = manager->registerFunction<decltype(&Foo::bar1),&Foo::bar1>();

    mpirpc::FunctionHandle fooBar2 = manager->registerFunction<decltype(&Foo::bar2),&Foo::bar2>();

    mpirpc::FunctionHandle fooBar3 = manager->registerFunction<decltype(&Foo::bar3),&Foo::bar3>();

    mpirpc::FunctionHandle fooBar4 = manager->registerFunction<decltype(&Foo::bar4<double,std::string>),&Foo::bar4<double,std::string>>();

    Foo *foo = nullptr;
    if (manager->rank() == 0) {
        foo = new Foo();
        foo->val = 7.1;
        manager->registerObject(foo);
        std::cout << "Foo::val is " << foo->val << std::endl;
    }

    manager->sync();

    mpirpc::ObjectWrapperBase *foo_w = manager->getObjectOfType<Foo>();

    std::map<double,std::string> testmap;
    testmap[0.4] = "string: 0.4";
    testmap[3.14] = "3 digit pi";

    if (manager->rank() == 0) {
        manager->invokeFunction(1, simple_lambda);
    }
    else if (manager->rank() == 1)
    {
        manager->invokeFunction(0, simple_lambda);
        float ret1 = manager->invokeFunctionR<float>(0, set_test);
        std::cout << "Rank 1 got " << ret1 << " as a return from set_test lambda" << std::endl;
        manager->invokeFunction(0, set_test2, 1);
        manager->invokeFunction(0, &f1, hf1);
        manager->invokeFunction(0, &f1, 0); //handle lookup
        int ret2 = manager->invokeFunctionR<int>(0, &f2, manager->getFunctionHandle<decltype(&f2),&f2>());
        std::cout << "Rank 1 got " << ret2 << " as a return from f2()" << std::endl;
        double ret3 = manager->invokeFunctionR<double>(0, &f3, 0, 5.3f); // 5.0f should convert to a double
        std::cout << "Rank 1 got " << ret3 << " as a return from f3()" << std::endl;
        manager->invokeFunction(foo_w, &Foo::bar1, fooBar);
        std::string ret4 = manager->invokeFunctionR<std::string>(foo_w, &Foo::bar2, 0);
        std::cout << "Rank 1 got \"" << ret4 << "\" as a return from Foo::bar2()" << std::endl;
        manager->invokeFunction(foo_w, &Foo::bar3, manager->getFunctionHandle<decltype(&Foo::bar3),&Foo::bar3>(), 1024, 64.23); //function with a return, but not grabbing return value
        manager->invokeFunction(foo_w, &Foo::bar4<double,std::string>, fooBar4, testmap);
    }

    manager->invokeFunction(0, done, manager->rank());
    
    while (manager->checkMessages() && procsToGo > 0) {}

    manager->shutdown();

    if (manager->rank() == 0)
    {
        std::cout << "Foo::val is now " << foo->val << std::endl;
    }

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
}
