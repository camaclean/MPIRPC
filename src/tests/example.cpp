/*
 * MPIRPC: MPI based invocation of functions on other ranks
 * Copyright (C) 2014-2016 Colin MacLean <cmaclean@illinois.edu>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

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

void f4( std::string s2)
{
    std::cout << "f4(std::string&): " << s2 << std::endl;
    s2 = "edited string";
}

void f5(int * p, int t1, int& t2, int&& t3) {
    std::cout << "basic pointer type: " << p[0] << " " << p[1] << " " << p[2] << " " << p[3] << " , int: " << t1 << " int&: " << t2 << " int&&: " << t3 << std::endl;
    t2 = -1;
    p[0] = -5;
}

void f6(const char* str) { std::cout << "f6(const char*): " << str << std::endl; }

void f7(const std::vector<int>& v)
{
    std::cout << "f7(const std::vector<int>&): ";
    for (const auto& i : v)
        std::cout << i << " ";
    std::cout << v[0] << v[0] << "v0" << std::endl;
}

void f8(int (&c)[2])
{
    std::cout << "f8(int (&)[2]: " << c[0] << " " << c[1] << std::endl;
    c[0] = 42;
}

void f9(std::size_t x, int (*c)[3])
{
    std::cout << "f9(std::size_t x, const int (*)[3]): {" << std::endl;
    for (std::size_t i = 0; i < x; ++i)
    {
        std::cout << "[" << c[i][0] << "," << c[i][1] << "," << c[i][2] << "]";
        if (i < x-1)
            std::cout << ",";
    }
    c[0][0] = 974;
    std::cout << std::endl;
}

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

    mpirpc::mpi_manager* manager = new mpirpc::mpi_manager();

    std::cout << "Running example main. Rank: " << manager->rank() << std::endl;

    bool test = false;
    int test2 = 5;
    int procsToGo = manager->num_pes();

    manager->register_type<Foo>();

    mpirpc::FnHandle simple_lambda = manager->register_lambda([]() {
        std::cout << "Running simple lambda" << std::endl;
    });

    std::string test3 = "test3";
    auto rlambda = [test = std::move(test3)](int r, std::string& s) {
            std::cout << "Rank " << r << " replied with a message: " << s << " " << test << std::endl;
            s = "rewrote string";
        };
    mpirpc::FnHandle reply = manager->register_lambda(rlambda);
    //mpirpc::FnHandle reply = manager->registerFunction<decltype(rlambda),rlambda>();

    mpirpc::FnHandle set_test = manager->register_lambda([&]() {
        std::cout << "Setting test to true" << std::endl;
        test = true;
        return 2.0f;
    });

    mpirpc::FnHandle set_test2 = manager->register_lambda([&](int p) {
        std::cout << "Setting test2 to " << p << std::endl; test2 = p;
        std::string s("Hello there!");
        //manager->invokeFunction(p, reply, manager->rank(), s);
        manager->invoke_lambda_r(p, rlambda, reply, manager->rank(), s);
        std::cout << "set_test2 got: " << s << std::endl;
    });

    mpirpc::FnHandle done = manager->register_lambda([&](int r) {
        --procsToGo;
        std::cout << "Done on rank " << r << ". Left: " << procsToGo << std::endl;
    });

    int (*parr)[3];
    parr = mpirpc::pointer_wrapper<int[3],2>(nullptr);
    mpirpc::FnHandle hf1 = manager->register_function<decltype(&f1),&f1>();
    mpirpc::FnHandle hf2 = manager->register_function<decltype(&f2),&f2>();
    mpirpc::FnHandle hf3 = manager->register_function<decltype(&f3),&f3>();
    mpirpc::FnHandle hf4 = manager->register_function<decltype(&f4),&f4>();
    mpirpc::FnHandle hf6 = manager->register_function<decltype(&f6),&f6>();
    mpirpc::FnHandle hf7 = manager->register_function<decltype(&f7),&f7>();
    mpirpc::FnHandle hf8 = manager->register_function<decltype(&f8),&f8>();
    //mpirpc::FnHandle hf9 = manager->register_function<void(*)(std::size_t,mpirpc::pointer_wrapper<int[3],2>),&f9>();
    mpirpc::FnHandle hf9 = manager->register_function<decltype(&f9),&f9>();
    std::vector<char> buffer;
    mpirpc::parameter_stream s(&buffer);
    const char* testcstr[] = {"test", "blahblah"};
    std::cout << "cstrsize: " << sizeof(testcstr)/sizeof(testcstr[0]);
    s << testcstr;
    //mpirpc::unmarshal<mpirpc::PointerWrapper<int,0UL,false,std::allocator<int>>>(s);
    //mpirpc::FnHandle hf5 = manager->register_function<void(*)(mpirpc::pointer_wrapper<int,4>,int,int&,int&&),&f5>();
    mpirpc::FnHandle hf5 = manager->register_function<decltype(&f5),&f5>();
    mpirpc::FnHandle fooBar1 = manager->register_function<decltype(&Foo::bar1),&Foo::bar1>();
    mpirpc::FnHandle fooBar2 = manager->register_function<decltype(&Foo::bar2),&Foo::bar2>();
    mpirpc::FnHandle fooBar3 = manager->register_function<decltype(&Foo::bar3),&Foo::bar3>();
    mpirpc::FnHandle fooBar4 = manager->register_function<decltype(&Foo::bar4<double,std::string>),&Foo::bar4<double,std::string>>();
    //mpirpc::FnHandle fooBar5 = manager->register_function<void(Foo::*)(mpirpc::pointer_wrapper<std::map<double,std::string>,1>),&Foo::bar5<double,std::string>>();
    mpirpc::FnHandle fooBar5 = manager->register_function<decltype(&Foo::bar5<double,std::string>),&Foo::bar5<double,std::string>>();
    mpirpc::FnHandle syscall = manager->register_function<decltype(&getuid),&getuid>();

    Foo *foo = nullptr;
    if (manager->rank() == 0) {
        foo = new Foo();
        foo->val = 7.1;
        manager->register_object(foo);
        std::cout << "Foo::val is " << foo->val << std::endl;
    }

    manager->sync();

    mpirpc::object_wrapper_base *foo_w = manager->get_object_of_type<Foo>();

    std::map<double,std::string> testmap;
    testmap[0.4] = "string: 0.4";
    testmap[3.14] = "3 digit pi";

    if (manager->rank() == 0) {
        manager->invoke_function(1, simple_lambda);
    }
    else if (manager->rank() == 1)
    {
        manager->invoke_function(0, simple_lambda);
        float ret1 = manager->invoke_function_r<float>(0, set_test);
        std::cout << "Rank 1 got " << ret1 << " as a return from set_test lambda" << std::endl;
        manager->invoke_function(0, set_test2, 1);
        manager->invoke_function(0, &f1, hf1);
        manager->invoke_function(0, &f1, 0); //handle lookup
        int ret2 = manager->invoke_function_r<int>(0, &f2, manager->get_fn_handle<decltype(&f2),&f2>());
        std::cout << "Rank 1 got " << ret2 << " as a return from f2()" << std::endl;
        double r = 5.3f;
        double ret3 = manager->invoke_function_r<double>(0, &f3, 0, r); // 5.3f should convert to a double
        ret3 = manager->invoke_function_r<decltype(&f3),&f3>(0, r);
        std::cout << "Rank 1 got " << ret3 << " as a return from f3()" << std::endl;
        std::string s("blah");
        //const char* cz = "C string";
        manager->invoke_function_r<decltype(&f4),&f4>(0, s);
        std::cout << "edited std::string: " << s << std:: endl;
        int *test3 = new int[4];
        test3[0] = 1;
        test3[1] = 2;
        test3[2] = 3;
        test3[3] = 4;
        int b = 7;
        std::vector<int> vectest{10,9,8,7,6,5,4,3,2,1};
        //manager->invoke_function_r<void(*)(mpirpc::pointer_wrapper<int,4>,int,int&,int&&),&f5>(0, mpirpc::pointer_wrapper<int,4>(test3), 6, b, 8);
        manager->invoke_function_r<decltype(&f5),&f5>(0, mpirpc::pointer_wrapper<int,4>(test3), 6, b, 8);
        std::cout << "test3[0]: " << test3[0] << std::endl;
        std::cout << "b: " << b << std::endl;
        manager->invoke_function<decltype(&f6),&f6>(0, (const char*) "test cstring");
        manager->invoke_function<decltype(&f7),&f7>(0, vectest);
        int c[2] = {1,2};
        manager->invoke_function_r<decltype(&f8),&f8>(0, c);
        std::cout << "c[0] and c[1]" << c[0] << " " << c[1] << std::endl;
        int c2[2][3] = {1,2,3,4,5,6};
        manager->invoke_function_r<decltype(&f9),&f9>(0,2, c2);
        std::cout << "c2[0][0]: " << c2[0][0] << std::endl;
        manager->invoke_function<decltype(&Foo::bar1),&Foo::bar1>(foo_w);
        std::string ret4 = manager->invoke_function_r<decltype(&Foo::bar2),&Foo::bar2>(foo_w);
        std::cout << "Rank 1 got \"" << ret4 << "\" as a return from Foo::bar2()" << std::endl;
        manager->invoke_function<decltype(&Foo::bar3),&Foo::bar3>(foo_w, 1024, 64.23); //function with a return, but not grabbing return value
        manager->invoke_function<decltype(&Foo::bar4<double,std::string>),&Foo::bar4<double,std::string>>(foo_w, testmap);
        //manager->invoke_function<void(Foo::*)(mpirpc::pointer_wrapper<std::map<double,std::string>,1>), &Foo::bar5<double,std::string>>(foo_w, mpirpc::pointer_wrapper<std::map<double,std::string>,1>(&testmap));
        manager->invoke_function<decltype(&Foo::bar5<double,std::string>), &Foo::bar5<double,std::string>>(foo_w, mpirpc::pointer_wrapper<std::map<double,std::string>,1>(&testmap));
        std::cout << "Rank 0 is running with UID: " << manager->invoke_function_r<uid_t>(0, &getuid, syscall) << std::endl;
        //std::cout << "any_true: " << mpirpc::any_true<false,true,true,false,false>::value << std::endl;
    }

    //manager->invoke_function(0, done, manager->rank());

    manager->sync();
    //while (manager->check_messages() && procsToGo > 0) {}


    if (manager->rank() == 0)
    {
        //manager->shutdown_all();
        std::cout << "Foo::val is now " << foo->val << std::endl;
    }

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
}

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle;
