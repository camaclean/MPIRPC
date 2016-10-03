#include <gtest/gtest.h>
#include <tuple>
#include <iostream>
#include "../internal/function_attributes.hpp"

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

TEST(ChooseReferenceTypeTest, Tl_Tr)
{
    using Arg = double&;
    using FArg = double&&;
    using test_type = mpirpc::internal::choose_reference_type<FArg,Arg>;
    using good_type = void;
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
}

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

TEST(ChooseReferenceTypeTest, Tl_Ur)
{
    using Arg = float&;
    using FArg = double&&;
    using test_type = mpirpc::internal::choose_reference_type<FArg,Arg>;
    using good_type = void;
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
}

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

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
