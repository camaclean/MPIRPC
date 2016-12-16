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

#include <gtest/gtest.h>

#include "../manager.hpp"

TEST(Manager,construct)
{
    mpirpc::mpi_manager m;
}

void foo() { std::cout << "ran " << __PRETTY_FUNCTION__ << std::endl; }

TEST(Manager,register_function)
{
    mpirpc::mpi_manager m;
    m.register_function<decltype(&foo),&foo>();
}

int foo2() { std::cout << "ran " << __PRETTY_FUNCTION__ << std::endl; return 5; }

TEST(Manager,register_function_w_return)
{
    mpirpc::mpi_manager m;
    m.register_function<decltype(&foo2),&foo2>();
}

struct Foo
{
    void bar()  { std::cout << "ran " << __PRETTY_FUNCTION__ << std::endl; }
    int  bar2() { std::cout << "ran " << __PRETTY_FUNCTION__ << std::endl; return 7; }
};

TEST(Manager,register_member_function)
{
    mpirpc::mpi_manager m;
    m.register_function<decltype(&Foo::bar),&Foo::bar>();
}

TEST(Manager,register_member_function_w_return)
{
    mpirpc::mpi_manager m;
    m.register_function<decltype(&Foo::bar2),&Foo::bar2>();
}

TEST(Manager,register_std_function)
{
    mpirpc::mpi_manager m;
    std::function<void()> f{&foo};
    m.register_function(&foo);
}

TEST(Manager,register_std_function_w_return)
{
    mpirpc::mpi_manager m;
    std::function<void()> f{&foo2};
    m.register_function(&foo2);
}

TEST(Manager,execute_function_local)
{
    mpirpc::mpi_manager m;
    m.register_function<decltype(&foo),&foo>();
    m.sync();
    m.invoke_function<decltype(&foo),&foo>(0);
    m.sync();
}

TEST(Manager,execute_function_local_w_return)
{
    mpirpc::mpi_manager m;
    m.register_function<decltype(&foo2),&foo2>();
    m.sync();
    int ret = m.invoke_function_r<decltype(&foo2),&foo2>(0);
    ASSERT_EQ(5, ret);
    m.sync();
}

TEST(Manager,execute_member_function)
{
    mpirpc::mpi_manager m;
    m.register_function<decltype(&Foo::bar),&Foo::bar>();
    if (m.rank() == 0)
    {
        Foo *f = new Foo();
        m.register_object(f);
    }
    m.sync();
    auto fwrap = m.get_object_of_type(mpirpc::type_identifier<Foo>::id(),0);
    m.invoke_function<decltype(&Foo::bar),&Foo::bar>(fwrap);
    m.sync();
}

TEST(Manager,execute_member_function_w_return)
{
    mpirpc::mpi_manager m;
    m.register_function<decltype(&Foo::bar2),&Foo::bar2>();
    m.sync();
    if (m.rank() == 0)
    {
        Foo *f = new Foo();
        m.register_object(f);
    }
    m.sync();
    auto fwrap = m.get_object_of_type(mpirpc::type_identifier<Foo>::id(),0);
    auto ret = m.invoke_function_r<decltype(&Foo::bar2),&Foo::bar2>(fwrap);
    ASSERT_EQ(7,ret);
    m.sync();
}

void foo3(double d)
{
    ASSERT_EQ(3.0,d);
}

TEST(Manager,execute_function_double_local)
{
    mpirpc::mpi_manager m;
    m.register_function<decltype(&foo3),&foo3>();
    m.sync();
    m.invoke_function<decltype(&foo3),&foo3>(0,3.0);
    m.sync();
}


TEST(Manager,execute_function_double_from_float_local)
{
    mpirpc::mpi_manager m;
    m.register_function<decltype(&foo3),&foo3>();
    m.sync();
    m.invoke_function<decltype(&foo3),&foo3>(m.rank(),3.0f);
    m.sync();
}

TEST(Manager,execute_function_double)
{
    mpirpc::mpi_manager m;
    double d = 1.0f;
    auto lambda = [](double& s){
        ASSERT_EQ(1.0,s);
        s = 2.0;
    };
    mpirpc::FnHandle handle = m.register_lambda(lambda);
    m.sync();
    m.invoke_lambda_r(0,lambda,handle,d);
    ASSERT_EQ(2.0f,d);
    m.sync();
}

TEST(Manager,execute_function_tuple)
{
    mpirpc::mpi_manager m;
    double d = 1.0f;
    auto lambda = [](std::tuple<bool,double> s){
        ASSERT_EQ(1.0,std::get<1>(s));
        std::get<1>(s) = 2.0;
    };
    mpirpc::FnHandle handle = m.register_lambda(lambda);
    m.sync();
    m.invoke_lambda_r(0,lambda,handle,std::tuple<bool,double>(true,d));
    ASSERT_EQ(1.0f,d);
    m.sync();
}

TEST(Manager,execute_function_std_string)
{
    mpirpc::mpi_manager m;
    std::string blah("blahblah");
    auto lambda = [](std::string& s){
        ASSERT_EQ("blahblah",s);
        s = "foobar";
    };
    mpirpc::FnHandle handle = m.register_lambda(lambda);
    m.sync();
    m.invoke_lambda_r(0,lambda,handle,blah);
    ASSERT_EQ("foobar",blah);
    m.sync();
}

TEST(Manager,execute_function_float_std_string)
{
    mpirpc::mpi_manager m;
    std::string blah("blahblah");
    float f = 3.0f;
    auto lambda = [](float a, std::string& b){
        ASSERT_EQ(3.0f,a);
        ASSERT_EQ("blahblah",b);
        a = 4.0f;
        b = "foobar";
    };
    mpirpc::FnHandle handle = m.register_lambda(lambda);
    m.sync();
    m.invoke_lambda_r(0,lambda,handle,f,blah);
    ASSERT_EQ(3.0f,f);
    ASSERT_EQ("foobar",blah);
    m.sync();
}

//Type Casting Returning Non-Member Invoker: non-void return type
std::tuple<float,long> tcrn_r(bool b, double d)
{
    if (b)
        return std::make_tuple(3.14f,(long) (d*1000));
    else
        return std::make_tuple(2.718f,(long) (d*10));
}

TEST(ManagerInvokers,tcrn_r)
{
    mpirpc::mpi_manager m;
    m.register_function<decltype(&tcrn_r),&tcrn_r>();
    m.sync();
    auto ret = m.invoke_function_r<decltype(&tcrn_r),&tcrn_r>(0,true,4.28);
    ASSERT_EQ(3.14f,std::get<0>(ret));
    ASSERT_EQ(4280L,std::get<1>(ret));
    m.sync();
    std::array<int,5> a = {1,2,3,4,5};
    std::tuple<std::array<int,5>> t(a);
    std::tuple<int[5]>* t2 = reinterpret_cast<std::tuple<int[5]>*>(&t);
    int t3 = std::get<0>(*t2)[0];
    ASSERT_EQ(1,t3);
}

void test(int d[5]) {}

int main(int argc, char **argv) {

    std::cout << std::boolalpha;
    ::testing::InitGoogleTest(&argc, argv);
    MPI_Init(&argc, &argv);
    return RUN_ALL_TESTS();
    MPI_Finalize();
}
