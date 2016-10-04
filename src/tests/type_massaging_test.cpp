#include <gtest/gtest.h>
#include <tuple>
#include <iostream>
#include "../internal/function_attributes.hpp"
#include "../internal/marshalling.hpp"
#include "../internal/parameterstream.hpp"

TEST(TypeMassagingStorageTest, TestBasicStorage)
{
    using function_type = void(*)(int);
    using test_type = typename mpirpc::internal::wrapped_function_parts<function_type>::storage_tuple_type;
    using good_type = std::tuple<int>;
    test_type();
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
}

TEST(TypeMassagingStorageTest, TestBasicReferenceStorage)
{
    using function_type = void(*)(int&);
    using test_type = typename mpirpc::internal::wrapped_function_parts<function_type>::storage_tuple_type;
    using good_type = std::tuple<int>;
    test_type();
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
}

TEST(TypeMassagingStorageTest, TestPointerStorage)
{
    using function_type = void(*)(int*);
    using test_type = typename mpirpc::internal::wrapped_function_parts<function_type>::storage_tuple_type;
    using good_type = std::tuple<int*>;
    test_type();
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
}

/**
 * @brief TypeMassagingStorageTest::TestArrayStorage
 * For non-reference arrays, we need to decay the type to a pointer.
 */
TEST(TypeMassagingStorageTest, TestArrayStorage)
{
    using function_type = void(*)(int[4]);
    using test_type = typename mpirpc::internal::wrapped_function_parts<function_type>::storage_tuple_type;
    using good_type = std::tuple<int*>;
    test_type();
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
}

/**
 * @brief TypeMassagingStorageTest::TestArrayStorage
 * For references to arrays, we need to keep the type as a reference to an array.
 */
TEST(TypeMassagingStorageTest, TestArrayReferenceStorage)
{
    using function_type = void(*)(int(&)[4]);
    using test_type = typename mpirpc::internal::wrapped_function_parts<function_type>::storage_tuple_type;
    using good_type = std::tuple<int[4]>;
    test_type t;
    std::get<0>(t)[1] = 2;
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
}

TEST(TypeMassagingStorageTest, TestCString)
{
    using function_type = void(*)(const char(&)[4]);
    using test_type = typename mpirpc::internal::wrapped_function_parts<function_type>::storage_tuple_type;
    using good_type = std::tuple<char[4]>;
    test_type();
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
}

TEST(TypeMassagingStorageTest, const_primitive)
{
    using function_type = void(*)(const double);
    using test_type = typename mpirpc::internal::wrapped_function_parts<function_type>::storage_tuple_type;
    using good_type = std::tuple<double>;
    test_type();
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
}

/**
 * @brief ChooseReferenceTypeTest
 *
 * choose_reference_type should do the following, where T and U and not the same type
 *
 *      Arg     FArg        Result              Test
 *      T       T           T&&                 TestT_T_Tr
 *      T&      T           T&
 *      T&&     T           T&&
 *      T       U           U
 *      T&      U           U
 *      T&&     U           U
 *      T       T&          T&&
 *      T&      T&          T&
 *      T&&     T&          T&&
 *      T       U&          U
 *      T&      U&          U
 *      T&&     U&          U
 *      T       T&&         T&&
 *      T&      T&&         void (illegal)
 *      T&&     T&&         T&&
 *      T       U&&         U
 *      T&      U&&         void (illegal)
 *      T&&     U&&         U
 */
TEST(ChooseReferenceTypeTest, T_T)
{
    using Arg = double;
    using FArg = double;
    using test_type = mpirpc::internal::choose_reference_type<FArg,Arg>;
    using good_type = double&&;
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_TRUE(std::is_rvalue_reference<test_type>::value);
    struct test
    {
        static good_type t(Arg&& a)
        {
            return static_cast<good_type>(a);
        }
    };
    double b = test::t(3.14);
    ASSERT_EQ(3.14,b);
}

TEST(ChooseReferenceTypeTest, Tl_T)
{
    using Arg = double&;
    using FArg = double;
    using test_type = mpirpc::internal::choose_reference_type<FArg,Arg>;
    using good_type = double&;
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_TRUE(std::is_lvalue_reference<test_type>::value);
    ASSERT_FALSE(std::is_rvalue_reference<test_type>::value);
    ASSERT_FALSE(!std::is_reference<test_type>::value);
    struct test
    {
        static good_type t(Arg a)
        {
            return static_cast<good_type>(a);
        }
    };
    double a = 3.14;
    good_type b = test::t(a);
    ASSERT_EQ(3.14,b);
}

TEST(ChooseReferenceTypeTest, Tr_T)
{
    using Arg = double&&;
    using FArg = double;
    using test_type = mpirpc::internal::choose_reference_type<FArg,Arg>;
    using good_type = double&&;
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_TRUE(std::is_rvalue_reference<test_type>::value);
    struct test
    {
        static good_type t(Arg a)
        {
            return static_cast<good_type>(a);
        }
    };
    Arg a = 3.14;
    good_type b = test::t(std::move(a));
    ASSERT_EQ(3.14,b);
}

TEST(ChooseReferenceTypeTest, T_U)
{
    using Arg = float;
    using FArg = double;
    using test_type = mpirpc::internal::choose_reference_type<FArg,Arg>;
    using good_type = double;
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_TRUE(!std::is_reference<test_type>::value);
    struct test
    {
        static good_type t(Arg a)
        {
            return static_cast<good_type>(a);
        }
    };
    good_type b = test::t(3.0f);
    ASSERT_EQ(3,b);
}

TEST(ChooseReferenceTypeTest, Tl_U)
{
    using Arg = float&;
    using FArg = double;
    using test_type = mpirpc::internal::choose_reference_type<FArg,Arg>;
    using good_type = double;
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_TRUE(!std::is_reference<test_type>::value);
    struct test
    {
        static good_type t(Arg a)
        {
            return static_cast<good_type>(a);
        }
    };
    float a = 3.0f;
    good_type b = test::t(a);
    ASSERT_EQ(3.0f,b);
}

TEST(ChooseReferenceTypeTest, Tr_U)
{
    using Arg = float&&;
    using FArg = double;
    using test_type = mpirpc::internal::choose_reference_type<FArg,Arg>;
    using good_type = double;
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_TRUE(!std::is_reference<test_type>::value);
    struct test
    {
        static good_type t(Arg a)
        {
            return static_cast<good_type>(a);
        }
    };
    Arg a = 3.0f;
    good_type b = test::t(std::move(a));
    ASSERT_EQ(3,b);
}

TEST(ChooseReferenceTypeTest, T_Tl)
{
    using Arg = double;
    using FArg = double&;
    using test_type = mpirpc::internal::choose_reference_type<FArg,Arg>;
    using good_type = double&&;
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_TRUE(std::is_rvalue_reference<test_type>::value);
    struct test
    {
        static good_type t(Arg&& a)
        {
            return static_cast<good_type>(a);
        }
    };
    double b = test::t(3.14);
    ASSERT_EQ(3.14,b);
}

TEST(ChooseReferenceTypeTest, Tl_Tl)
{
    using Arg = double&;
    using FArg = double&;
    using test_type = mpirpc::internal::choose_reference_type<FArg,Arg>;
    using good_type = double&;
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_TRUE(std::is_lvalue_reference<test_type>::value);
    struct test
    {
        static good_type t(Arg a)
        {
            return static_cast<good_type>(a);
        }
    };
    double a = 3.14;
    good_type b = test::t(a);
    ASSERT_EQ(3.14,b);
}

TEST(ChooseReferenceTypeTest, Tr_Tl)
{
    using Arg = double&&;
    using FArg = double&;
    using test_type = mpirpc::internal::choose_reference_type<FArg,Arg>;
    using good_type = double&&;
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_TRUE(std::is_rvalue_reference<test_type>::value);
    struct test
    {
        static good_type t(Arg a)
        {
            return static_cast<good_type>(a);
        }
    };
    Arg a = 3.14;
    good_type b = test::t(std::move(a));
    ASSERT_EQ(3.14,b);
}

TEST(ChooseReferenceTypeTest, T_Ul)
{
    using Arg = float;
    using FArg = double&;
    using test_type = mpirpc::internal::choose_reference_type<FArg,Arg>;
    using good_type = double;
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_TRUE(!std::is_reference<test_type>::value);
    struct test
    {
        static good_type t(Arg a)
        {
            return static_cast<good_type>(a);
        }
    };
    good_type b = test::t(3.0f);
    ASSERT_EQ(3.0,b);
}

TEST(ChooseReferenceTypeTest, Tl_Ul)
{
    using Arg = float&;
    using FArg = double&;
    using test_type = mpirpc::internal::choose_reference_type<FArg,Arg>;
    using good_type = double;
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_TRUE(!std::is_reference<test_type>::value);
    struct test
    {
        static good_type t(Arg a)
        {
            return static_cast<good_type>(a);
        }
    };
    float a = 3.0f;
    good_type b = test::t(a);
    ASSERT_EQ(3.0,b);
}

TEST(ChooseReferenceTypeTest, Tr_Ul)
{
    using Arg = float&&;
    using FArg = double&;
    using test_type = mpirpc::internal::choose_reference_type<FArg,Arg>;
    using good_type = double;
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_TRUE(!std::is_reference<test_type>::value);
    struct test
    {
        static good_type t(Arg a)
        {
            return static_cast<good_type>(a);
        }
    };
    float a = 3.0f;
    good_type b = test::t(std::move(a));
    ASSERT_EQ(3.0,b);
}

TEST(ChooseReferenceTypeTest, T_Tr)
{
    using Arg = double;
    using FArg = double&&;
    using test_type = mpirpc::internal::choose_reference_type<FArg,Arg>;
    using good_type = double&&;
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_TRUE(std::is_rvalue_reference<test_type>::value);
    struct test
    {
        static good_type t(Arg&& a)
        {
            return static_cast<good_type>(a);
        }
    };
    double b = test::t(3.0);
    ASSERT_EQ(3.0,b);
}

/*TEST(ChooseReferenceTypeTest, Tl_Tr)
{
    using Arg = double&;
    using FArg = double&&;
    using test_type = mpirpc::internal::choose_reference_type<FArg,Arg>;
    using good_type = void;
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
}*/

TEST(ChooseReferenceTypeTest, Tr_Tr)
{
    using Arg = double&&;
    using FArg = double&&;
    using test_type = mpirpc::internal::choose_reference_type<FArg,Arg>;
    using good_type = double&&;
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_TRUE(std::is_rvalue_reference<test_type>::value);
    struct test
    {
        static good_type t(Arg a)
        {
            return static_cast<good_type>(a);
        }
    };
    double a = 3.0;
    double b = test::t(std::move(a));
    ASSERT_EQ(3.0,b);
}

TEST(ChooseReferenceTypeTest, T_Ur)
{
    using Arg = float;
    using FArg = double&&;
    using test_type = mpirpc::internal::choose_reference_type<FArg,Arg>;
    using good_type = double;
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_TRUE(!std::is_reference<test_type>::value);
    struct test
    {
        static good_type t(Arg a)
        {
            return static_cast<good_type>(a);
        }
    };
    good_type b = test::t(3.0f);
    ASSERT_EQ(3.0,b);
}

/*TEST(ChooseReferenceTypeTest, Tl_Ur)
{
    using Arg = float&;
    using FArg = double&&;
    using test_type = mpirpc::internal::choose_reference_type<FArg,Arg>;
    using good_type = void;
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
}*/

TEST(ChooseReferenceTypeTest, Tr_Ur)
{
    using Arg = float&&;
    using FArg = double&&;
    using test_type = mpirpc::internal::choose_reference_type<FArg,Arg>;
    using good_type = double;
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_TRUE(!std::is_reference<test_type>::value);
    struct test
    {
        static good_type t(Arg a)
        {
            return static_cast<good_type>(a);
        }
    };
    float a = 3.0f;
    good_type b = test::t(std::move(a));
    ASSERT_EQ(3.0,b);
}

TEST(WrappedFunctionType, basic)
{
    using F = void(*)();
    using test_type = mpirpc::internal::wrapped_function_type<F>;
    using good_type = void(*)();
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_EQ(typeid(good_type),typeid(test_type));
}

TEST(WrappedFunctionType, primitive)
{
    using F = void(*)(double);
    using test_type = mpirpc::internal::wrapped_function_type<F>;
    using good_type = void(*)(double);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_EQ(typeid(good_type),typeid(test_type));
}

TEST(WrappedFunctionType, return_primitive)
{
    using F = int(*)();
    using test_type = mpirpc::internal::wrapped_function_type<F>;
    using good_type = int(*)();
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_EQ(typeid(good_type),typeid(test_type));
}

TEST(WrappedFunctionType, pointer)
{
    using F = void(*)(double*);
    using test_type = mpirpc::internal::wrapped_function_type<F>;
    using good_type = void(*)(double*);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_EQ(typeid(good_type),typeid(test_type));
}

TEST(WrappedFunctionType, return_pointer)
{
    using F = int*(*)();
    using test_type = mpirpc::internal::wrapped_function_type<F>;
    using good_type = int*(*)();
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_EQ(typeid(good_type),typeid(test_type));
}

TEST(WrappedFunctionType, array)
{
    using F = void(*)(double[4]);
    using test_type = mpirpc::internal::wrapped_function_type<F>;
    using good_type = void(*)(double*);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_EQ(typeid(good_type),typeid(test_type));
}

TEST(WrappedFunctionType, ref_to_array)
{
    using F = void(*)(double(&)[4]);
    using test_type = mpirpc::internal::wrapped_function_type<F>;
    using good_type = void(*)(double(&)[4]);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_EQ(typeid(good_type),typeid(test_type));
}

TEST(WrappedFunctionType, return_ref_to_array)
{
    using F = int(&(*)())[4];
    using test_type = mpirpc::internal::wrapped_function_type<F>;
    using good_type = int(&(*)())[4];
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_EQ(typeid(good_type),typeid(test_type));
}

TEST(WrappedFunctionType, return_pointer_to_array)
{
    using F = int(*(*)())[4];
    using test_type = mpirpc::internal::wrapped_function_type<F>;
    using good_type = int(*(*)())[4];
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_EQ(typeid(good_type),typeid(test_type));
}

TEST(WrappedFunctionType, const_primitive)
{
    using F = void(*)(const double);
    using test_type = mpirpc::internal::wrapped_function_type<F>;
    using good_type = void(*)(const double);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_EQ(typeid(good_type),typeid(test_type));
}

TEST(WrappedFunctionType, ref)
{
    using F = void(*)(double&);
    using test_type = mpirpc::internal::wrapped_function_type<F>;
    using good_type = void(*)(double&);
    using bad_type1 = void(*)(double);
    using bad_type2 = void(*)(double&&);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    char* bad_name1 = abi::__cxa_demangle(typeid(bad_type1).name(),0,0,&status);
    char* bad_name2 = abi::__cxa_demangle(typeid(bad_type2).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_STRNE(good_name, bad_name1);
    ASSERT_STRNE(good_name, bad_name2);
    ASSERT_EQ(typeid(good_type),typeid(test_type));
    ASSERT_NE(typeid(good_type),typeid(bad_type1));
    ASSERT_NE(typeid(good_type),typeid(bad_type2));
}

TEST(WrappedFunctionType, rref)
{
    using F = void(*)(double&&);
    using test_type = mpirpc::internal::wrapped_function_type<F>;
    using good_type = void(*)(double&&);
    using bad_type1 = void(*)(double);
    using bad_type2 = void(*)(double&);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    char* bad_name1 = abi::__cxa_demangle(typeid(bad_type1).name(),0,0,&status);
    char* bad_name2 = abi::__cxa_demangle(typeid(bad_type2).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_STRNE(good_name, bad_name1);
    ASSERT_STRNE(good_name, bad_name2);
    ASSERT_EQ(typeid(good_type),typeid(test_type));
    ASSERT_NE(typeid(good_type),typeid(bad_type1));
    ASSERT_NE(typeid(good_type),typeid(bad_type2));
}

TEST(WrappedFunctionType, cstring)
{
    using F = void(*)(const char*);
    using test_type = mpirpc::internal::wrapped_function_type<F>;
    using good_type = void(*)(const char*);
    using bad_type1 = void(*)(const char*&);
    using bad_type2 = void(*)(const char*&&);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    char* bad_name1 = abi::__cxa_demangle(typeid(bad_type1).name(),0,0,&status);
    char* bad_name2 = abi::__cxa_demangle(typeid(bad_type2).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_STRNE(good_name, bad_name1);
    ASSERT_STRNE(good_name, bad_name2);
    ASSERT_EQ(typeid(good_type),typeid(test_type));
    ASSERT_NE(typeid(good_type),typeid(bad_type1));
    ASSERT_NE(typeid(good_type),typeid(bad_type2));
}

TEST(WrappedFunctionType, const_char_array)
{
    using F = void(*)(const char (&)[4]);
    using test_type = mpirpc::internal::wrapped_function_type<F>;
    using good_type = void(*)(const char(&)[4]);
    using bad_type1 = void(*)(const char*);
    using bad_type2 = void(*)(char*);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    char* bad_name1 = abi::__cxa_demangle(typeid(bad_type1).name(),0,0,&status);
    char* bad_name2 = abi::__cxa_demangle(typeid(bad_type2).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_STRNE(good_name, bad_name1);
    ASSERT_STRNE(good_name, bad_name2);
    ASSERT_EQ(typeid(good_type),typeid(test_type));
    ASSERT_NE(typeid(good_type),typeid(bad_type1));
    ASSERT_NE(typeid(good_type),typeid(bad_type2));
}

TEST(ForwardParameterType, T_T)
{
    using Arg = double;
    using FArg = double;
    auto res = mpirpc::internal::forward_parameter<FArg>(3.0);
    using test_type = void(*)(decltype(mpirpc::internal::forward_parameter<FArg>(3.0)));
    using good_type = void(*)(double&&);
    using bad_type1 = void(*)(double);
    using bad_type2 = void(*)(double&);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    char* bad_name1 = abi::__cxa_demangle(typeid(bad_type1).name(),0,0,&status);
    char* bad_name2 = abi::__cxa_demangle(typeid(bad_type2).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_STRNE(good_name, bad_name1);
    ASSERT_STRNE(good_name, bad_name2);
    ASSERT_EQ(typeid(good_type),typeid(test_type));
    ASSERT_NE(typeid(good_type),typeid(bad_type1));
    ASSERT_NE(typeid(good_type),typeid(bad_type2));
    ASSERT_EQ(3.0,res);
}

TEST(ForwardParameterType, Tl_T)
{
    using Arg = double&;
    using FArg = double;
    double a = 3.0;
    auto res = mpirpc::internal::forward_parameter<FArg>(a);
    using test_type = void(*)(decltype(mpirpc::internal::forward_parameter<FArg>(a)));
    using good_type = void(*)(double&);
    using bad_type1 = void(*)(double);
    using bad_type2 = void(*)(double&&);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    char* bad_name1 = abi::__cxa_demangle(typeid(bad_type1).name(),0,0,&status);
    char* bad_name2 = abi::__cxa_demangle(typeid(bad_type2).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_STRNE(good_name, bad_name1);
    ASSERT_STRNE(good_name, bad_name2);
    ASSERT_EQ(typeid(good_type),typeid(test_type));
    ASSERT_NE(typeid(good_type),typeid(bad_type1));
    ASSERT_NE(typeid(good_type),typeid(bad_type2));
    ASSERT_EQ(3.0,res);
}

TEST(ForwardParameterType, Tr_T)
{
    using Arg = double&&;
    using FArg = double;
    double a = 3.0;
    auto res = mpirpc::internal::forward_parameter<FArg>(std::move(a));
    using test_type = void(*)(decltype(mpirpc::internal::forward_parameter<FArg>(std::move(a))));
    using good_type = void(*)(double&&);
    using bad_type1 = void(*)(double);
    using bad_type2 = void(*)(double&);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    char* bad_name1 = abi::__cxa_demangle(typeid(bad_type1).name(),0,0,&status);
    char* bad_name2 = abi::__cxa_demangle(typeid(bad_type2).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_STRNE(good_name, bad_name1);
    ASSERT_STRNE(good_name, bad_name2);
    ASSERT_EQ(typeid(good_type),typeid(test_type));
    ASSERT_NE(typeid(good_type),typeid(bad_type1));
    ASSERT_NE(typeid(good_type),typeid(bad_type2));
    ASSERT_EQ(3.0,res);
}

TEST(ForwardParameterType, T_U)
{
    using Arg = float;
    using FArg = double;
    auto res = mpirpc::internal::forward_parameter<FArg>(3.0f);
    using test_type = void(*)(decltype(mpirpc::internal::forward_parameter<FArg>(3.0f)));
    using good_type = void(*)(double);
    using bad_type1 = void(*)(double&);
    using bad_type2 = void(*)(double&&);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    char* bad_name1 = abi::__cxa_demangle(typeid(bad_type1).name(),0,0,&status);
    char* bad_name2 = abi::__cxa_demangle(typeid(bad_type2).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_STRNE(good_name, bad_name1);
    ASSERT_STRNE(good_name, bad_name2);
    ASSERT_EQ(typeid(good_type),typeid(test_type));
    ASSERT_NE(typeid(good_type),typeid(bad_type1));
    ASSERT_NE(typeid(good_type),typeid(bad_type2));
    ASSERT_EQ(3.0,res);
}

TEST(ForwardParameterType, Tl_U)
{
    using Arg = float&;
    using FArg = double;
    float a = 3.0f;
    auto res = mpirpc::internal::forward_parameter<FArg>(a);
    using test_type = void(*)(decltype(mpirpc::internal::forward_parameter<FArg>(a)));
    using good_type = void(*)(double);
    using bad_type1 = void(*)(double&);
    using bad_type2 = void(*)(double&&);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    char* bad_name1 = abi::__cxa_demangle(typeid(bad_type1).name(),0,0,&status);
    char* bad_name2 = abi::__cxa_demangle(typeid(bad_type2).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_STRNE(good_name, bad_name1);
    ASSERT_STRNE(good_name, bad_name2);
    ASSERT_EQ(typeid(good_type),typeid(test_type));
    ASSERT_NE(typeid(good_type),typeid(bad_type1));
    ASSERT_NE(typeid(good_type),typeid(bad_type2));
    ASSERT_EQ(3.0,res);
}

TEST(ForwardParameterType, Tr_U)
{
    using Arg = float&&;
    using FArg = double;
    float a = 3.0f;
    auto res = mpirpc::internal::forward_parameter<FArg>(std::move(a));
    using test_type = void(*)(decltype(mpirpc::internal::forward_parameter<FArg>(std::move(a))));
    using good_type = void(*)(double);
    using bad_type1 = void(*)(double&);
    using bad_type2 = void(*)(double&&);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    char* bad_name1 = abi::__cxa_demangle(typeid(bad_type1).name(),0,0,&status);
    char* bad_name2 = abi::__cxa_demangle(typeid(bad_type2).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_STRNE(good_name, bad_name1);
    ASSERT_STRNE(good_name, bad_name2);
    ASSERT_EQ(typeid(good_type),typeid(test_type));
    ASSERT_NE(typeid(good_type),typeid(bad_type1));
    ASSERT_NE(typeid(good_type),typeid(bad_type2));
    ASSERT_EQ(3.0,res);
}

TEST(ForwardParameterType, T_Tl)
{
    using Arg = double;
    using FArg = double&;
    auto res = mpirpc::internal::forward_parameter<FArg>(3.0);
    using test_type = void(*)(decltype(mpirpc::internal::forward_parameter<FArg>(3.0)));
    using good_type = void(*)(double&&);
    using bad_type1 = void(*)(double);
    using bad_type2 = void(*)(double&);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    char* bad_name1 = abi::__cxa_demangle(typeid(bad_type1).name(),0,0,&status);
    char* bad_name2 = abi::__cxa_demangle(typeid(bad_type2).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_STRNE(good_name, bad_name1);
    ASSERT_STRNE(good_name, bad_name2);
    ASSERT_EQ(typeid(good_type),typeid(test_type));
    ASSERT_NE(typeid(good_type),typeid(bad_type1));
    ASSERT_NE(typeid(good_type),typeid(bad_type2));
    ASSERT_EQ(3.0,res);
}

TEST(ForwardParameterType, Tl_Tl)
{
    using Arg = double&;
    using FArg = double&;
    double a = 3.0;
    auto res = mpirpc::internal::forward_parameter<FArg>(a);
    using test_type = void(*)(decltype(mpirpc::internal::forward_parameter<FArg>(a)));
    using good_type = void(*)(double&);
    using bad_type1 = void(*)(double);
    using bad_type2 = void(*)(double&&);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    char* bad_name1 = abi::__cxa_demangle(typeid(bad_type1).name(),0,0,&status);
    char* bad_name2 = abi::__cxa_demangle(typeid(bad_type2).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_STRNE(good_name, bad_name1);
    ASSERT_STRNE(good_name, bad_name2);
    ASSERT_EQ(typeid(good_type),typeid(test_type));
    ASSERT_NE(typeid(good_type),typeid(bad_type1));
    ASSERT_NE(typeid(good_type),typeid(bad_type2));
    ASSERT_EQ(3.0,res);
}

TEST(ForwardParameterType, Tr_Tl)
{
    using Arg = double&&;
    using FArg = double&;
    double a = 3.0;
    auto res = mpirpc::internal::forward_parameter<FArg>(std::move(a));
    using test_type = void(*)(decltype(mpirpc::internal::forward_parameter<FArg>(std::move(a))));
    using good_type = void(*)(double&&);
    using bad_type1 = void(*)(double);
    using bad_type2 = void(*)(double&);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    char* bad_name1 = abi::__cxa_demangle(typeid(bad_type1).name(),0,0,&status);
    char* bad_name2 = abi::__cxa_demangle(typeid(bad_type2).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_STRNE(good_name, bad_name1);
    ASSERT_STRNE(good_name, bad_name2);
    ASSERT_EQ(typeid(good_type),typeid(test_type));
    ASSERT_NE(typeid(good_type),typeid(bad_type1));
    ASSERT_NE(typeid(good_type),typeid(bad_type2));
    ASSERT_EQ(3.0,res);
}

TEST(ForwardParameterType, T_Ul)
{
    using Arg = float;
    using FArg = double&;
    auto res = mpirpc::internal::forward_parameter<FArg>(3.0f);
    using test_type = void(*)(decltype(mpirpc::internal::forward_parameter<FArg>(3.0f)));
    using good_type = void(*)(double);
    using bad_type1 = void(*)(double&);
    using bad_type2 = void(*)(double&&);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    char* bad_name1 = abi::__cxa_demangle(typeid(bad_type1).name(),0,0,&status);
    char* bad_name2 = abi::__cxa_demangle(typeid(bad_type2).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_STRNE(good_name, bad_name1);
    ASSERT_STRNE(good_name, bad_name2);
    ASSERT_EQ(typeid(good_type),typeid(test_type));
    ASSERT_NE(typeid(good_type),typeid(bad_type1));
    ASSERT_NE(typeid(good_type),typeid(bad_type2));
    ASSERT_EQ(3.0,res);
}

TEST(ForwardParameterType, Tl_Ul)
{
    using Arg = float&;
    using FArg = double&;
    float a = 3.0f;
    auto res = mpirpc::internal::forward_parameter<FArg>(a);
    using test_type = void(*)(decltype(mpirpc::internal::forward_parameter<FArg>(a)));
    using good_type = void(*)(double);
    using bad_type1 = void(*)(double&);
    using bad_type2 = void(*)(double&&);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    char* bad_name1 = abi::__cxa_demangle(typeid(bad_type1).name(),0,0,&status);
    char* bad_name2 = abi::__cxa_demangle(typeid(bad_type2).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_STRNE(good_name, bad_name1);
    ASSERT_STRNE(good_name, bad_name2);
    ASSERT_EQ(typeid(good_type),typeid(test_type));
    ASSERT_NE(typeid(good_type),typeid(bad_type1));
    ASSERT_NE(typeid(good_type),typeid(bad_type2));
    ASSERT_EQ(3.0,res);
}

TEST(ForwardParameterType, T_Tr)
{
    using Arg = double;
    using FArg = double&&;
    auto res = mpirpc::internal::forward_parameter<FArg>(3.0);
    using test_type = void(*)(decltype(mpirpc::internal::forward_parameter<FArg>(3.0)));
    using good_type = void(*)(double&&);
    using bad_type1 = void(*)(double);
    using bad_type2 = void(*)(double&);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    char* bad_name1 = abi::__cxa_demangle(typeid(bad_type1).name(),0,0,&status);
    char* bad_name2 = abi::__cxa_demangle(typeid(bad_type2).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_STRNE(good_name, bad_name1);
    ASSERT_STRNE(good_name, bad_name2);
    ASSERT_EQ(typeid(good_type),typeid(test_type));
    ASSERT_NE(typeid(good_type),typeid(bad_type1));
    ASSERT_NE(typeid(good_type),typeid(bad_type2));
    ASSERT_EQ(3.0,res);
}

/*TEST(ForwardParameterType, Tl_Tr)
{
    using Arg = double&;
    using FArg = double&&;
    double a = 3.0;
    //auto res = mpirpc::internal::forward_parameter<FArg>(a);
    using test_type = void(*)(decltype(mpirpc::internal::forward_parameter<FArg>(a)));
    using good_type = void(*)(void);
    using bad_type1 = void(*)(double);
    using bad_type2 = void(*)(double&);
    using bad_type3 = void(*)(double&&);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    char* bad_name1 = abi::__cxa_demangle(typeid(bad_type1).name(),0,0,&status);
    char* bad_name2 = abi::__cxa_demangle(typeid(bad_type2).name(),0,0,&status);
    char* bad_name3 = abi::__cxa_demangle(typeid(bad_type3).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_STRNE(good_name, bad_name1);
    ASSERT_STRNE(good_name, bad_name2);
    ASSERT_STRNE(good_name, bad_name3);
    ASSERT_EQ(typeid(good_type),typeid(test_type));
    ASSERT_NE(typeid(good_type),typeid(bad_type1));
    ASSERT_NE(typeid(good_type),typeid(bad_type2));
    ASSERT_NE(typeid(good_type),typeid(bad_type3));
    //ASSERT_EQ(3.0,res);
}*/

TEST(ForwardParameterType, Tr_Tr)
{
    using Arg = double&&;
    using FArg = double&&;
    double a = 3.0;
    auto res = mpirpc::internal::forward_parameter<FArg>(std::move(a));
    using test_type = void(*)(decltype(mpirpc::internal::forward_parameter<FArg>(std::move(a))));
    using good_type = void(*)(double&&);
    using bad_type1 = void(*)(double);
    using bad_type2 = void(*)(double&);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    char* bad_name1 = abi::__cxa_demangle(typeid(bad_type1).name(),0,0,&status);
    char* bad_name2 = abi::__cxa_demangle(typeid(bad_type2).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_STRNE(good_name, bad_name1);
    ASSERT_STRNE(good_name, bad_name2);
    ASSERT_EQ(typeid(good_type),typeid(test_type));
    ASSERT_NE(typeid(good_type),typeid(bad_type1));
    ASSERT_NE(typeid(good_type),typeid(bad_type2));
    ASSERT_EQ(3.0,res);
}

TEST(ForwardParameterType, T_Ur)
{
    using Arg = float;
    using FArg = double&&;
    auto res = mpirpc::internal::forward_parameter<FArg>(3.0f);
    using test_type = void(*)(decltype(mpirpc::internal::forward_parameter<FArg>(3.0f)));
    using good_type = void(*)(double);
    using bad_type1 = void(*)(double&);
    using bad_type2 = void(*)(double&&);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    char* bad_name1 = abi::__cxa_demangle(typeid(bad_type1).name(),0,0,&status);
    char* bad_name2 = abi::__cxa_demangle(typeid(bad_type2).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_STRNE(good_name, bad_name1);
    ASSERT_STRNE(good_name, bad_name2);
    ASSERT_EQ(typeid(good_type),typeid(test_type));
    ASSERT_NE(typeid(good_type),typeid(bad_type1));
    ASSERT_NE(typeid(good_type),typeid(bad_type2));
    ASSERT_EQ(3.0,res);
}

/*TEST(ForwardParameterType, Tl_Ur)
{
    using Arg = float&;
    using FArg = double&&;
    float a = 3.0;
    //auto res = mpirpc::internal::forward_parameter<FArg>(a);
    using test_type = void(*)(decltype(mpirpc::internal::forward_parameter<FArg>(a)));
    using good_type = void(*)(void);
    using bad_type1 = void(*)(double);
    using bad_type2 = void(*)(double&);
    using bad_type3 = void(*)(double&&);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    char* bad_name1 = abi::__cxa_demangle(typeid(bad_type1).name(),0,0,&status);
    char* bad_name2 = abi::__cxa_demangle(typeid(bad_type2).name(),0,0,&status);
    char* bad_name3 = abi::__cxa_demangle(typeid(bad_type3).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_STRNE(good_name, bad_name1);
    ASSERT_STRNE(good_name, bad_name2);
    ASSERT_STRNE(good_name, bad_name3);
    ASSERT_EQ(typeid(good_type),typeid(test_type));
    ASSERT_NE(typeid(good_type),typeid(bad_type1));
    ASSERT_NE(typeid(good_type),typeid(bad_type2));
    ASSERT_NE(typeid(good_type),typeid(bad_type3));
    //ASSERT_EQ(3.0,res);
}*/

TEST(ForwardParameterType, Tr_Ur)
{
    using Arg = float&&;
    using FArg = double&&;
    float a = 3.0;
    auto res = mpirpc::internal::forward_parameter<FArg>(std::move(a));
    using test_type = void(*)(decltype(mpirpc::internal::forward_parameter<FArg>(std::move(a))));
    using good_type = void(*)(double);
    using bad_type1 = void(*)(double&);
    using bad_type2 = void(*)(double&&);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    char* bad_name1 = abi::__cxa_demangle(typeid(bad_type1).name(),0,0,&status);
    char* bad_name2 = abi::__cxa_demangle(typeid(bad_type2).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_STRNE(good_name, bad_name1);
    ASSERT_STRNE(good_name, bad_name2);
    ASSERT_EQ(typeid(good_type),typeid(test_type));
    ASSERT_NE(typeid(good_type),typeid(bad_type1));
    ASSERT_NE(typeid(good_type),typeid(bad_type2));
    ASSERT_EQ(3.0,res);
}

TEST(FnTypeMarshaller, simple)
{
    using F = void(*)();
    using test_type = typename ::mpirpc::internal::marshaller_function_signature<F>::applier;
    using good_type = std::function<void()>;
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_EQ(typeid(good_type),typeid(test_type));
}

TEST(FnTypeMarshaller, T_T)
{
    using F = void(*)(double);
    using test_type = typename ::mpirpc::internal::marshaller_function_signature<F,double>::applier;
    using good_type = std::function<void(double&&)>;
    using bad_type1 = std::function<void(double)>;
    using bad_type2 = std::function<void(double&)>;
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    char* bad_name1 = abi::__cxa_demangle(typeid(bad_type1).name(),0,0,&status);
    char* bad_name2 = abi::__cxa_demangle(typeid(bad_type2).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_STRNE(good_name, bad_name1);
    ASSERT_STRNE(good_name, bad_name2);
    ASSERT_EQ(typeid(good_type),typeid(test_type));
    ASSERT_NE(typeid(good_type),typeid(bad_type1));
    ASSERT_NE(typeid(good_type),typeid(bad_type2));

    mpirpc::parameter_stream s;
    mpirpc::internal::fn_type_marshaller<F>::marshal(s, 3.0);
    double a1;
    s >> a1;
    ASSERT_EQ(3.0,a1);
}

TEST(FnTypeMarshaller, Tl_T)
{
    using F = void(*)(double);
    using test_type = typename ::mpirpc::internal::marshaller_function_signature<F,double&>::applier;
    using good_type = std::function<void(double&)>;
    using bad_type1 = std::function<void(double)>;
    using bad_type2 = std::function<void(double&&)>;
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    char* bad_name1 = abi::__cxa_demangle(typeid(bad_type1).name(),0,0,&status);
    char* bad_name2 = abi::__cxa_demangle(typeid(bad_type2).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_STRNE(good_name, bad_name1);
    ASSERT_STRNE(good_name, bad_name2);
    ASSERT_EQ(typeid(good_type),typeid(test_type));
    ASSERT_NE(typeid(good_type),typeid(bad_type1));
    ASSERT_NE(typeid(good_type),typeid(bad_type2));

    mpirpc::parameter_stream s;
    double p1 = 3.0;
    mpirpc::internal::fn_type_marshaller<F>::marshal(s, p1);
    double a1;
    s >> a1;
    ASSERT_EQ(3.0,a1);
}

TEST(FnTypeMarshaller, Tr_T)
{
    using F = void(*)(double);
    using test_type = typename ::mpirpc::internal::marshaller_function_signature<F,double&&>::applier;
    using good_type = std::function<void(double&&)>;
    using bad_type1 = std::function<void(double)>;
    using bad_type2 = std::function<void(double&)>;
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    char* bad_name1 = abi::__cxa_demangle(typeid(bad_type1).name(),0,0,&status);
    char* bad_name2 = abi::__cxa_demangle(typeid(bad_type2).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_STRNE(good_name, bad_name1);
    ASSERT_STRNE(good_name, bad_name2);
    ASSERT_EQ(typeid(good_type),typeid(test_type));
    ASSERT_NE(typeid(good_type),typeid(bad_type1));
    ASSERT_NE(typeid(good_type),typeid(bad_type2));

    mpirpc::parameter_stream s;
    double p1 = 3.0;
    mpirpc::internal::fn_type_marshaller<F>::marshal(s, std::move(p1));
    double a1;
    s >> a1;
    ASSERT_EQ(3.0,a1);
}

TEST(FnTypeMarshaller, T_U)
{
    using F = void(*)(double);
    using test_type = typename ::mpirpc::internal::marshaller_function_signature<F,float>::applier;
    using good_type = std::function<void(double)>;
    using bad_type1 = std::function<void(double&)>;
    using bad_type2 = std::function<void(double&&)>;
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    char* bad_name1 = abi::__cxa_demangle(typeid(bad_type1).name(),0,0,&status);
    char* bad_name2 = abi::__cxa_demangle(typeid(bad_type2).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_STRNE(good_name, bad_name1);
    ASSERT_STRNE(good_name, bad_name2);
    ASSERT_EQ(typeid(good_type),typeid(test_type));
    ASSERT_NE(typeid(good_type),typeid(bad_type1));
    ASSERT_NE(typeid(good_type),typeid(bad_type2));

    mpirpc::parameter_stream s;
    mpirpc::internal::fn_type_marshaller<F>::marshal(s, 3.0f);
    double a1;
    s >> a1;
    ASSERT_EQ(3.0,a1);
}

TEST(FnTypeMarshaller, Tl_U)
{
    using F = void(*)(double);
    using test_type = typename ::mpirpc::internal::marshaller_function_signature<F,float&>::applier;
    using good_type = std::function<void(double)>;
    using bad_type1 = std::function<void(double&)>;
    using bad_type2 = std::function<void(double&&)>;
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    char* bad_name1 = abi::__cxa_demangle(typeid(bad_type1).name(),0,0,&status);
    char* bad_name2 = abi::__cxa_demangle(typeid(bad_type2).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_STRNE(good_name, bad_name1);
    ASSERT_STRNE(good_name, bad_name2);
    ASSERT_EQ(typeid(good_type),typeid(test_type));
    ASSERT_NE(typeid(good_type),typeid(bad_type1));
    ASSERT_NE(typeid(good_type),typeid(bad_type2));

    mpirpc::parameter_stream s;
    float p1 = 3.0f;
    mpirpc::internal::fn_type_marshaller<F>::marshal(s, p1);
    double a1;
    s >> a1;
    ASSERT_EQ(3.0,a1);
}

TEST(FnTypeMarshaller, Tr_U)
{
    using F = void(*)(double);
    using test_type = typename ::mpirpc::internal::marshaller_function_signature<F,float&&>::applier;
    using good_type = std::function<void(double)>;
    using bad_type1 = std::function<void(double&)>;
    using bad_type2 = std::function<void(double&&)>;
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    char* bad_name1 = abi::__cxa_demangle(typeid(bad_type1).name(),0,0,&status);
    char* bad_name2 = abi::__cxa_demangle(typeid(bad_type2).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_STRNE(good_name, bad_name1);
    ASSERT_STRNE(good_name, bad_name2);
    ASSERT_EQ(typeid(good_type),typeid(test_type));
    ASSERT_NE(typeid(good_type),typeid(bad_type1));
    ASSERT_NE(typeid(good_type),typeid(bad_type2));

    mpirpc::parameter_stream s;
    float p1 = 3.0f;
    mpirpc::internal::fn_type_marshaller<F>::marshal(s, std::move(p1));
    double a1;
    s >> a1;
    ASSERT_EQ(3.0,a1);
}

TEST(FnTypeMarshaller, T_Tl)
{
    using F = void(*)(double&);
    using test_type = typename ::mpirpc::internal::marshaller_function_signature<F,double>::applier;
    using good_type = std::function<void(double&&)>;
    using bad_type1 = std::function<void(double)>;
    using bad_type2 = std::function<void(double&)>;
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    char* bad_name1 = abi::__cxa_demangle(typeid(bad_type1).name(),0,0,&status);
    char* bad_name2 = abi::__cxa_demangle(typeid(bad_type2).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_STRNE(good_name, bad_name1);
    ASSERT_STRNE(good_name, bad_name2);
    ASSERT_EQ(typeid(good_type),typeid(test_type));
    ASSERT_NE(typeid(good_type),typeid(bad_type1));
    ASSERT_NE(typeid(good_type),typeid(bad_type2));

    mpirpc::parameter_stream s;
    mpirpc::internal::fn_type_marshaller<F>::marshal(s, 3.0);
    double a1;
    s >> a1;
    ASSERT_EQ(3.0,a1);
}

TEST(FnTypeMarshaller, Tl_Tl)
{
    using F = void(*)(double&);
    using test_type = typename ::mpirpc::internal::marshaller_function_signature<F,double&>::applier;
    using good_type = std::function<void(double&)>;
    using bad_type1 = std::function<void(double)>;
    using bad_type2 = std::function<void(double&&)>;
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    char* bad_name1 = abi::__cxa_demangle(typeid(bad_type1).name(),0,0,&status);
    char* bad_name2 = abi::__cxa_demangle(typeid(bad_type2).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_STRNE(good_name, bad_name1);
    ASSERT_STRNE(good_name, bad_name2);
    ASSERT_EQ(typeid(good_type),typeid(test_type));
    ASSERT_NE(typeid(good_type),typeid(bad_type1));
    ASSERT_NE(typeid(good_type),typeid(bad_type2));

    mpirpc::parameter_stream s;
    double p1 = 3.0;
    mpirpc::internal::fn_type_marshaller<F>::marshal(s, p1);
    double a1;
    s >> a1;
    ASSERT_EQ(3.0,a1);
}

TEST(FnTypeMarshaller, Tr_Tl)
{
    using F = void(*)(double&);
    using test_type = typename ::mpirpc::internal::marshaller_function_signature<F,double&&>::applier;
    using good_type = std::function<void(double&&)>;
    using bad_type1 = std::function<void(double)>;
    using bad_type2 = std::function<void(double&)>;
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    char* bad_name1 = abi::__cxa_demangle(typeid(bad_type1).name(),0,0,&status);
    char* bad_name2 = abi::__cxa_demangle(typeid(bad_type2).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_STRNE(good_name, bad_name1);
    ASSERT_STRNE(good_name, bad_name2);
    ASSERT_EQ(typeid(good_type),typeid(test_type));
    ASSERT_NE(typeid(good_type),typeid(bad_type1));
    ASSERT_NE(typeid(good_type),typeid(bad_type2));

    mpirpc::parameter_stream s;
    double p1 = 3.0;
    mpirpc::internal::fn_type_marshaller<F>::marshal(s, std::move(p1));
    double a1;
    s >> a1;
    ASSERT_EQ(3.0,a1);
}

TEST(FnTypeMarshaller, T_Ul)
{
    using F = void(*)(double&);
    using test_type = typename ::mpirpc::internal::marshaller_function_signature<F,float>::applier;
    using good_type = std::function<void(double)>;
    using bad_type1 = std::function<void(double&)>;
    using bad_type2 = std::function<void(double&&)>;
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    char* bad_name1 = abi::__cxa_demangle(typeid(bad_type1).name(),0,0,&status);
    char* bad_name2 = abi::__cxa_demangle(typeid(bad_type2).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_STRNE(good_name, bad_name1);
    ASSERT_STRNE(good_name, bad_name2);
    ASSERT_EQ(typeid(good_type),typeid(test_type));
    ASSERT_NE(typeid(good_type),typeid(bad_type1));
    ASSERT_NE(typeid(good_type),typeid(bad_type2));

    mpirpc::parameter_stream s;
    mpirpc::internal::fn_type_marshaller<F>::marshal(s, 3.0f);
    double a1;
    s >> a1;
    ASSERT_EQ(3.0,a1);
}

TEST(FnTypeMarshaller, Tl_Ul)
{
    using F = void(*)(double&);
    using test_type = typename ::mpirpc::internal::marshaller_function_signature<F,float&>::applier;
    using good_type = std::function<void(double)>;
    using bad_type1 = std::function<void(double&)>;
    using bad_type2 = std::function<void(double&&)>;
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    char* bad_name1 = abi::__cxa_demangle(typeid(bad_type1).name(),0,0,&status);
    char* bad_name2 = abi::__cxa_demangle(typeid(bad_type2).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_STRNE(good_name, bad_name1);
    ASSERT_STRNE(good_name, bad_name2);
    ASSERT_EQ(typeid(good_type),typeid(test_type));
    ASSERT_NE(typeid(good_type),typeid(bad_type1));
    ASSERT_NE(typeid(good_type),typeid(bad_type2));

    mpirpc::parameter_stream s;
    float p1 = 3.0f;
    mpirpc::internal::fn_type_marshaller<F>::marshal(s, p1);
    double a1;
    s >> a1;
    ASSERT_EQ(3.0,a1);
}

TEST(FnTypeMarshaller, Tr_Ul)
{
    using F = void(*)(double&);
    using test_type = typename ::mpirpc::internal::marshaller_function_signature<F,float&&>::applier;
    using good_type = std::function<void(double)>;
    using bad_type1 = std::function<void(double&)>;
    using bad_type2 = std::function<void(double&&)>;
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    char* bad_name1 = abi::__cxa_demangle(typeid(bad_type1).name(),0,0,&status);
    char* bad_name2 = abi::__cxa_demangle(typeid(bad_type2).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_STRNE(good_name, bad_name1);
    ASSERT_STRNE(good_name, bad_name2);
    ASSERT_EQ(typeid(good_type),typeid(test_type));
    ASSERT_NE(typeid(good_type),typeid(bad_type1));
    ASSERT_NE(typeid(good_type),typeid(bad_type2));

    mpirpc::parameter_stream s;
    float p1 = 3.0f;
    mpirpc::internal::fn_type_marshaller<F>::marshal(s, std::move(p1));
    double a1;
    s >> a1;
    ASSERT_EQ(3.0,a1);
}

TEST(FnTypeMarshaller, T_Tr)
{
    using F = void(*)(double&&);
    using test_type = typename ::mpirpc::internal::marshaller_function_signature<F,double>::applier;
    using good_type = std::function<void(double&&)>;
    using bad_type1 = std::function<void(double)>;
    using bad_type2 = std::function<void(double&)>;
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    char* bad_name1 = abi::__cxa_demangle(typeid(bad_type1).name(),0,0,&status);
    char* bad_name2 = abi::__cxa_demangle(typeid(bad_type2).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_STRNE(good_name, bad_name1);
    ASSERT_STRNE(good_name, bad_name2);
    ASSERT_EQ(typeid(good_type),typeid(test_type));
    ASSERT_NE(typeid(good_type),typeid(bad_type1));
    ASSERT_NE(typeid(good_type),typeid(bad_type2));

    mpirpc::parameter_stream s;
    mpirpc::internal::fn_type_marshaller<F>::marshal(s, 3.0);
    double a1;
    s >> a1;
    ASSERT_EQ(3.0,a1);
}

/*TEST(FnTypeMarshaller, Tl_Tr)
{
    using F = void(*)(double&&);
    using test_type = typename ::mpirpc::internal::marshaller_function_signature<F,double&>::applier;
    using good_type = std::function<void(double&)>;
    using bad_type1 = std::function<void(double)>;
    using bad_type2 = std::function<void(double&&)>;
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    char* bad_name1 = abi::__cxa_demangle(typeid(bad_type1).name(),0,0,&status);
    char* bad_name2 = abi::__cxa_demangle(typeid(bad_type2).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_STRNE(good_name, bad_name1);
    ASSERT_STRNE(good_name, bad_name2);
    ASSERT_EQ(typeid(good_type),typeid(test_type));
    ASSERT_NE(typeid(good_type),typeid(bad_type1));
    ASSERT_NE(typeid(good_type),typeid(bad_type2));

    mpirpc::parameter_stream s;
    double p1 = 3.0;
    mpirpc::internal::fn_type_marshaller<F>::marshal(s, p1);
    double a1;
    s >> a1;
    ASSERT_EQ(3.0,a1);
}*/

TEST(FnTypeMarshaller, Tr_Tr)
{
    using F = void(*)(double&&);
    using test_type = typename ::mpirpc::internal::marshaller_function_signature<F,double&&>::applier;
    using good_type = std::function<void(double&&)>;
    using bad_type1 = std::function<void(double)>;
    using bad_type2 = std::function<void(double&)>;
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    char* bad_name1 = abi::__cxa_demangle(typeid(bad_type1).name(),0,0,&status);
    char* bad_name2 = abi::__cxa_demangle(typeid(bad_type2).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_STRNE(good_name, bad_name1);
    ASSERT_STRNE(good_name, bad_name2);
    ASSERT_EQ(typeid(good_type),typeid(test_type));
    ASSERT_NE(typeid(good_type),typeid(bad_type1));
    ASSERT_NE(typeid(good_type),typeid(bad_type2));

    mpirpc::parameter_stream s;
    double p1 = 3.0;
    mpirpc::internal::fn_type_marshaller<F>::marshal(s, std::move(p1));
    double a1;
    s >> a1;
    ASSERT_EQ(3.0,a1);
}

TEST(FnTypeMarshaller, T_Ur)
{
    using F = void(*)(double&&);
    using test_type = typename ::mpirpc::internal::marshaller_function_signature<F,float>::applier;
    using good_type = std::function<void(double)>;
    using bad_type1 = std::function<void(double&)>;
    using bad_type2 = std::function<void(double&&)>;
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    char* bad_name1 = abi::__cxa_demangle(typeid(bad_type1).name(),0,0,&status);
    char* bad_name2 = abi::__cxa_demangle(typeid(bad_type2).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_STRNE(good_name, bad_name1);
    ASSERT_STRNE(good_name, bad_name2);
    ASSERT_EQ(typeid(good_type),typeid(test_type));
    ASSERT_NE(typeid(good_type),typeid(bad_type1));
    ASSERT_NE(typeid(good_type),typeid(bad_type2));

    mpirpc::parameter_stream s;
    mpirpc::internal::fn_type_marshaller<F>::marshal(s, 3.0);
    double a1;
    s >> a1;
    ASSERT_EQ(3.0,a1);
}

TEST(FnTypeMarshaller, Tr_Ur)
{
    using F = void(*)(double&&);
    using test_type = typename ::mpirpc::internal::marshaller_function_signature<F,float>::applier;
    using good_type = std::function<void(double)>;
    using bad_type1 = std::function<void(double&)>;
    using bad_type2 = std::function<void(double&&)>;
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    char* bad_name1 = abi::__cxa_demangle(typeid(bad_type1).name(),0,0,&status);
    char* bad_name2 = abi::__cxa_demangle(typeid(bad_type2).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_STRNE(good_name, bad_name1);
    ASSERT_STRNE(good_name, bad_name2);
    ASSERT_EQ(typeid(good_type),typeid(test_type));
    ASSERT_NE(typeid(good_type),typeid(bad_type1));
    ASSERT_NE(typeid(good_type),typeid(bad_type2));

    mpirpc::parameter_stream s;
    double p1 = 3.0;
    mpirpc::internal::fn_type_marshaller<F>::marshal(s, std::move(p1));
    double a1;
    s >> a1;
    ASSERT_EQ(3.0,a1);
}

TEST(FnTypeMarshaller, pointer)
{
    using F = void(*)(double*);
    using test_type = typename ::mpirpc::internal::marshaller_function_signature<F,::mpirpc::pointer_wrapper<double>>::applier;
    using good_type = std::function<void(mpirpc::pointer_wrapper<double>)>;
    using bad_type1 = std::function<void(double*)>;
    using bad_type2 = std::function<void(double*&)>;
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    char* bad_name1 = abi::__cxa_demangle(typeid(bad_type1).name(),0,0,&status);
    char* bad_name2 = abi::__cxa_demangle(typeid(bad_type2).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_STRNE(good_name, bad_name1);
    ASSERT_STRNE(good_name, bad_name2);
    ASSERT_EQ(typeid(good_type),typeid(test_type));
    ASSERT_NE(typeid(good_type),typeid(bad_type1));
    ASSERT_NE(typeid(good_type),typeid(bad_type2));

    mpirpc::parameter_stream s;
    mpirpc::internal::fn_type_marshaller<F>::marshal(s, ::mpirpc::pointer_wrapper<double>(new double[5]{1.0,2.0,3.0,4.0,5.0},5));
    double *a1 = new double[5]();
    s >> a1;
    ASSERT_EQ(1.0,a1[0]);
    ASSERT_EQ(2.0,a1[1]);
    ASSERT_EQ(3.0,a1[2]);
    ASSERT_EQ(4.0,a1[3]);
    ASSERT_EQ(5.0,a1[4]);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
