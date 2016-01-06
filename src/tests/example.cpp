#include "../manager.hpp"
#include "../parameterstream.hpp"

#include <map>
#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/types.h>

void f1() { std::cout << "Running function f1()" << std::endl; }

int f2() { std::cout << "Running function f2()" << std::endl; return 2; }

double f3(double p) { std::cout << "Running function f3(double)" << std::endl; return p; }

void f4(const char* s) { std::cout << "f4(const char*): " << s << std::endl; }

void f5(int* p, int t1, int& t2, int&& t3) { std::cout << "basic pointer type: " << *p << " , int: " << t1 << " int&: " << t2 << " int&&: " << t3 << std::endl; }

struct Foo
{
    void bar1() { std::cout << "Ran Foo::bar()" << std::endl; }
    std::string bar2() { std::cout << "Ran Foo::bar2()" << std::endl; return std::string("Have a string!"); }
    virtual int bar3(int p, double v) { std::cout << "Ran Foo::bar3(int,double)" << std::endl; val = v; return p; }
    template <typename T1, typename T2>
    void bar4(const std::map<T1,T2>& m) {
        std::cout << "Ran Foo::bar4(std::map)" << std::endl;
        for (auto& i : m) {
            std::cout << i.first << " " << i.second << std::endl;
        }
    }
    template <typename T1, typename T2>
    void bar5(const std::map<T1,T2>* m) {
        std::cout << "Ran Foo::bar5(std::map*)" << std::endl;
        for (auto& i : *m) {
            std::cout << i.first << " " << i.second << std::endl;
        }
    }
    double val;
};

template<typename... Dims, typename T>
void testvla(T *&t, Dims... dims)
{
  t[1];
}

auto testvla2(size_t x, size_t y, int* vla)
{
    return reinterpret_cast<int(*)[x]>(vla);
}

void testvla3(std::size_t x, std::size_t y, int* pvla)
{
    int (*vla)[x] = reinterpret_cast<int(*)[x]>(pvla);
    for(int i = 0; i < y; ++i)
    {
        for(int j = 0; j < x; ++j)
        {
            std::cout << vla[i][j] << std::endl;
        }
    }
}

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);
    
    size_t x = 4, y=5;
    int vla[y][x] = { {1,2,3,4},
                      {5,6,7,8},
                      {9,10,11,12},
                      {13,14,15,16},
                      {17,18,19,20}
                    };
    int *pvla = (int*) vla;
    int (*vla2)[x] = reinterpret_cast<int(*)[x]>(pvla);
    
    testvla2(x,y,pvla);
    
    
    //auto testvar = testvla(pvla, x,y);
    testvla3(x,y,pvla);

    mpirpc::Manager* manager = new mpirpc::Manager();

    std::cout << "Running example main. Rank: " << manager->rank() << std::endl;

    bool test = false;
    int test2 = 5;
    int procsToGo = manager->numProcs();

    manager->registerType<Foo>();

    mpirpc::FunctionHandle simple_lambda = manager->registerLambda([]() {
        std::cout << "Running simple lambda" << std::endl;
    });

    mpirpc::FunctionHandle reply = manager->registerLambda([](int r, const std::string& s) {
        std::cout << "Rank " << r << " replied with a message: " << s << std::endl;
    });

    mpirpc::FunctionHandle set_test = manager->registerLambda([&]() {
        std::cout << "Setting test to true" << std::endl;
        test = true;
        return 2.0f;
    });

    mpirpc::FunctionHandle set_test2 = manager->registerLambda([&](int p) {
        std::cout << "Setting test2 to " << p << std::endl; test2 = p;
        manager->invokeFunction(p, reply, manager->rank(), std::string("Hello there!"));
    });

    mpirpc::FunctionHandle done = manager->registerLambda([&](int r) {
        --procsToGo;
        std::cout << "Done on rank " << r << ". Left: " << procsToGo << std::endl;
    });

    //mpirpc::FunctionHandle hf1 = manager->registerFunction<decltype(&f1),&f1>();
    //mpirpc::FunctionHandle hf2 = manager->registerFunction<decltype(&f2),&f2>();
    //mpirpc::FunctionHandle hf3 = manager->registerFunction<decltype(&f3),&f3>();
    //mpirpc::FunctionHandle hf4 = manager->registerFunction<decltype(&f4),&f4>();
    mpirpc::FunctionHandle hf5 = manager->registerFunction<decltype(&f5),&f5>();
    //mpirpc::FunctionHandle fooBar1 = manager->registerFunction<decltype(&Foo::bar1),&Foo::bar1>();
    //mpirpc::FunctionHandle fooBar2 = manager->registerFunction<decltype(&Foo::bar2),&Foo::bar2>();
    //mpirpc::FunctionHandle fooBar3 = manager->registerFunction<decltype(&Foo::bar3),&Foo::bar3>();
    //mpirpc::FunctionHandle fooBar4 = manager->registerFunction<decltype(&Foo::bar4<double,std::string>),&Foo::bar4<double,std::string>>();
    //mpirpc::FunctionHandle fooBar5 = manager->registerFunction<decltype(&Foo::bar5<double,std::string>),&Foo::bar5<double,std::string>>();
    //mpirpc::FunctionHandle fooBar5 = manager->registerFunction<void(Foo::*)(PointerParameter<std::map<double,std::string>>&),&Foo::bar5<double,std::string>>();
    //mpirpc::FunctionHandle syscall = manager->registerFunction<decltype(&getuid),&getuid>();

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
        //manager->invokeFunction(1, simple_lambda);
    }
    else if (manager->rank() == 1)
    {
        //manager->invokeFunction(0, simple_lambda);
        //float ret1 = manager->invokeFunctionR<float>(0, set_test);
        //std::cout << "Rank 1 got " << ret1 << " as a return from set_test lambda" << std::endl;
        manager->invokeFunction(0, set_test2, 1);
        //manager->invokeFunction(0, &f1, hf1);
        //manager->invokeFunction(0, &f1, 0); //handle lookup
        //int ret2 = manager->invokeFunctionR<int>(0, &f2, manager->getFunctionHandle<decltype(&f2),&f2>());
        //std::cout << "Rank 1 got " << ret2 << " as a return from f2()" << std::endl;
        //double ret3 = manager->invokeFunctionR<double>(0, &f3, 0, 5.3f); // 5.3f should convert to a double
        //std::cout << "Rank 1 got " << ret3 << " as a return from f3()" << std::endl;
        //manager->invokeFunction(0, &f4, 0, "C string");
	int *test3 = new int[4];
        manager->invokeFunction(0, &f5, 0, mpirpc::PointerWrapper<int[]>(new int[4]), 6, 7, 8);
        //manager->invokeFunction(foo_w, &Foo::bar1, fooBar1);
        //std::string ret4 = manager->invokeFunctionR<std::string>(foo_w, &Foo::bar2, 0);
        //std::cout << "Rank 1 got \"" << ret4 << "\" as a return from Foo::bar2()" << std::endl;
        //manager->invokeFunction(foo_w, &Foo::bar3, manager->getFunctionHandle<decltype(&Foo::bar3),&Foo::bar3>(), 1024, 64.23); //function with a return, but not grabbing return value
        //manager->invokeFunction(foo_w, &Foo::bar4<double,std::string>, fooBar4, testmap);
        //manager->invokeFunction(foo_w, &Foo::bar5<double,std::string>, fooBar5, mpirpc::PointerParameter<std::map<double,std::string>>(&testmap));
        //std::cout << "Rank 0 is running with UID: " << manager->invokeFunctionR<uid_t>(0, &getuid, syscall) << std::endl;
    }

    manager->invokeFunction(0, done, manager->rank());

    while (manager->checkMessages() && procsToGo > 0) {}


    if (manager->rank() == 0)
    {
        manager->shutdownAll();
        std::cout << "Foo::val is now " << foo->val << std::endl;
    }

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
}

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle;