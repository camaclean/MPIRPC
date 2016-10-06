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
 * choose_reference_type should do the following, where T and U and not the same type and T and U are not pointer types.
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
 *
 * If T and U are pointer types:
 *      Arg                 FArg                Pass Ownership      Pass Back   Can Realloc
 *      T *                 T *                 false               true        false
 *      T *                 T *const            false               true        false
 *      T *                 const T *           false               false       false
 *      T *                 const T *const      false               false       false
 *      const T *           T *                 false               false       false
 *      const T *           T *const            false               false       false
 *      const T *           const T *           false               false       false
 *      const T *           const T *const      false               false       false
 *      T *const            T *                 false               true        false
 *      T *const            T *const            false               true        false
 *      T *const            const T *           false               false       false
 *      T *const            const T *const      false               false       false
 *      const T *const      T *                 false               false       false
 *      const T *const      T *const            false               false       false
 *      const T *const      const T *           false               false       false
 *      const T *const      const T *const      false               false       false
 *
 *      T *&                T *                 false               true        false
 *      T *&                T *const            false               true        false
 *      T *&                const T *           false               false       false
 *      T *&                const T *const      false               false       false
 *      const T *&          T *                 false               false       false
 *      const T *&          T *const            false               false       false
 *      const T *&          const T *           false               false       false
 *      const T *&          const T *const      false               false       false
 *      T *const &          T *                 false               true        false
 *      T *const &          T *const            false               true        false
 *      T *const &          const T *           false               false       false
 *      T *const &          const T *const      false               false       false
 *      const T *const &    T *                 false               false       false
 *      const T *const &    T *const            false               false       false
 *      const T *const &    const T *           false               false       false
 *      const T *const &    const T *const      false               false       false
 *
 *      T *&&               T *                 false               true        false
 *      T *&&               T *const            false               true        false
 *      T *&&               const T *           false               false       false
 *      T *&&               const T *const      false               false       false
 *      const T *&&         T *                 false               false       false
 *      const T *&&         T *const            false               false       false
 *      const T *&&         const T *           false               false       false
 *      const T *&&         const T *const      false               false       false
 *      T *const &&         T *                 false               true        false
 *      T *const &&         T *const            false               true        false
 *      T *const &&         const T *           false               false       false
 *      T *const &&         const T *const      false               false       false
 *      const T *const &&   T *                 false               false       false
 *      const T *const &&   T *const            false               false       false
 *      const T *const &&   const T *           false               false       false
 *      const T *const &&   const T *const      false               false       false
 *
 *      T *                 T *&                false               true        false
 *      T *                 T *const &          false               true        false
 *      T *                 const T *&          false               false       false
 *      T *                 const T *const &    false               false       false
 *      const T *           T *&                false               false       false
 *      const T *           T *const &          false               false       false
 *      const T *           const T *&          false               false       false
 *      const T *           const T *const &    false               false       false
 *      T *const            T *&                false               true        false
 *      T *const            T *const &          false               true        false
 *      T *const            const T *&          false               false       false
 *      T *const            const T *const &    false               false       false
 *      const T *const      T *&                false               false       false
 *      const T *const      T *const &          false               false       false
 *      const T *const      const T *&          false               false       false
 *      const T *const      const T *const &    false               false       false
 *
 *      T *&                T *&                false               true        true
 *      T *&                T *const &          false               true        false
 *      T *&                const T *&          false               false       true
 *      T *&                const T *const &    false               false       false
 *      const T *&          T *&                false               false       true
 *      const T *&          T *const &          false               false       false
 *      const T *&          const T *&          false               false       true
 *      const T *&          const T *const &    false               false       false
 *      T *const &          T *&                false               true        false
 *      T *const &          T *const &          false               true        false
 *      T *const &          const T *&          false               false       false
 *      T *const &          const T *const &    false               false       false
 *      const T *const &    T *&                false               false       false
 *      const T *const &    T *const &          false               false       false
 *      const T *const &    const T *&          false               false       false
 *      const T *const &    const T *const &    false               false       false
 *
 *      T *&&               T *&                false               true        false
 *      T *&&               T *const &          false               true        false
 *      T *&&               const T *&          false               false       false
 *      T *&&               const T *const &    false               false       false
 *      const T *&&         T *&                false               false       false
 *      const T *&&         T *const &          false               false       false
 *      const T *&&         const T *&          false               false       false
 *      const T *&&         const T *const &    false               false       false
 *      T *const &&         T *&                false               true        false
 *      T *const &&         T *const &          false               true        false
 *      T *const &&         const T *&          false               false       false
 *      T *const &&         const T *const &    false               false       false
 *      const T *const &&   T *&                false               false       false
 *      const T *const &&   T *const &          false               false       false
 *      const T *const &&   const T *&          false               false       false
 *      const T *const &&   const T *const &    false               false       false
 *
 *      T *                 T *&&               true                false       false
 *      T *                 T *const &&         true                false       false
 *      T *                 const T *&&         true                false       false
 *      T *                 const T *const &&   true                false       false
 *      const T *           T *&&               true                false       false
 *      const T *           T *const &&         true                false       false
 *      const T *           const T *&&         true                false       false
 *      const T *           const T *const &&   true                false       false
 *      T *const            T *&&               true                false       false
 *      T *const            T *const &&         true                false       false
 *      T *const            const T *&&         true                false       false
 *      T *const            const T *const &&   true                false       false
 *      const T *const      T *&&               true                false       false
 *      const T *const      T *const &&         true                false       false
 *      const T *const      const T *&&         true                false       false
 *      const T *const      const T *const &&   true                false       false
 *
 *      T *&                T *&&               (illegal)
 *      T *&                T *const &&         (illegal)
 *      T *&                const T *&&         (illegal)
 *      T *&                const T *const &&   (illegal)
 *      const T *&          T *&&               (illegal)
 *      const T *&          T *const &&         (illegal)
 *      const T *&          const T *&&         (illegal)
 *      const T *&          const T *const &&   (illegal)
 *      T *const &          T *&&               (illegal)
 *      T *const &          T *const &&         (illegal)
 *      T *const &          const T *&&         (illegal)
 *      T *const &          const T *const &&   (illegal)
 *      const T *const &    T *&&               (illegal)
 *      const T *const &    T *const &&         (illegal)
 *      const T *const &    const T *&&         (illegal)
 *      const T *const &    const T *const &&   (illegal)
 *
 *      T *&&               T *&&               true                false       false
 *      T *&&               T *const &&         true                false       false
 *      T *&&               const T *&&         true                false       false
 *      T *&&               const T *const &&   true                false       false
 *      const T *&&         T *&&               true                false       false
 *      const T *&&         T *const &&         true                false       false
 *      const T *&&         const T *&&         true                false       false
 *      const T *&&         const T *const &&   true                false       false
 *      T *const &&         T *&&               true                false       false
 *      T *const &&         T *const &&         true                false       false
 *      T *const &&         const T *&&         true                false       false
 *      T *const &&         const T *const &&   true                false       false
 *      const T *const &&   T *&&               true                false       false
 *      const T *const &&   T *const &&         true                false       false
 *      const T *const &&   const T *&&         true                false       false
 *      const T *const &&   const T *const &&   true                false       false
 *
 * For array type decaying:
 *      Arg                 FArg                Pass Ownership      Pass Back   Can Realloc
 *      T(&)[N]             T *                 false               true        false
 *      T(&)[N]             T *const            false               true        false
 *      T(&)[N]             const T *           false               false       false
 *      T(&)[N]             const T * const     false               false       false
 *      T(&)[N]             T *&                false               true        false
 *      T(&)[N]             T *const &          false               true        false
 *      T(&)[N]             const T * &         false               false       false
 *      T(&)[N]             const T *const &    false               false       false
 *      T(&)[N]             T *&&               true                false       false
 *      T(&)[N]             T *const &&         true                false       false
 *      T(&)[N]             const T * &&        true                false       false
 *      T(&)[N]             const T *const &&   true                false       false
 *      const T(&)[N]       T *                 false               false       false
 *      const T(&)[N]       T *const            false               false       false
 *      const T(&)[N]       const T *           false               false       false
 *      const T(&)[N]       const T * const     false               false       false
 *      const T(&)[N]       T *&                false               false       false
 *      const T(&)[N]       T *const &          false               false       false
 *      const T(&)[N]       const T * &         false               false       false
 *      const T(&)[N]       const T *const &    false               false       false
 *      const T(&)[N]       T *&&               true                false       false
 *      const T(&)[N]       T *const &&         true                false       false
 *      const T(&)[N]       const T * &&        true                false       false
 *      const T(&)[N]       const T *const &&   true                false       false
 *
 *      T(&&)[N]            T *                 false               false       false
 *      T(&&)[N]            T *const            false               false       false
 *      T(&&)[N]            const T *           false               false       false
 *      T(&&)[N]            const T * const     false               false       false
 *      T(&&)[N]            T *&                false               false       false
 *      T(&&)[N]            T *const &          false               false       false
 *      T(&&)[N]            const T * &         false               false       false
 *      T(&&)[N]            const T *const &    false               false       false
 *      T(&&)[N]            T *&&               true                false       false
 *      T(&&)[N]            T *const &&         true                false       false
 *      T(&&)[N]            const T * &&        true                false       false
 *      T(&&)[N]            const T *const &&   true                false       false
 *      const T(&&)[N]      T *                 false               false       false
 *      const T(&&)[N]      T *const            false               false       false
 *      const T(&&)[N]      const T *           false               false       false
 *      const T(&&)[N]      const T * const     false               false       false
 *      const T(&&)[N]      T *&                false               false       false
 *      const T(&&)[N]      T *const &          false               false       false
 *      const T(&&)[N]      const T * &         false               false       false
 *      const T(&&)[N]      const T *const &    false               false       false
 *      const T(&&)[N]      T *&&               true                false       false
 *      const T(&&)[N]      T *const &&         true                false       false
 *      const T(&&)[N]      const T * &&        true                false       false
 *      const T(&&)[N]      const T *const &&   true                false       false
 *
 */

#define SINGLE_ARG(...) __VA_ARGS__
#define TEST_REFERENCE_TYPE(Name1,Name2,FArgument,Argument,GoodArgument,Realloc) \
    TEST(Name1, Name2) { \
        using Arg = Argument; \
        using FArg = FArgument; \
        using GArg = GoodArgument; \
        using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>); \
        using good_type = void(*)(GArg); \
        int status; \
        char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status); \
        char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status); \
        ASSERT_STREQ(good_name, test_name); \
        constexpr bool can_realloc = mpirpc::internal::can_realloc<FArg,Arg>::value; \
        ASSERT_EQ(Realloc, can_realloc); \
    }

#define TEST_REFERENCE_SCALAR_TYPE(Name1,Name2,Argument,FArgument,GoodArgument,Realloc,TestVal) \
    TEST(Name1, Name2) { \
        using Arg = Argument; \
        using FArg = FArgument; \
        using GArg = GoodArgument; \
        using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>); \
        using good_type = void(*)(GArg); \
        using cast_type = Arg&&; \
        int status; \
        char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status); \
        char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status); \
        std::cout << test_name << " " << good_name << std::endl; \
        ASSERT_STREQ(good_name, test_name); \
        constexpr bool can_realloc = mpirpc::internal::can_realloc<FArg,Arg>::value; \
        ASSERT_EQ(Realloc, can_realloc); \
        struct test \
        { \
            static GArg t(Arg&& a) \
            { \
                return static_cast<GArg>(a); \
            } \
        }; \
        std::remove_reference_t<Arg> a = TestVal; \
        std::remove_reference_t<GArg> b = test::t(static_cast<Arg>(a)); \
        ASSERT_EQ(TestVal,b); \
    }

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lANT_pT, double*,double(&)[5],SINGLE_ARG(mpirpc::pointer_wrapper<double,5,false,true>),false)


TEST_REFERENCE_SCALAR_TYPE(ChooseReferenceTypeTest, T_T  , double  , double  , double&&, false, 3.0 )
TEST_REFERENCE_SCALAR_TYPE(ChooseReferenceTypeTest, Tl_T , double& , double  , double& , false, 3.0 )
TEST_REFERENCE_SCALAR_TYPE(ChooseReferenceTypeTest, Tr_T , double&&, double  , double&&, false, 3.0 )
TEST_REFERENCE_SCALAR_TYPE(ChooseReferenceTypeTest, T_U  , float   , double  , double  , false, 3.0f)
TEST_REFERENCE_SCALAR_TYPE(ChooseReferenceTypeTest, Tl_U , float&  , double  , double  , false, 3.0f)
TEST_REFERENCE_SCALAR_TYPE(ChooseReferenceTypeTest, Tr_U , float&& , double  , double  , false, 3.0f)


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

/*      T *                 T *                 false               true        false */
TEST(ChooseReferenceTypeTest, pT_pT)
{
    using Arg = double*;
    using FArg = double*;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,true>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *                 T *const            false               true        false */
TEST(ChooseReferenceTypeTest, pT_cpT)
{
    using Arg = double*;
    using FArg = double *const;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,true>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *                 const T *           false               false       false*/
TEST(ChooseReferenceTypeTest, pT_pcT)
{
    using Arg = double*;
    using FArg = const double *;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *                 const T *const      false               false       false*/
TEST(ChooseReferenceTypeTest, pT_cpcT)
{
    using Arg = double*;
    using FArg = const double *const;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *           T *                 false               false       false*/
TEST(ChooseReferenceTypeTest, pcT_pT)
{
    using Arg = const double*;
    using FArg = double *;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *           T *const            false               false       false */
TEST(ChooseReferenceTypeTest, pcT_cpT)
{
    using Arg = const double*;
    using FArg = double *const;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *           const T *           false               false       false */
TEST(ChooseReferenceTypeTest, pcT_pcT)
{
    using Arg = const double*;
    using FArg = const double *;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *           const T *const      false               false       false*/
TEST(ChooseReferenceTypeTest, pcT_cpcT)
{
    using Arg = const double*;
    using FArg = const double *const;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *const            T *                 false               true        false*/
TEST(ChooseReferenceTypeTest, cpT_pT)
{
    using Arg = double *const;
    using FArg = double *;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,true>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *const            T *const            false               true        false */
TEST(ChooseReferenceTypeTest, cpT_cpT)
{
    using Arg = double *const;
    using FArg = double *const;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,true>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *const            const T *           false               false       false*/
TEST(ChooseReferenceTypeTest, cpT_pcT)
{
    using Arg = double *const;
    using FArg = const double *;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *const            const T *const      false               false       false*/
TEST(ChooseReferenceTypeTest, cpT_cpcT)
{
    using Arg = double *const;
    using FArg = const double *const;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *const      T *                 false               false       false*/
TEST(ChooseReferenceTypeTest, cpcT_pT)
{
    using Arg = const double *const;
    using FArg = double *;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *const      T *const            false               false       false*/
TEST(ChooseReferenceTypeTest, cpcT_cpT)
{
    using Arg = const double *const;
    using FArg = double *const;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *const      const T *           false               false       false*/
TEST(ChooseReferenceTypeTest, cpcT_pcT)
{
    using Arg = const double *const;
    using FArg = const double *;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *const      const T *const      false               false       false*/
TEST(ChooseReferenceTypeTest, cpcT_cpcT)
{
    using Arg = const double *const;
    using FArg = const double *const;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *&                T *                 false               true        false */
TEST(ChooseReferenceTypeTest, lpT_pT)
{
    using Arg = double *&;
    using FArg = double *;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,true>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *&                T *const            false               true        false */
TEST(ChooseReferenceTypeTest, lpT_cpT)
{
    using Arg = double *&;
    using FArg = double *const;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,true>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *&                const T *           false               false       false */
TEST(ChooseReferenceTypeTest, lpT_pcT)
{
    using Arg = double *&;
    using FArg = const double *;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *&                const T *const      false               false       false */
TEST(ChooseReferenceTypeTest, lpT_cpcT)
{
    using Arg = double *&;
    using FArg = const double *const;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *&          T *                 false               false       false */
TEST(ChooseReferenceTypeTest, lpcT_pT)
{
    using Arg = const double *&;
    using FArg = double *;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *&          T *const            false               false       false */
TEST(ChooseReferenceTypeTest, lpcT_cpT)
{
    using Arg = const double *&;
    using FArg = double *const;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *&          const T *           false               false       false*/
TEST(ChooseReferenceTypeTest, lpcT_pcT)
{
    using Arg = const double *&;
    using FArg = const double *;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *&          const T *const      false               false       false*/
TEST(ChooseReferenceTypeTest, lpcT_cpcT)
{
    using Arg = const double *&;
    using FArg = const double *const;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *const &          T *                 false               true        false */
TEST(ChooseReferenceTypeTest, lcpT_pT)
{
    using Arg = double *const &;
    using FArg = double *;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,true>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *const &          T *const            false               true        false */
TEST(ChooseReferenceTypeTest, lcpT_cpT)
{
    using Arg = double *const &;
    using FArg = double *const;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,true>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *const &          const T *           false               false       false */
TEST(ChooseReferenceTypeTest, lcpT_pcT)
{
    using Arg = double *const &;
    using FArg = const double *;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *const &          const T *const      false               false       false*/
TEST(ChooseReferenceTypeTest, lcpT_cpcT)
{
    using Arg = double *const &;
    using FArg = const double *const;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *const &    T *                 false               false       false*/
TEST(ChooseReferenceTypeTest, lcpcT_pT)
{
    using Arg = const double *const &;
    using FArg = double *;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *const &    T *const            false               false       false*/
TEST(ChooseReferenceTypeTest, lcpcT_cpT)
{
    using Arg = const double *const &;
    using FArg = double *const;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *const &    const T *           false               false       false*/
TEST(ChooseReferenceTypeTest, lcpcT_pcT)
{
    using Arg = const double *const &;
    using FArg = const double *;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *const &    const T *const      false               false       false*/
TEST(ChooseReferenceTypeTest, lcpcT_cpcT)
{
    using Arg = const double *const &;
    using FArg = const double *const;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *&&               T *                 false               true        false*/
TEST(ChooseReferenceTypeTest, rpT_pT)
{
    using Arg = double *&&;
    using FArg = double *;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,true>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *&&               T *const            false               true        false*/
TEST(ChooseReferenceTypeTest, rpT_cpT)
{
    using Arg = double *&&;
    using FArg = double *const;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,true>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *&&               const T *           false               false       false*/
TEST(ChooseReferenceTypeTest, rpT_pcT)
{
    using Arg = double *&&;
    using FArg = const double *;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *&&               const T *const      false               false       false */
TEST(ChooseReferenceTypeTest, rpT_cpcT)
{
    using Arg = double *&&;
    using FArg = const double *const;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *&&         T *                 false               false       false */
TEST(ChooseReferenceTypeTest, rpcT_pT)
{
    using Arg = const double *&&;
    using FArg = double *;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *&&         T *const            false               false       false */
TEST(ChooseReferenceTypeTest, rpcT_cpT)
{
    using Arg = const double *&&;
    using FArg = double *;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *&&         const T *           false               false       false */
TEST(ChooseReferenceTypeTest, rpcT_pcT)
{
    using Arg = const double *&&;
    using FArg = const double *;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *&&         const T *const      false               false       false */
TEST(ChooseReferenceTypeTest, rpcT_cpcT)
{
    using Arg = const double *&&;
    using FArg = const double *const;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *const &&         T *                 false               true        false */
TEST(ChooseReferenceTypeTest, rcpT_pT)
{
    using Arg = double *const &&;
    using FArg = double *;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,true>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *const &&         T *const            false               true        false */
TEST(ChooseReferenceTypeTest, rcpT_cpT)
{
    using Arg = double *const &&;
    using FArg = double *const;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,true>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *const &&         const T *           false               false       false */
TEST(ChooseReferenceTypeTest, rcpT_pcT)
{
    using Arg = double *const &&;
    using FArg = const double *;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *const &&         const T *const      false               false       false */
TEST(ChooseReferenceTypeTest, rcpT_cpcT)
{
    using Arg = double *const &&;
    using FArg = const double *const;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *const &&   T *                 false               false       false */
TEST(ChooseReferenceTypeTest, rcpcT_pT)
{
    using Arg = const double *const &&;
    using FArg = double *;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *const &&   T *const            false               false       false */
TEST(ChooseReferenceTypeTest, rcpcT_cpT)
{
    using Arg = const double *const &&;
    using FArg = double *const;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *const &&   const T *           false               false       false */
TEST(ChooseReferenceTypeTest, rcpcT_pcT)
{
    using Arg = const double *const &&;
    using FArg = const double *;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *const &&   const T *const      false               false       false */
TEST(ChooseReferenceTypeTest, rcpcT_cpcT)
{
    using Arg = const double *const &&;
    using FArg = const double *const;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *                 T *&                false               true        false */
TEST(ChooseReferenceTypeTest, pT_lpT)
{
    using Arg = double *;
    using FArg = double *&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,true>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *                 T *const &          false               true        false */
TEST(ChooseReferenceTypeTest, pT_lcpT)
{
    using Arg = double *;
    using FArg = double *const &;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,true>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *                 const T *&          false               false       false */
TEST(ChooseReferenceTypeTest, pT_lpcT)
{
    using Arg = double *;
    using FArg = const double *&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *                 const T *const &    false               false       false */
TEST(ChooseReferenceTypeTest, pT_lcpcT)
{
    using Arg = double *;
    using FArg = const double *const &;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *           T *&                false               false       false */
TEST(ChooseReferenceTypeTest, pcT_lpT)
{
    using Arg = const double *;
    using FArg = double *&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *           T *const &          false               false       false */
TEST(ChooseReferenceTypeTest, pcT_lcpT)
{
    using Arg = const double *;
    using FArg = double *const &;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *           const T *&          false               false       false */
TEST(ChooseReferenceTypeTest, pcT_lpcT)
{
    using Arg = const double *;
    using FArg = const double *&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *           const T *const &    false               false       false */
TEST(ChooseReferenceTypeTest, pcT_lcpcT)
{
    using Arg = const double *;
    using FArg = const double *const &;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *const            T *&                false               true        false */
TEST(ChooseReferenceTypeTest, cpT_lpT)
{
    using Arg = double *const;
    using FArg = double *&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,true>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *const            T *const &          false               true        false */
TEST(ChooseReferenceTypeTest, cpT_lcpT)
{
    using Arg = double *const;
    using FArg = double *const &;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,true>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *const            const T *&          false               false       false */
TEST(ChooseReferenceTypeTest, cpT_lpcT)
{
    using Arg = double *const;
    using FArg = const double *&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *const            const T *const &    false               false       false */
TEST(ChooseReferenceTypeTest, cpT_lcpcT)
{
    using Arg = double *const;
    using FArg = const double *const &;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *const      T *&                false               false       false */
TEST(ChooseReferenceTypeTest, cpcT_lpT)
{
    using Arg = const double *const;
    using FArg = double *&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *const      T *const &          false               false       false */
TEST(ChooseReferenceTypeTest, cpcT_lcpT)
{
    using Arg = const double *const;
    using FArg = double *const &;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *const      const T *&          false               false       false */
TEST(ChooseReferenceTypeTest, cpcT_lpcT)
{
    using Arg = const double *const;
    using FArg = const double *&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *const      const T *const &    false               false       false */
TEST(ChooseReferenceTypeTest, cpcT_lcpcT)
{
    using Arg = const double *const;
    using FArg = const double *const &;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *&                T *&                false               true        true  */
TEST(ChooseReferenceTypeTest, lpT_lpT)
{
    using Arg = double *&;
    using FArg = double *&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,true>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_TRUE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *&                T *const &          false               true        false */
TEST(ChooseReferenceTypeTest, lpT_lcpT)
{
    using Arg = double *&;
    using FArg = double *const &;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,true>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *&                const T *&          false               false       true  */
TEST(ChooseReferenceTypeTest, lpT_lpcT)
{
    using Arg = double *&;
    using FArg = const double *&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_TRUE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *&                const T *const &    false               false       false */
TEST(ChooseReferenceTypeTest, lpT_lcpcT)
{
    using Arg = double *&;
    using FArg = const double *const &;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *&          T *&                false               false       true  */
TEST(ChooseReferenceTypeTest, lpcT_lpT)
{
    using Arg = const double *&;
    using FArg = double *&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_TRUE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *&          T *const &          false               false       false */
TEST(ChooseReferenceTypeTest, lpcT_lcpT)
{
    using Arg = const double *&;
    using FArg = double *const &;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *&          const T *&          false               false       true  */
TEST(ChooseReferenceTypeTest, lpcT_lpcT)
{
    using Arg = const double *&;
    using FArg = const double *&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_TRUE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *&          const T *const &    false               false       false */
TEST(ChooseReferenceTypeTest, lpcT_lcpcT)
{
    using Arg = const double *&;
    using FArg = const double *const &;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *const &          T *&                false               true        false */
TEST(ChooseReferenceTypeTest, lcpT_lpT)
{
    using Arg = double *const &;
    using FArg = double *&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,true>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *const &          T *const &          false               true        false */
TEST(ChooseReferenceTypeTest, lcpT_lcpT)
{
    using Arg = double *const &;
    using FArg = double *const &;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,true>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *const &          const T *&          false               false       false */
TEST(ChooseReferenceTypeTest, lcpT_lpcT)
{
    using Arg = double *const &;
    using FArg = const double *&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *const &          const T *const &    false               false       false */
TEST(ChooseReferenceTypeTest, lcpT_lcpcT)
{
    using Arg = double *const &;
    using FArg = const double *const &;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *const &    T *&                false               false       false */
TEST(ChooseReferenceTypeTest, lcpcT_lpT)
{
    using Arg = const double *const &;
    using FArg = double *&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *const &    T *const &          false               false       false */
TEST(ChooseReferenceTypeTest, lcpcT_lcpT)
{
    using Arg = const double *const &;
    using FArg = double *const &;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *const &    const T *&          false               false       false */
TEST(ChooseReferenceTypeTest, lcpcT_lpcT)
{
    using Arg = const double *const &;
    using FArg = const double *&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *const &    const T *const &    false               false       false */
TEST(ChooseReferenceTypeTest, lcpcT_lcpcT)
{
    using Arg = const double *const &;
    using FArg = const double *const &;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *&&               T *&                false               true        false */
TEST(ChooseReferenceTypeTest, rpT_lpT)
{
    using Arg = double *&&;
    using FArg = double *&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,true>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *&&               T *const &          false               true        false */
TEST(ChooseReferenceTypeTest, rpT_lcpT)
{
    using Arg = double *&&;
    using FArg = double *const &;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,true>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *&&               const T *&          false               false       false */
TEST(ChooseReferenceTypeTest, rpT_lpcT)
{
    using Arg = double *&&;
    using FArg = const double *&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *&&               const T *const &    false               false       false */
TEST(ChooseReferenceTypeTest, rpT_lcpcT)
{
    using Arg = double *&&;
    using FArg = const double *const &;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *&&         T *&                false               false       false */
TEST(ChooseReferenceTypeTest, rpcT_lpT)
{
    using Arg = const double *&&;
    using FArg = double *&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *&&         T *const &          false               false       false */
TEST(ChooseReferenceTypeTest, rpcT_lcpT)
{
    using Arg = const double *&&;
    using FArg = double *const &;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *&&         const T *&          false               false       false */
TEST(ChooseReferenceTypeTest, rpcT_lpcT)
{
    using Arg = const double *&&;
    using FArg = const double *&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *&&         const T *const &    false               false       false */
TEST(ChooseReferenceTypeTest, rpcT_lcpcT)
{
    using Arg = const double *&&;
    using FArg = const double *const &;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *const &&         T *&                false               true        false */
TEST(ChooseReferenceTypeTest, rcpT_lpT)
{
    using Arg = double *const &&;
    using FArg = double *&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,true>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *const &&         T *const &          false               true        false */
TEST(ChooseReferenceTypeTest, rcpT_lcpT)
{
    using Arg = double *const &&;
    using FArg = double *const &;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,true>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *const &&         const T *&          false               false       false */
TEST(ChooseReferenceTypeTest, rcpT_lpcT)
{
    using Arg = double *const &&;
    using FArg = const double *&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *const &&         const T *const &    false               false       false */
TEST(ChooseReferenceTypeTest, rcpT_lcpcT)
{
    using Arg = double *const &&;
    using FArg = const double *const &;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *const &&   T *&                false               false       false */
TEST(ChooseReferenceTypeTest, rcpcT_lpT)
{
    using Arg = const double *const &&;
    using FArg = double *&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *const &&   T *const &          false               false       false */
TEST(ChooseReferenceTypeTest, rcpcT_lcpT)
{
    using Arg = const double *const &&;
    using FArg = double *const &;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *const &&   const T *&          false               false       false */
TEST(ChooseReferenceTypeTest, rcpcT_lpcT)
{
    using Arg = const double *const &&;
    using FArg = const double *&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *const &&   const T *const &    false               false       false */
TEST(ChooseReferenceTypeTest, rcpcT_lcpcT)
{
    using Arg = const double *const &&;
    using FArg = const double *const &;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *                 T *&&               true                false       false */
TEST(ChooseReferenceTypeTest, pT_rpT)
{
    using Arg = double *;
    using FArg = double *&&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,true,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *                 T *const &&         true                false       false */
TEST(ChooseReferenceTypeTest, pT_rcpT)
{
    using Arg = double *;
    using FArg = double *const &&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,true,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *                 const T *&&         true                false       false */
TEST(ChooseReferenceTypeTest, pT_rpcT)
{
    using Arg = double *;
    using FArg = const double *&&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,true,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *                 const T *const &&   true                false       false */
TEST(ChooseReferenceTypeTest, pT_rcpcT)
{
    using Arg = double *;
    using FArg = const double *const &&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,true,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *           T *&&               true                false       false */
TEST(ChooseReferenceTypeTest, pcT_rpT)
{
    using Arg = const double *;
    using FArg = double *&&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,true,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *           T *const &&         true                false       false */
TEST(ChooseReferenceTypeTest, pcT_rcpT)
{
    using Arg = const double *;
    using FArg = double *const &&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,true,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *           const T *&&         true                false       false */
TEST(ChooseReferenceTypeTest, pcT_rpcT)
{
    using Arg = const double *;
    using FArg = const double *&&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,true,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *           const T *const &&   true                false       false */
TEST(ChooseReferenceTypeTest, pcT_rcpcT)
{
    using Arg = const double *;
    using FArg = const double *const &&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,true,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *const            T *&&               true                false       false */
TEST(ChooseReferenceTypeTest, cpT_rpT)
{
    using Arg = double *const;
    using FArg = double *&&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,true,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *const            T *const &&         true                false       false */
TEST(ChooseReferenceTypeTest, cpT_rcpT)
{
    using Arg = double *const;
    using FArg = double *const &&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,true,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *const            const T *&&         true                false       false */
TEST(ChooseReferenceTypeTest, cpT_rpcT)
{
    using Arg = double *const;
    using FArg = const double *&&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,true,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *const            const T *const &&   true                false       false */
TEST(ChooseReferenceTypeTest, cpT_rcpcT)
{
    using Arg = double *const;
    using FArg = const double *const &&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,true,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *const      T *&&               true                false       false */
TEST(ChooseReferenceTypeTest, cpcT_rpT)
{
    using Arg = const double *const;
    using FArg = double *&&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,true,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *const      T *const &&         true                false       false */
TEST(ChooseReferenceTypeTest, cpcT_rcpT)
{
    using Arg = const double *const;
    using FArg = double *const &&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,true,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *const      const T *&&         true                false       false */
TEST(ChooseReferenceTypeTest, cpcT_rpcT)
{
    using Arg = const double *const;
    using FArg = const double *&&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,true,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *const      const T *const &&   true                false       false */
TEST(ChooseReferenceTypeTest, cpcT_rcpcT)
{
    using Arg = const double *const;
    using FArg = const double *const &&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,true,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *&                T *&&               (illegal) */
/*TEST(ChooseReferenceTypeTest, lpT_rpT)
{
    using Arg = double *&;
    using FArg = double *&&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_TRUE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}*/

/*      T *&                T *const &&         (illegal) */
/*TEST(ChooseReferenceTypeTest, lpT_rcpT)
{
    using Arg = double *&;
    using FArg = double *const &&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_TRUE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}*/

/*      T *&                const T *&&         (illegal) */
/*TEST(ChooseReferenceTypeTest, lpT_rpcT)
{
    using Arg = double *&;
    using FArg = const double *&&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_TRUE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}*/

/*      T *&                const T *const &&   (illegal) */
/*TEST(ChooseReferenceTypeTest, lpT_rcpcT)
{
    using Arg = double *&;
    using FArg = const double *const &&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_TRUE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}*/

/*      const T *&          T *&&               (illegal) */
/*TEST(ChooseReferenceTypeTest, lpcT_rpT)
{
    using Arg = const double *&;
    using FArg = double *&&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_TRUE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}*/

/*      const T *&          T *const &&         (illegal) */
/*TEST(ChooseReferenceTypeTest, lpcT_rcpT)
{
    using Arg = const double *&;
    using FArg = double *const &&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_TRUE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}*/

/*      const T *&          const T *&&         (illegal) */
/*TEST(ChooseReferenceTypeTest, lpcT_rpcT)
{
    using Arg = const double *&;
    using FArg = const double *&&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_TRUE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}*/

/*      const T *&          const T *const &&   (illegal) */
/*TEST(ChooseReferenceTypeTest, lpcT_rcpcT)
{
    using Arg = const double *&;
    using FArg = const double *const &&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_TRUE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}*/

/*      T *const &          T *&&               (illegal) */
/*TEST(ChooseReferenceTypeTest, lcpT_rpT)
{
    using Arg = double *const &;
    using FArg = double *&&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_TRUE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}*/

/*      T *const &          T *const &&         (illegal) */
/*TEST(ChooseReferenceTypeTest, lcpT_rcpT)
{
    using Arg = double *const &;
    using FArg = double *const &&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_TRUE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}*/

/*      T *const &          const T *&&         (illegal) */
/*TEST(ChooseReferenceTypeTest, lcpT_rpcT)
{
    using Arg = double *const &;
    using FArg = const double *&&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_TRUE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}*/

/*      T *const &          const T *const &&   (illegal) */
/*TEST(ChooseReferenceTypeTest, lcpT_rcpcT)
{
    using Arg = double *const &;
    using FArg = const double *const &&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_TRUE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}*/

/*      const T *const &    T *&&               (illegal) */
/*TEST(ChooseReferenceTypeTest, lcpcT_rpT)
{
    using Arg = const double *const &;
    using FArg = double *&&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_TRUE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}*/

/*      const T *const &    T *const &&         (illegal) */
/*TEST(ChooseReferenceTypeTest, lcpcT_rcpT)
{
    using Arg = const double *const &;
    using FArg = double *const &&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_TRUE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}*/

/*      const T *const &    const T *&&         (illegal) */
/*TEST(ChooseReferenceTypeTest, lcpcT_rpcT)
{
    using Arg = const double *const &;
    using FArg = const double *&&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_TRUE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}*/

/*      const T *const &    const T *const &&   (illegal) */
/*TEST(ChooseReferenceTypeTest, lcpcT_rcpcT)
{
    using Arg = const double *const &;
    using FArg = const double *const &&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,false,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_TRUE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}*/

/*      T *&&               T *&&               true                false       false */
TEST(ChooseReferenceTypeTest, rpT_rpT)
{
    using Arg = double *&&;
    using FArg = double *&&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,true,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *&&               T *const &&         true                false       false */
TEST(ChooseReferenceTypeTest, rpT_rcpT)
{
    using Arg = double *&&;
    using FArg = double *const &&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,true,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *&&               const T *&&         true                false       false */
TEST(ChooseReferenceTypeTest, rpT_rpcT)
{
    using Arg = double *&&;
    using FArg = const double *&&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,true,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *&&               const T *const &&   true                false       false */
TEST(ChooseReferenceTypeTest, rpT_rcpcT)
{
    using Arg = double *&&;
    using FArg = const double *const &&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,true,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *&&         T *&&               true                false       false */
TEST(ChooseReferenceTypeTest, rpcT_rpT)
{
    using Arg = const double *&&;
    using FArg = double *&&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,true,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *&&         T *const &&         true                false       false */
TEST(ChooseReferenceTypeTest, rpcT_rcpT)
{
    using Arg = const double *&&;
    using FArg = double *const &&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,true,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *&&         const T *&&         true                false       false */
TEST(ChooseReferenceTypeTest, rpcT_rpcT)
{
    using Arg = const double *&&;
    using FArg = const double *&&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,true,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *&&         const T *const &&   true                false       false */
TEST(ChooseReferenceTypeTest, rpcT_rcpcT)
{
    using Arg = const double *&&;
    using FArg = const double *const &&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,true,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *const &&         T *&&               true                false       false */
TEST(ChooseReferenceTypeTest, rcpT_rpT)
{
    using Arg = double *const &&;
    using FArg = double *&&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,true,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *const &&         T *const &&         true                false       false */
TEST(ChooseReferenceTypeTest, rcpT_rcpT)
{
    using Arg = double *const &&;
    using FArg = double *const &&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,true,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *const &&         const T *&&         true                false       false */
TEST(ChooseReferenceTypeTest, rcpT_rpcT)
{
    using Arg = double *const &&;
    using FArg = const double *&&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,true,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      T *const &&         const T *const &&   true                false       false */
TEST(ChooseReferenceTypeTest, rcpT_rcpcT)
{
    using Arg = double *const &&;
    using FArg = const double *const &&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,true,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *const &&   T *&&               true                false       false */
TEST(ChooseReferenceTypeTest, rcpcT_rpT)
{
    using Arg = const double *const &&;
    using FArg = double *&&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,true,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *const &&   T *const &&         true                false       false */
TEST(ChooseReferenceTypeTest, rcpcT_rcpT)
{
    using Arg = const double *const &&;
    using FArg = double *const &&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,true,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *const &&   const T *&&         true                false       false */
TEST(ChooseReferenceTypeTest, rcpcT_rpcT)
{
    using Arg = const double *const &&;
    using FArg = const double *&&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,true,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
}

/*      const T *const &&   const T *const &&   true                false       false */
TEST(ChooseReferenceTypeTest, rcpcT_rcpcT)
{
    using Arg = const double *const &&;
    using FArg = const double *const &&;
    using test_type = void(*)(mpirpc::internal::choose_reference_type<FArg,Arg>);
    using good_type = void(*)(::mpirpc::pointer_wrapper<double,1,true,false>);
    int status;
    char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status);
    char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status);
    ASSERT_STREQ(good_name, test_name);
    ASSERT_FALSE((mpirpc::internal::can_realloc<FArg,Arg>::value));
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

TEST(WrappedFunctionType, rref_to_array)
{
    using F = void(*)(double(&&)[4]);
    using test_type = mpirpc::internal::wrapped_function_type<F>;
    using good_type = void(*)(double(&&)[4]);
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
    auto res = mpirpc::internal::forward_parameter<FArg,Arg>(3.0);
    using test_type = void(*)(decltype(mpirpc::internal::forward_parameter<FArg,Arg>(3.0)));
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
    auto res = mpirpc::internal::forward_parameter<FArg,Arg>(a);
    using test_type = void(*)(decltype(mpirpc::internal::forward_parameter<FArg,Arg>(a)));
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
    auto res = mpirpc::internal::forward_parameter<FArg,Arg>(std::move(a));
    using test_type = void(*)(decltype(mpirpc::internal::forward_parameter<FArg,Arg>(std::move(a))));
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
    auto res = mpirpc::internal::forward_parameter<FArg,Arg>(3.0f);
    using test_type = void(*)(decltype(mpirpc::internal::forward_parameter<FArg,Arg>(3.0f)));
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
    auto res = mpirpc::internal::forward_parameter<FArg,Arg>(a);
    using test_type = void(*)(decltype(mpirpc::internal::forward_parameter<FArg,Arg>(a)));
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
    auto res = mpirpc::internal::forward_parameter<FArg,Arg>(std::move(a));
    using test_type = void(*)(decltype(mpirpc::internal::forward_parameter<FArg,Arg>(std::move(a))));
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
    auto res = mpirpc::internal::forward_parameter<FArg,Arg>(3.0);
    using test_type = void(*)(decltype(mpirpc::internal::forward_parameter<FArg,Arg>(3.0)));
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
    auto res = mpirpc::internal::forward_parameter<FArg,Arg>(a);
    using test_type = void(*)(decltype(mpirpc::internal::forward_parameter<FArg,Arg>(a)));
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
    auto res = mpirpc::internal::forward_parameter<FArg,Arg>(std::move(a));
    using test_type = void(*)(decltype(mpirpc::internal::forward_parameter<FArg,Arg>(std::move(a))));
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
    auto res = mpirpc::internal::forward_parameter<FArg,Arg>(3.0f);
    using test_type = void(*)(decltype(mpirpc::internal::forward_parameter<FArg,Arg>(3.0f)));
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
    auto res = mpirpc::internal::forward_parameter<FArg,Arg>(a);
    using test_type = void(*)(decltype(mpirpc::internal::forward_parameter<FArg,Arg>(a)));
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
    auto res = mpirpc::internal::forward_parameter<FArg,Arg>(3.0);
    using test_type = void(*)(decltype(mpirpc::internal::forward_parameter<FArg,Arg>(3.0)));
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
    auto res = mpirpc::internal::forward_parameter<FArg,Arg>(std::move(a));
    using test_type = void(*)(decltype(mpirpc::internal::forward_parameter<FArg,Arg>(std::move(a))));
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
    auto res = mpirpc::internal::forward_parameter<FArg,Arg>(3.0f);
    using test_type = void(*)(decltype(mpirpc::internal::forward_parameter<FArg,Arg>(3.0f)));
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
    auto res = mpirpc::internal::forward_parameter<FArg,Arg>(std::move(a));
    using test_type = void(*)(decltype(mpirpc::internal::forward_parameter<FArg,Arg>(std::move(a))));
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
    double* p1 = new double[5]{1.0,2.0,3.0,4.0,5.0};
    using F = void(*)(double*);
    using test_type = void(*)(decltype(mpirpc::internal::forward_parameter<double*,double*&>(p1)));
    using good_type = void(*)(mpirpc::pointer_wrapper<double,1,false,true>);
    using bad_type1 = void(*)(double*);
    using bad_type2 = void(*)(mpirpc::pointer_wrapper<double,1,true,false>);
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
    mpirpc::internal::fn_type_marshaller<F>::marshal(s, ::mpirpc::pointer_wrapper<double>(p1,5));
    double *a1 = new double[5]();
    s >> a1;
    ASSERT_EQ(1.0,a1[0]);
    ASSERT_EQ(2.0,a1[1]);
    ASSERT_EQ(3.0,a1[2]);
    ASSERT_EQ(4.0,a1[3]);
    ASSERT_EQ(5.0,a1[4]);
}

TEST(FnTypeMarshaller, pointer_pointer_wrapper)
{
    using F = void(*)(double*);
    using test_type = typename ::mpirpc::internal::marshaller_function_signature<F,::mpirpc::pointer_wrapper<double,0,false,false>>::applier;
    using good_type = std::function<void(mpirpc::pointer_wrapper<double,0,false,false>)>;
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

TEST(FnTypeMarshaller, pointer_ref_to_array)
{
    using F = void(*)(double*);
    using test_type = typename ::mpirpc::internal::marshaller_function_signature<F,double(&)[5]>::applier;
    using good_type = std::function<void(mpirpc::pointer_wrapper<double,5,false,true>)>;
    using bad_type1 = std::function<void(double(&)[5])>;
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
    double d[5]{1.0,2.0,3.0,4.0,5.0};
    mpirpc::internal::fn_type_marshaller<F>::marshal(s, d);
    double a1[5];
    mpirpc::pointer_wrapper<double,5,false,true> p1(a1);
    s >> p1;
    ASSERT_EQ(1.0,a1[0]);
    ASSERT_EQ(2.0,a1[1]);
    ASSERT_EQ(3.0,a1[2]);
    ASSERT_EQ(4.0,a1[3]);
    ASSERT_EQ(5.0,a1[4]);

    //std::cout << typeid(void(double(&&)[5])).name() << " " << abi::__cxa_demangle(typeid(double(&&)[5]).name(),0,0,&status);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
