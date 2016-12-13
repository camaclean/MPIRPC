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
    m.register_type<Foo>();
    if (m.rank() == 0)
    {
        Foo *f = new Foo();
        m.register_object(f);
    }
    m.sync();
    auto fwrap = m.get_object_of_type(m.get_type_id<Foo>(),0);
    m.invoke_function<decltype(&Foo::bar),&Foo::bar>(fwrap);
    m.sync();
}

TEST(Manager,execute_member_function_w_return)
{
    mpirpc::mpi_manager m;
    m.register_function<decltype(&Foo::bar2),&Foo::bar2>();
    m.register_type<Foo>();
    m.sync();
    if (m.rank() == 0)
    {
        Foo *f = new Foo();
        m.register_object(f);
    }
    m.sync();
    auto fwrap = m.get_object_of_type(m.get_type_id<Foo>(),0);
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
    m.invoke_function<decltype(&foo3),&foo3>(0,3.0f);
    m.sync();
}

TEST(Manager,execute_function_std_string)
{
    mpirpc::mpi_manager m;
    auto lambda = [](std::string s){ std::cout << s << std::endl; };
    mpirpc::FnHandle handle = m.register_lambda(lambda);
    m.sync();
    std::string blah("blahblah");
    m.invoke_lambda_r(0,lambda,handle,blah);
    m.sync();
}

int main(int argc, char **argv) {
    std::cout << std::boolalpha;
    ::testing::InitGoogleTest(&argc, argv);
    MPI_Init(&argc, &argv);
    return RUN_ALL_TESTS();
    MPI_Finalize();
}
