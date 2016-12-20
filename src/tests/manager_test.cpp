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
#include "../parameter_buffer.hpp"

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

/*template<typename Buffer, typename Type, typename Alignment>
constexpr std::size_t element_size_v = unpacked_elemet_info<Buffer,Type,Alignment>::size;*/

template<typename Buffer, bool SkipBuildTypes, bool SkipNonBuildTypes, typename Types, typename Alignments>
struct type_filter;

template<typename Buffer, bool SkipBuildTypes, bool SkipNonBuildTypes, typename T, typename... Ts, typename Alignment, typename... Alignments>
struct type_filter<Buffer,SkipBuildTypes,SkipNonBuildTypes, std::tuple<T, Ts...>, std::tuple<Alignment,Alignments...>>
{
    static constexpr bool predicate = (mpirpc::is_buildtype<std::remove_reference_t<T>,Buffer> && !SkipBuildTypes) || (!mpirpc::is_buildtype<std::remove_reference_t<T>,Buffer> && !SkipNonBuildTypes);
    using type = std::conditional_t<predicate, mpirpc::internal::tuple_cat_type<std::tuple<std::pair<std::remove_reference_t<T>,Alignment>>,typename type_filter<Buffer,SkipBuildTypes,SkipNonBuildTypes,std::tuple<Ts...>,std::tuple<Alignments...>>::type>,
                                               typename type_filter<Buffer,SkipBuildTypes,SkipNonBuildTypes,std::tuple<Ts...>,std::tuple<Alignments...>>::type>;
};

template<typename Buffer, bool SkipBuildTypes, bool SkipNonBuildTypes>
struct type_filter<Buffer,SkipBuildTypes,SkipNonBuildTypes, std::tuple<>, std::tuple<>>
{
    using type = std::tuple<>;
};

template<typename Alignment>
struct alignment_reader;

template<std::size_t Alignment>
struct alignment_reader<std::integral_constant<std::size_t,Alignment>>
{
    static constexpr std::size_t value = Alignment;
};

template<typename Alignment, typename... Alignments>
struct alignment_reader<std::tuple<Alignment,Alignments...>>
{
    static constexpr std::size_t value = alignment_reader<Alignment>::value;
};

template<typename Tuple>
struct unpack_buffer_storage_helper;

template<typename... Ts, typename... Alignments>
struct unpack_buffer_storage_helper<std::tuple<std::pair<Ts,Alignments>...>>
{
    using type = std::tuple<typename std::aligned_storage<sizeof(Ts),alignment_reader<Alignments>::value>::type...>;
};

template<typename Buffer, typename Types, typename Alignments>
using unpack_buffer_storage = typename unpack_buffer_storage_helper<typename type_filter<Buffer,false,false,Types,Alignments>::type>::type;

template<typename Buffer, bool SkipBuildTypes, bool SkipNonBuildTypes, typename Types, typename Alignments>
struct aligned_unpack_buffer;

template<typename Buffer, bool SkipBuildTypes, bool SkipNonBuildTypes, typename... Ts, typename... Alignments>
struct aligned_unpack_buffer<Buffer, SkipBuildTypes, SkipNonBuildTypes, std::tuple<Ts...>, std::tuple<Alignments...>>
{
    //using main_storage
};

template<typename Types, typename Alignments>
struct __attribute__((packed)) aligned_type_storage;

template<>
struct aligned_type_storage<std::tuple<>,std::tuple<>> {} __attribute__((packed));

template<typename T, typename... Ts, typename Alignment, typename... Alignments>
struct aligned_type_storage<std::tuple<T,Ts...>,std::tuple<Alignment,Alignments...>> : aligned_type_storage<std::tuple<Ts...>,std::tuple<Alignments...>>
{
    typename std::aligned_storage<sizeof(T),alignment_reader<Alignment>::value>::type t;
} __attribute__((packed));

void blah(double& d) {}

TEST(Test,test)
{
    std::tuple<bool,typename std::aligned_storage<sizeof(int),128>::type> t;
    std::cout << sizeof(t) << " " << sizeof(double) << " " << sizeof(double&) << " " << sizeof(std::tuple<float>) << " " << sizeof(std::tuple<float&&>) << std::endl;
    using Alignments = std::tuple<std::integral_constant<std::size_t,1>,std::integral_constant<std::size_t,1>,std::integral_constant<std::size_t,64>>;
    using Types = std::tuple<char,char,char>;
    using Buffer = mpirpc::parameter_buffer<>;
    std::cout << sizeof(unpack_buffer_storage<Buffer,std::tuple<char,char,char>,Alignments>) << std::endl;
    std::cout << sizeof(unpack_buffer_storage<Buffer,std::tuple<char,char,char>,Alignments>) << std::endl;
    aligned_type_storage<Types,Alignments> n;
    std::cout << sizeof(n) << std::endl;
    unpack_buffer_storage<Buffer,std::tuple<char,char,char>,Alignments> a;
    struct test {
        typename std::aligned_storage<1,1>::type a;
        typename std::aligned_storage<1,1>::type b;
        typename std::aligned_storage<1,64>::type c;
        typename std::aligned_storage<1,2>::type d;
    } __attribute__((packed));
    struct test2 {
        alignas(1) char a;
        alignas(1) char b;
        alignas(64) char c;
        alignas(2) char d;
    } __attribute__((packed));

    std::cout << sizeof(test) << " " << sizeof(test2) << std::endl;
    std::cout << (uintptr_t) &std::get<0>(a) << " " << (uintptr_t) &std::get<1>(a) << " " << (uintptr_t) &std::get<1>(a) % 64 <<  " " << sizeof(std::aligned_storage<1,64>) << std::endl;
    double d = 3.14;
    double&& d2 = std::move(d);
    blah(d2);
}

int main(int argc, char **argv) {

    std::cout << std::boolalpha;
    ::testing::InitGoogleTest(&argc, argv);
    MPI_Init(&argc, &argv);
    return RUN_ALL_TESTS();
    MPI_Finalize();
}
