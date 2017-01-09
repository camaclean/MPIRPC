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
    Foo() {}
    Foo(int&,float&&) {}
    void bar()  { std::cout << "ran " << __PRETTY_FUNCTION__ << std::endl; }
    int  bar2() { std::cout << "ran " << __PRETTY_FUNCTION__ << std::endl; return 7; }
    void bar3(int&,float&&) {}
    void bar3() {}
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

struct reference_piecewise_construct_type {};
constexpr reference_piecewise_construct_type reference_piecewise_construct;

/*template<typename T, typename Buffer, typename Alignment>
struct reference_storage_wrapper
{
    using unmarshaller_return_type = std::result_of_t<decltype(mpirpc::unmarshaller<T,Buffer,Alignment>::unmarshal)(std::allocator<void>,Buffer)>;
    using type = std::conditional_t<std::is_same<T,unmarshaller_return_type>::value,T,
                                    std::conditional_t<std::is_reference<T>::value,>;
};*/

template<typename Types, typename Alignments>
class PACKED aligned_type_storage;

template<std::size_t Index, typename T, typename Alignment>
class PACKED aligned_type_storage_element
{
public:
    using type = T;
    using pointer = T*;
    using reference = T&;
    using const_reference = const T&;
    static constexpr std::size_t index = Index;
    using alignment_type = Alignment;
    static constexpr std::size_t alignment = alignment_reader<Alignment>::value;
    using data_type = typename std::aligned_storage<sizeof(T),alignment>::type;
private:
    data_type elem;
public:
    template<typename...Args>
    void construct(Args&&... args) { new (static_cast<T*>(static_cast<void*>(&elem))) T(std::forward<Args>(args)...); }
    void destruct() { static_cast<T*>(static_cast<void*>(&elem))->~T(); }

    pointer address() { return static_cast<pointer>(static_cast<void*>(&elem)); }
    const pointer address() const { return static_cast<pointer>(static_cast<void*>(&elem)); }

    reference get() { return *static_cast<pointer>(static_cast<void*>(&elem)); }
    const_reference get() const { return *static_cast<const pointer>(static_cast<const void*>(&elem)); }
};

/*template<typename Types, typename Alignments>
struct reference_storage_helper;

template<typename... Types, typename... Alignments>
struct reference_storage_helper<std::tuple<Types...>,std::tuple<Alignments...>>
{

};*/

template<std::size_t Size, typename T, typename... Ts>
struct reference_types_filter
{
    using type = std::conditional_t<std::is_reference<T>::value,
                                    mpirpc::internal::tuple_cat_type<std::tuple<T>,typename reference_types_filter<Size, Ts...>::type>,
                                    typename reference_types_filter<Size, Ts...>::type>;
    using map = std::conditional_t<std::is_reference<T>::value,
                                   mpirpc::internal::tuple_cat_type<std::tuple<std::integral_constant<std::size_t, Size-sizeof...(Ts)-1>>,typename reference_types_filter<Size, Ts...>::map>,
                                   mpirpc::internal::tuple_cat_type<std::tuple<decltype(std::ignore)>,typename reference_types_filter<Size, Ts...>::map>>;
};

/*template<typename T>
struct reference_types
{
    using type = std::conditional_t<std::is_reference<T>::value,std::tuple<std::remove_reference_t<T>>,std::tuple<>>;
};

template<typename... Ts>
struct reference_types<std::tuple<Ts...>>
{
    using type = typename reference_types_filter<sizeof...(Ts),Ts...>::type;
    using map = typename reference_types_filter<sizeof...(Ts),Ts...>::map;
};

template<typename T>
using reference_types_type = typename reference_types_extractor<T>::type;*/

/**
 * unmarshaller<T>::unmarshal() can produce 2 types of output:
 * T
 * std::tuple<std::piecewise_construct_t,ConstructorArguments...>
 */
template<typename T>
struct reference_types_impl
{
};

template<typename T>
struct reference_types
{

};

//template<typename T>
//using reference_types_map = typename reference_types_extractor<T>::map;

/**
 * A constructors: A(double), A(int), A(long)
 * B constructors: B(int, long)
 * Type to build: std::tuple(A, A&, A&&, B&)
 * Original construction tuple: double, int, long, std::tuple<std::piecewise_construct_t,int,long>
 * Reference storage: A, A, B
 * Construction tuple: A, A&, A&&, B&
 * Construction tuple forwarded to constructor: A&&, A&, A&&, B&
 */
/*template<std::size_t Index, typename T, typename Alignment, typename... Alignments>
class aligned_type_storage_element<Index,T,std::tuple<Alignment,Alignments...>>
{
public:
    using type = T;
    using pointer = T*;
    using reference = T&;
    using const_reference = const T&;
    static constexpr std::size_t index = Index;
    using alignment_type = Alignment;
    static constexpr std::size_t alignment = alignment_reader<Alignment>::value;
    using reference_types_tuple = reference_types_type<type>;
    //using reference_map = reference_types_map<type>;
    using reference_storage_type = aligned_type_storage<reference_types_tuple,std::tuple<Alignments...>>;
    using data_type = std::pair<typename std::aligned_storage<sizeof(T),alignment>::type,reference_storage_type>;
private:
    data_type elem;

    template<typename... Args, typename... CArgs, std::size_t... Is>
    void construct_reference_types(std::tuple<std::piecewise_construct_t,CArgs...>& args, std::index_sequence<Is...>)
    {

    }

    template<typename U, typename... Args>
    U construct_type(std::tuple<std::piecewise_construct_t,Args...>& args)
    {
        return U();
    }

    template<typename U, std::size_t I, typename... CArgs,
             std::enable_if_t<!std::is_same<U, std::tuple_element_t<I,std::tuple<CArgs...>>>::value>* = nullptr,
             std::enable_if_t<!std::is_reference<U>::value>* = nullptr>
    U get_arg(std::tuple<std::piecewise_construct_t,CArgs...>& pre_args)
    {
        return construct_type(std::get<I>(pre_args));
    }

    template<typename U, std::size_t I, typename... CArgs, std::enable_if_t<std::is_same<U, std::tuple_element_t<I,std::tuple<CArgs...>>>::value>* = nullptr>
    U& get_arg(std::tuple<std::piecewise_construct_t,CArgs...>& pre_args)
    {
        return static_cast<T&&>(std::get<I>(pre_args));
    }

    template<typename... Args, typename... CArgs, std::size_t... Is>
    void construct_impl(std::tuple<std::piecewise_construct_t, CArgs...>& pre_args, std::index_sequence<Is...>)
    {
        //using reference_types = typename reference_types_filter<sizeof...(Args),Args...>::type;
        //using reference_map = typename reference_types_filter<sizeof...(Args),Args...>::map;
        std::tuple<decltype(get_arg<Args,Is>(pre_args))...> args{get_arg<Args,Is>(pre_args)...};
        new (static_cast<T*>(static_cast<void*>(&elem.first))) T(std::get<Is>(args)...);
    }
    
    template<typename... Args, typename... CArgs, typename... RArgs, std::size_t... Is, std::size_t... RIs>
    void construct_impl(std::tuple<reference_piecewise_construct_type,std::tuple<CArgs...>,std::tuple<RArgs...>> &pre_args, std::index_sequence<Is...>, std::index_sequence<RIs...>)
    {
        
    }

public:

    template<typename... Args, typename... CArgs, std::enable_if_t<std::is_constructible<T,CArgs...>::value>* = nullptr>
    void construct(std::tuple<std::piecewise_construct_t, CArgs...>& args)
    {
    }

    template<typename... Args, typename... CArgs, typename... RArgs, std::enable_if_t<std::is_constructible<T,CArgs...>::value>* = nullptr>
    void construct(std::tuple<reference_piecewise_construct_type,std::tuple<CArgs...>,std::tuple<RArgs...>>)
    {

    }

    void destruct() { static_cast<T*>(static_cast<void*>(&elem))->~T(); }

    pointer address() { return static_cast<pointer>(static_cast<void*>(&elem)); }
    const pointer address() const { return static_cast<pointer>(static_cast<void*>(&elem)); }

    reference get() { return *static_cast<pointer>(static_cast<void*>(&elem)); }
    const_reference get() const { return *static_cast<const pointer>(static_cast<const void*>(&elem)); }
} PACKED;*/

template<typename Sizes, typename Types, typename Alignments>
class __attribute__((packed)) aligned_type_storage_impl;

template<std::size_t... Sizes, typename... Types, typename... Alignments>
class __attribute__((packed)) aligned_type_storage_impl<std::index_sequence<Sizes...>,std::tuple<Types...>,std::tuple<Alignments...>> : aligned_type_storage_element<Sizes,Types,Alignments>...
{
public:
    template<std::size_t Index>
    using type_at_index = std::tuple_element_t<Index,std::tuple<Types...>>;

    template<std::size_t Index>
    using alignment_at_index = std::tuple_element_t<Index,std::tuple<Alignments...>>;

    template<std::size_t Index>
    using element_type = aligned_type_storage_element<Index,type_at_index<Index>,alignment_at_index<Index>>;

    template<std::size_t Index, typename... Args>
    void construct(Args&&... args) { element_type<Index>::construct(std::forward<Args>(args)...); }

    template<std::size_t Index>
    void destruct() { element_type<Index>::destruct(); }

    template<std::size_t Index>
    decltype(auto) get() { return element_type<Index>::get(); }

    template<std::size_t Index>
    decltype(auto) get() const { return element_type<Index>::get(); }

    template<std::size_t Index>
    decltype(auto) address() { return element_type<Index>::address(); }

    template<std::size_t Index>
    decltype(auto) address() const { return element_type<Index>::address(); }
};

template<typename... Types, typename... Alignments>
class __attribute__((packed)) aligned_type_storage<std::tuple<Types...>, std::tuple<Alignments...>> : public aligned_type_storage_impl<std::make_index_sequence<sizeof...(Types)>, std::tuple<Types...>, std::tuple<Alignments...>>
{};

void blah(double d1, double& d2, double&& d3) {}


//template<typename ... Args>
//using print_type = decltype(std::declval<Foo>().print(std::declval<Args>() ...)) (Foo::*)(Args ...);

//template<typename... Args>
//using ConstructorType = decltype(std::declval<Foo>().bar3(std::declval<Args>()...)) (Foo::*)(Args...);

template<typename... Ts>
class packedtuple : public std::tuple<Ts...> {} PACKED;

TEST(Test,test)
{
    std::tuple<bool,typename std::aligned_storage<sizeof(int),128>::type> t;
    std::cout << sizeof(t) << " " << sizeof(double) << " " << sizeof(double&) << " " << sizeof(std::tuple<float>) << " " << sizeof(std::tuple<float&&>) << std::endl;
    using Alignments = std::tuple<std::integral_constant<std::size_t,1>,std::integral_constant<std::size_t,1>,std::integral_constant<std::size_t,64>,std::integral_constant<std::size_t,8>>;
    using Types = std::tuple<char,char,char,char>;
    using Buffer = mpirpc::parameter_buffer<>;
    std::cout << sizeof(unpack_buffer_storage<Buffer,Types,Alignments>) << std::endl;
    std::cout << sizeof(unpack_buffer_storage<Buffer,Types,Alignments>) << std::endl;
    aligned_type_storage<Types,Alignments> n;
    aligned_type_storage<std::tuple<>,std::tuple<>> n2;
    n.construct<0>('a');
    n.construct<1>('b');
    n.construct<2>('c');
    n.construct<3>('t');
    unpack_buffer_storage<Buffer,Types,Alignments> a;
    std::cout << "aligned_type_storage: " << sizeof(n) << " n2: " << sizeof(n2) << " a: " << sizeof(a) << std::endl;
    std::cout << (uintptr_t) &n.get<0>() % 1 << " " << (uintptr_t) &n.get<1>() % 1 << " " << (uintptr_t) &n.get<2>() % 64 << " " << (uintptr_t) &n.get<3>() % 8 << std::endl;
    std::tuple<double,double,double> tup{1,2,3};

    struct test {
        typename std::aligned_storage<1,1>::type a;
        typename std::aligned_storage<1,1>::type b;
        typename std::aligned_storage<1,64>::type c;
        typename std::aligned_storage<1,8>::type d;
    };
    struct test2 {
        alignas(1) char a;
        alignas(1) char b;
        alignas(64) char c;
        alignas(8) char d;
    } __attribute__((packed));

    struct test3 {
        test a __attribute__((packed));
    } __attribute__((packed));

    std::tuple<typename std::aligned_storage<1,1>::type, typename std::aligned_storage<1,64>::type, typename std::aligned_storage<1,2>::type> be;
    test ba;

    std::cout << sizeof(test) << " " << sizeof(test2) << " " << sizeof(test3) << " " << sizeof(a) << std::endl;
    std::cout << "test: " << (uintptr_t) &ba.a % 1 << " " << (uintptr_t) &ba.b % 1 << " " << (uintptr_t) &ba.c % 64 << " " << (uintptr_t) &ba.d % 8 << std::endl;
    std::cout << (uintptr_t) &std::get<0>(a) % 1 << " " << (uintptr_t) &std::get<2>(a) % 64 << " " << (uintptr_t) &std::get<3>(a) % 8 << " " << sizeof(std::aligned_storage<1,64>) << " " << sizeof(be) << std::endl;

    //std::cout << abi::__cxa_demangle(typeid(ConstructorType<int,float>).name(),0,0,0) << std::endl;
    //double d = 3.14;
    //double&& d2 = std::move(d);
    //blah(d2);
}

int main(int argc, char **argv) {

    std::cout << std::boolalpha;
    ::testing::InitGoogleTest(&argc, argv);
    MPI_Init(&argc, &argv);
    return RUN_ALL_TESTS();
    MPI_Finalize();
}
