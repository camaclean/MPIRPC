#include "../manager.hpp"
#include "../parameterstream.hpp"

#include <map>
#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/types.h>

void f1() { std::cout << "Running function f1()" << std::endl; }

int f2() { std::cout << "Running function f2()" << std::endl; return 2; }

double f3(double p) { std::cout << "Running function f3(double) " << p << std::endl; return p; }

void f4( std::string& s2) { std::cout << "f4(const char*): " << s2 << std::endl;
    s2 = "edited string";

}

void f5(int p[], int t1, int& t2, int&& t3) { std::cout << "basic pointer type: " << p[0] << " " << p[1] << " " << p[2] << " " << p[3] << " , int: " << t1 << " int&: " << t2 << " int&&: " << t3 << std::endl; t2 = -1; }

struct Foo
{
    void bar1() { std::cerr << "Ran Foo::bar()" << std::endl; }
    std::string bar2() { std::cerr << "Ran Foo::bar2()" << std::endl; return std::string("Have a string!"); }
    virtual int bar3(int p, double v) { std::cout << "Ran Foo::bar3(int,double)" << std::endl; val = v; return p; }
    template <typename T1, typename T2>
    void bar4(const std::map<T1,T2>& m) {
        std::cout << "Ran Foo::bar4(std::map)" << std::endl;
        for (auto& i : m) {
            std::cout << i.first << " " << i.second << std::endl;
        }
    }
    template <typename T1, typename T2>
    void bar5(std::map<T1,T2>* m) {
        std::cout << "Ran Foo::bar5(std::map*)" << std::endl;
        for (auto& i : *m) {
            std::cout << i.first << " " << i.second << std::endl;
        }
    }
    double val;
};

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);
    std::string t2("blah");
    auto t = std::bind(&f4,std::ref(t2));
    t();
    std::cout << t2 << std::endl;

    mpirpc::MpiManager* manager = new mpirpc::MpiManager();

    std::cout << "Running example main. Rank: " << manager->rank() << std::endl;

    bool test = false;
    int test2 = 5;
    int procsToGo = manager->numProcs();

    manager->registerType<Foo>();

    mpirpc::FnHandle simple_lambda = manager->registerLambda([]() {
        std::cout << "Running simple lambda" << std::endl;
    });

    mpirpc::FnHandle reply = manager->registerLambda([](int r, std::string& s) {
        std::cout << "Rank " << r << " replied with a message: " << s << std::endl;
        s = "rewrote string";
    });

    mpirpc::FnHandle set_test = manager->registerLambda([&]() {
        std::cout << "Setting test to true" << std::endl;
        test = true;
        return 2.0f;
    });

    mpirpc::FnHandle set_test2 = manager->registerLambda([&](int p) {
        std::cout << "Setting test2 to " << p << std::endl; test2 = p;
        std::string s("Hello there!");
        manager->invokeFunction(p, reply, manager->rank(), s);
    });

    mpirpc::FnHandle done = manager->registerLambda([&](int r) {
        --procsToGo;
        std::cout << "Done on rank " << r << ". Left: " << procsToGo << std::endl;
    });

    mpirpc::FnHandle hf1 = manager->registerFunction<decltype(&f1),&f1>();
    mpirpc::FnHandle hf2 = manager->registerFunction<decltype(&f2),&f2>();
    mpirpc::FnHandle hf3 = manager->registerFunction<decltype(&f3),&f3>();
    mpirpc::FnHandle hf4 = manager->registerFunction<decltype(&f4),&f4>();
    //mpirpc::fnhandle_t hf5 = manager->registerFunction<decltype(&f5),&f5>();
    //std::vector<char> buffer;
    //mpirpc::ParameterStream s(&buffer);
    //mpirpc::unmarshal<mpirpc::PointerWrapper<int,0UL,false,std::allocator<int>>>(s);
    mpirpc::FnHandle hf5 = manager->registerFunction<void(*)(mpirpc::PointerWrapper<int,4>,int,int&,int&&),&f5>();
    mpirpc::FnHandle fooBar1 = manager->registerFunction<decltype(&Foo::bar1),&Foo::bar1>();
    mpirpc::FnHandle fooBar2 = manager->registerFunction<decltype(&Foo::bar2),&Foo::bar2>();
    mpirpc::FnHandle fooBar3 = manager->registerFunction<decltype(&Foo::bar3),&Foo::bar3>();
    mpirpc::FnHandle fooBar4 = manager->registerFunction<decltype(&Foo::bar4<double,std::string>),&Foo::bar4<double,std::string>>();
    //mpirpc::fnhandle_t fooBar5 = manager->registerFunction<decltype(&Foo::bar5<double,std::string>),&Foo::bar5<double,std::string>>();
    mpirpc::FnHandle fooBar5 = manager->registerFunction<void(Foo::*)(mpirpc::PointerWrapper<std::map<double,std::string>,1>),&Foo::bar5<double,std::string>>();
    mpirpc::FnHandle syscall = manager->registerFunction<decltype(&getuid),&getuid>();

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
        int ret2 = manager->invokeFunctionR<int>(0, &f2, manager->get_fn_handle<decltype(&f2),&f2>());
        std::cout << "Rank 1 got " << ret2 << " as a return from f2()" << std::endl;
        double ret3 = manager->invokeFunctionR<double>(0, &f3, 0, 5.3f); // 5.3f should convert to a double
        ret3 = manager->invokeFunctionR<decltype(&f3),&f3>(0, 5.3f);
        std::cout << "Rank 1 got " << ret3 << " as a return from f3()" << std::endl;
        std::string s("blah");
        const char* cz = "C string";
        manager->invokeFunction<decltype(&f4),&f4>(0, s);
        int *test3 = new int[4];
        test3[0] = 1;
        test3[1] = 2;
        test3[2] = 3;
        test3[3] = 4;
        int b = 7;
        manager->invokeFunction<void(*)(mpirpc::PointerWrapper<int,4>,int,int&,int&&),&f5>(0, mpirpc::PointerWrapper<int,4>(test3), 6, b, 8);
        manager->invokeFunction<decltype(&Foo::bar1),&Foo::bar1>(foo_w);
        std::string ret4 = manager->invokeFunctionR<decltype(&Foo::bar2),&Foo::bar2>(foo_w);
        std::cout << "Rank 1 got \"" << ret4 << "\" as a return from Foo::bar2()" << std::endl;
        manager->invokeFunction<decltype(&Foo::bar3),&Foo::bar3>(foo_w, 1024, 64.23); //function with a return, but not grabbing return value
        manager->invokeFunction<decltype(&Foo::bar4<double,std::string>),&Foo::bar4<double,std::string>>(foo_w, testmap);
        manager->invokeFunction<void(Foo::*)(mpirpc::PointerWrapper<std::map<double,std::string>,1>), &Foo::bar5<double,std::string>>(foo_w, mpirpc::PointerWrapper<std::map<double,std::string>,1>(&testmap));
        std::cout << "Rank 0 is running with UID: " << manager->invokeFunctionR<uid_t>(0, &getuid, syscall) << std::endl;
        std::cout << "any_true: " << mpirpc::any_true<false,true,true,false,false>::value << std::endl;
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
