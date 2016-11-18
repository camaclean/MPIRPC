#include <gtest/gtest.h>
#include <tuple>
#include <iostream>
#include <typeindex>
#include "../internal/function_attributes.hpp"
#include "../internal/marshalling.hpp"
#include "../internal/orderedcall.hpp"
#include "../internal/parameterstream.hpp"
#include "../internal/unmarshalling.hpp"
#include "../internal/utility.hpp"

#define SINGLE_ARG(...) __VA_ARGS__
#define TEST_REFERENCE_TYPE(Name1,Name2,Argument,FArgument,GoodArgument,Realloc) \
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
        /*std::cout << test_name << " " << good_name << std::endl;*/ \
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

#define TEST_WRAPPED_FUNCTION_TYPE(Name,FunctionType,GoodType) \
    TEST(WrappedFunctionType,Name) { \
        using F = FunctionType; \
        using test_type = mpirpc::internal::wrapped_function_type<F>; \
        using good_type = GoodType; \
        int status; \
        char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status); \
        char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status); \
        ASSERT_STREQ(good_name, test_name); \
        ASSERT_EQ(typeid(good_type),typeid(test_type)); \
    }

#define TEST_STORAGE_TUPLE_TYPE(Name,FunctionType,GoodType) \
    TEST(StorageTupleTest,Name) { \
        using function_type = FunctionType; \
        using test_type = typename mpirpc::internal::wrapped_function_parts<function_type>::storage_tuple_type; \
        using good_type = GoodType; \
        /*test_type();*/ \
        int status; \
        char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status); \
        char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status); \
        ASSERT_STREQ(good_name, test_name); \
        ASSERT_EQ(typeid(good_type),typeid(test_type)); \
    }

#define TEST_FORWARD_PARAMETER_PRVALUE(Name,Argument,FArgument,GoodArgument,TestValue) \
    TEST(ForwardParameterType,Name) { \
        using Arg = Argument; \
        using FArg = FArgument; \
        auto res = mpirpc::internal::forward_parameter<FArg,Arg>(TestValue); \
        using test_type = void(*)(decltype(mpirpc::internal::forward_parameter<FArg,Arg>(TestValue))); \
        using good_type = void(*)(GoodArgument); \
        int status; \
        char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status); \
        char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status); \
        ASSERT_STREQ(good_name, test_name); \
        ASSERT_EQ(typeid(good_type),typeid(test_type)); \
        ASSERT_EQ(TestValue,res); \
    }

#define TEST_FORWARD_PARAMETER_LVALUE(Name,Argument,FArgument,GoodArgument,TestValue) \
    TEST(ForwardParameterType,Name) { \
        using Arg = Argument; \
        using FArg = FArgument; \
        std::remove_reference_t<Arg> a = TestValue; \
        auto res = mpirpc::internal::forward_parameter<FArg,Arg>(a); \
        using test_type = void(*)(decltype(mpirpc::internal::forward_parameter<FArg,Arg>(a))); \
        using good_type = void(*)(GoodArgument); \
        int status; \
        char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status); \
        char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status); \
        ASSERT_STREQ(good_name, test_name); \
        ASSERT_EQ(typeid(good_type),typeid(test_type)); \
        ASSERT_EQ(TestValue,res); \
    }

#define TEST_FORWARD_PARAMETER_XVALUE(Name,Argument,FArgument,GoodArgument,TestValue) \
    TEST(ForwardParameterType,Name) { \
        using Arg = Argument; \
        using FArg = FArgument; \
        std::remove_reference_t<Arg> a = TestValue; \
        auto res = mpirpc::internal::forward_parameter<FArg,Arg>(std::move(a)); \
        using test_type = void(*)(decltype(mpirpc::internal::forward_parameter<FArg,Arg>(a))); \
        using good_type = void(*)(GoodArgument); \
        int status; \
        char* test_name = abi::__cxa_demangle(typeid(test_type).name(),0,0,&status); \
        char* good_name = abi::__cxa_demangle(typeid(good_type).name(),0,0,&status); \
        ASSERT_STREQ(good_name, test_name); \
        ASSERT_EQ(typeid(good_type),typeid(test_type)); \
        ASSERT_EQ(TestValue,res); \
    }

TEST_STORAGE_TUPLE_TYPE(none    ,void(*)()           , std::tuple<>           )
TEST_STORAGE_TUPLE_TYPE(scalar  ,void(*)(int)        , std::tuple<int>        )
TEST_STORAGE_TUPLE_TYPE(cscalar ,void(*)(const int)  , std::tuple<int>        )
TEST_STORAGE_TUPLE_TYPE(pscalar ,void(*)(int*)       , std::tuple<mpirpc::pointer_wrapper<int>>       )
TEST_STORAGE_TUPLE_TYPE(lscalar ,void(*)(int&)       , std::tuple<int>        )
TEST_STORAGE_TUPLE_TYPE(rscalar ,void(*)(int&&)      , std::tuple<int>        )
TEST_STORAGE_TUPLE_TYPE(Ascalar ,void(*)(int[4])     , std::tuple<mpirpc::pointer_wrapper<int>>       )
TEST_STORAGE_TUPLE_TYPE(Alscalar,void(*)(int(&)[4])  , std::tuple<int[4]>  )
TEST_STORAGE_TUPLE_TYPE(Arscalar,void(*)(int(&&)[4]) , std::tuple<int[4]> )
TEST_STORAGE_TUPLE_TYPE(Achar   ,void(*)(char[4])    , std::tuple<char*>      )
TEST_STORAGE_TUPLE_TYPE(Alchar  ,void(*)(char(&)[4]) , std::tuple<char[4]> )
TEST_STORAGE_TUPLE_TYPE(Archar  ,void(*)(char(&&)[4]), std::tuple<char[4]>)


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

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lANT_pT    , double(&)[5]       , double *              , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lANT_cpT   , double(&)[5]       , double *const         , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lANT_pcT   , double(&)[5]       , const double *        , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lANT_cpcT  , double(&)[5]       , const double *const   , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lANT_lpT   , double(&)[5]       , double *&             , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lANT_lcpT  , double(&)[5]       , double *const &       , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lANT_lpcT  , double(&)[5]       , const double *&       , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lANT_lcpcT , double(&)[5]       , const double *const & , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lANT_rpT   , double(&)[5]       , double *&&            , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lANT_rcpT  , double(&)[5]       , double *const &&      , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lANT_rpcT  , double(&)[5]       , const double *&&      , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lANT_rcpcT , double(&)[5]       , const double *const &&, SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)


TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lANcT_pT   , const double(&)[5] , double *              , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lANcT_cpT  , const double(&)[5] , double *const         , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lANcT_pcT  , const double(&)[5] , const double *        , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lANcT_cpcT , const double(&)[5] , const double *const   , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lANcT_lpT  , const double(&)[5] , double *&             , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lANcT_lcpT , const double(&)[5] , double *const &       , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lANcT_lpcT , const double(&)[5] , const double *&       , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lANcT_lcpcT, const double(&)[5] , const double *const & , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lANcT_rpT  , const double(&)[5] , double *&&            , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lANcT_rcpT , const double(&)[5] , double *const &&      , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lANcT_rpcT , const double(&)[5] , const double *&&      , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lANcT_rcpcT, const double(&)[5] , const double *const &&, SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)



TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rANT_pT    , double(&&)[5]      , double *              , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rANT_cpT   , double(&&)[5]      , double *const         , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rANT_pcT   , double(&&)[5]      , const double *        , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rANT_cpcT  , double(&&)[5]      , const double *const   , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rANT_lpT   , double(&&)[5]      , double *&             , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rANT_lcpT  , double(&&)[5]      , double *const &       , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rANT_lpcT  , double(&&)[5]      , const double *&       , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rANT_lcpcT , double(&&)[5]      , const double *const & , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rANT_rpT   , double(&&)[5]      , double *&&            , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rANT_rcpT  , double(&&)[5]      , double *const &&      , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rANT_rpcT  , double(&&)[5]      , const double *&&      , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rANT_rcpcT , double(&&)[5]      , const double *const &&, SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)


TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rANcT_pT   , const double(&&)[5], double *              , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rANcT_cpT  , const double(&&)[5], double *const         , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rANcT_pcT  , const double(&&)[5], const double *        , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rANcT_cpcT , const double(&&)[5], const double *const   , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rANcT_lpT  , const double(&&)[5], double *&             , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rANcT_lcpT , const double(&&)[5], double *const &       , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rANcT_lpcT , const double(&&)[5], const double *&       , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rANcT_lcpcT, const double(&&)[5], const double *const & , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rANcT_rpT  , const double(&&)[5], double *&&            , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rANcT_rcpT , const double(&&)[5], double *const &&      , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rANcT_rpcT , const double(&&)[5], const double *&&      , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rANcT_rcpcT, const double(&&)[5], const double *const &&, SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)



TEST_REFERENCE_SCALAR_TYPE(ChooseReferenceTypeTest, d_Cd  , double        , const double  , const double&&, false, 3.0 )
TEST_REFERENCE_SCALAR_TYPE(ChooseReferenceTypeTest, Cd_d  , const double  , double        , const double&&, false, 3.0 )
TEST_REFERENCE_SCALAR_TYPE(ChooseReferenceTypeTest, Cd_Cd , const double  , const double  , const double&&, false, 3.0 )

TEST_REFERENCE_SCALAR_TYPE(ChooseReferenceTypeTest, T_T  , double  , double  , double&&, false, 3.0 )
TEST_REFERENCE_SCALAR_TYPE(ChooseReferenceTypeTest, Tl_T , double& , double  , double& , false, 3.0 )
TEST_REFERENCE_SCALAR_TYPE(ChooseReferenceTypeTest, Tr_T , double&&, double  , double&&, false, 3.0 )
TEST_REFERENCE_SCALAR_TYPE(ChooseReferenceTypeTest, T_U  , float   , double  , double  , false, 3.0f)
TEST_REFERENCE_SCALAR_TYPE(ChooseReferenceTypeTest, Tl_U , float&  , double  , double  , false, 3.0f)
TEST_REFERENCE_SCALAR_TYPE(ChooseReferenceTypeTest, Tr_U , float&& , double  , double  , false, 3.0f)
TEST_REFERENCE_SCALAR_TYPE(ChooseReferenceTypeTest, T_Tl , double  , double& , double&&, false, 3.0 )
TEST_REFERENCE_SCALAR_TYPE(ChooseReferenceTypeTest, Tl_Tl, double& , double& , double& , false, 3.0 )
TEST_REFERENCE_SCALAR_TYPE(ChooseReferenceTypeTest, Tr_Tl, double&&, double& , double&&, false, 3.0 )
TEST_REFERENCE_SCALAR_TYPE(ChooseReferenceTypeTest, T_Ul , float   , double& , double  , false, 3.0f)
TEST_REFERENCE_SCALAR_TYPE(ChooseReferenceTypeTest, Tl_Ul, float&  , double& , double  , false, 3.0f)
TEST_REFERENCE_SCALAR_TYPE(ChooseReferenceTypeTest, Tr_Ul, float&& , double& , double  , false, 3.0f)
TEST_REFERENCE_SCALAR_TYPE(ChooseReferenceTypeTest, T_Tr , double  , double&&, double&&, false, 3.0 )
TEST_REFERENCE_SCALAR_TYPE(ChooseReferenceTypeTest, Tr_Tr, double&&, double&&, double&&, false, 3.0 )
TEST_REFERENCE_SCALAR_TYPE(ChooseReferenceTypeTest, T_Ur , float   , double&&, double  , false, 3.0f)
TEST_REFERENCE_SCALAR_TYPE(ChooseReferenceTypeTest, Tr_Ur, float&& , double&&, double  , false, 3.0f)
//TEST_REFERENCE_SCALAR_TYPE(ChooseReferenceTypeTest, Tl_Tr, double& , double&&, double& , false, 3.0 )
//TEST_REFERENCE_SCALAR_TYPE(ChooseReferenceTypeTest, Tl_Ur, float&  , double&&, double  , false, 3.0f)


TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, pT_pT      , double *              , double *              , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, pT_cpT     , double *              , double *const         , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, pT_pcT     , double *              , const double *        , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, pT_cpcT    , double *              , const double *const   , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, pcT_pT     , const double *        , double *              , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, pcT_cpT    , const double *        , double *const         , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, pcT_pcT    , const double *        , const double *        , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, pcT_cpcT   , const double *        , const double *const   , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, cpT_pT     , double *const         , double *              , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, cpT_cpT    , double *const         , double *const         , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, cpT_pcT    , double *const         , const double *        , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, cpT_cpcT   , double *const         , const double *const   , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, cpcT_pT    , const double *const   , double *              , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, cpcT_cpT   , const double *const   , double *const         , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, cpcT_pcT   , const double *const   , const double *        , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, cpcT_cpcT  , const double *const   , const double *const   , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)


TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lpT_pT     , double *&             , double *              , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lpT_cpT    , double *&             , double *const         , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lpT_pcT    , double *&             , const double *        , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lpT_cpcT   , double *&             , const double *const   , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lpcT_pT    , const double *&       , double *              , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lpcT_cpT   , const double *&       , double *const         , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lpcT_pcT   , const double *&       , const double *        , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lpcT_cpcT  , const double *&       , const double *const   , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lcpT_pT    , double *const &       , double *              , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lcpT_cpT   , double *const &       , double *const         , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lcpT_pcT   , double *const &       , const double *        , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lcpT_cpcT  , double *const &       , const double *const   , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lcpcT_pT   , const double *const & , double *              , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lcpcT_cpT  , const double *const & , double *const         , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lcpcT_pcT  , const double *const & , const double *        , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lcpcT_cpcT , const double *const & , const double *const   , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)


TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rpT_pT     , double *&&            , double *              , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rpT_cpT    , double *&&            , double *const         , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rpT_pcT    , double *&&            , const double *        , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rpT_cpcT   , double *&&            , const double *const   , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rpcT_pT    , const double *&&      , double *              , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rpcT_cpT   , const double *&&      , double *const         , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rpcT_pcT   , const double *&&      , const double *        , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rpcT_cpcT  , const double *&&      , const double *const   , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rcpT_pT    , double *const &&      , double *              , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rcpT_cpT   , double *const &&      , double *const         , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rcpT_pcT   , double *const &&      , const double *        , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rcpT_cpcT  , double *const &&      , const double *const   , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rcpcT_pT   , const double *const &&, double *              , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rcpcT_cpT  , const double *const &&, double *const         , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rcpcT_pcT  , const double *const &&, const double *        , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rcpcT_cpcT , const double *const &&, const double *const   , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)



TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, pT_lpT     , double *              , double *&             , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, pT_lcpT    , double *              , double *const &       , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, pT_lpcT    , double *              , const double *&       , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, pT_lcpcT   , double *              , const double *const & , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, pcT_lpT    , const double *        , double *&             , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, pcT_lcpT   , const double *        , double *const &       , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, pcT_lpcT   , const double *        , const double *&       , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, pcT_lcpcT  , const double *        , const double *const & , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, cpT_lpT    , double *const         , double * &            , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, cpT_lcpT   , double *const         , double *const &       , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, cpT_lpcT   , double *const         , const double * &      , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, cpT_lcpcT  , double *const         , const double *const & , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, cpcT_lpT   , const double *const   , double *&             , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, cpcT_lcpT  , const double *const   , double *const &       , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, cpcT_lpcT  , const double *const   , const double *&       , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, cpcT_lcpcT , const double *const   , const double *const & , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)


TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lpT_lpT    , double *&             , double *&             , SINGLE_ARG(mpirpc::pointer_wrapper<double>),true )
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lpT_lcpT   , double *&             , double *const &       , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lpT_lpcT   , double *&             , const double *&       , SINGLE_ARG(mpirpc::pointer_wrapper<double>),true )
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lpT_lcpcT  , double *&             , const double *const & , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lpcT_lpT   , const double *&       , double *&             , SINGLE_ARG(mpirpc::pointer_wrapper<double>),true )
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lpcT_lcpT  , const double *&       , double *const &       , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lpcT_lpcT  , const double *&       , const double *&       , SINGLE_ARG(mpirpc::pointer_wrapper<double>),true )
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lpcT_lcpcT , const double *&       , const double *const & , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lcpT_lpT   , double *const &       , double *&             , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lcpT_lcpT  , double *const &       , double *const &       , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lcpT_lpcT  , double *const &       , const double *&       , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lcpT_lcpcT , double *const &       , const double *const & , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lcpcT_lpT  , const double *const & , double *&             , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lcpcT_lcpT , const double *const & , double *const &       , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lcpcT_lpcT , const double *const & , const double *&       , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lcpcT_lcpcT, const double *const & , const double *const & , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)


TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rpT_lpT    , double *&&            , double *&             , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rpT_lcpT   , double *&&            , double *const &       , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rpT_lpcT   , double *&&            , const double *&       , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rpT_lcpcT  , double *&&            , const double *const & , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rpcT_lpT   , const double *&&      , double *&             , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rpcT_lcpT  , const double *&&      , double *const &       , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rpcT_lpcT  , const double *&&      , const double *&       , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rpcT_lcpcT , const double *&&      , const double *const & , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rcpT_lpT   , double *const &&      , double *&             , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rcpT_lcpT  , double *const &&      , double *const &       , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rcpT_lpcT  , double *const &&      , const double *&       , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rcpT_lcpcT , double *const &&      , const double *const & , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rcpcT_lpT  , const double *const &&, double *&             , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rcpcT_lcpT , const double *const &&, double *const &       , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rcpcT_lpcT , const double *const &&, const double *&       , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rcpcT_lcpcT, const double *const &&, const double *const & , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)



TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, pT_rpT     , double *              , double *&&            , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, pT_rcpT    , double *              , double *const &&      , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, pT_rpcT    , double *              , const double *&&      , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, pT_rcpcT   , double *              , const double *const &&, SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, pcT_rpT    , const double *        , double *&&            , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, pcT_rcpT   , const double *        , double *const &&      , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, pcT_rpcT   , const double *        , const double *&&      , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, pcT_rcpcT  , const double *        , const double *const &&, SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, cpT_rpT    , double *const         , double *&&            , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, cpT_rcpT   , double *const         , double *const &&      , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, cpT_rpcT   , double *const         , const double *&&      , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, cpT_rcpcT  , double *const         , const double *const &&, SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, cpcT_rpT   , const double *const   , double *&&            , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, cpcT_rcpT  , const double *const   , double *const &&      , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, cpcT_rpcT  , const double *const   , const double *&&      , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, cpcT_rcpcT , const double *const   , const double *const &&, SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)


TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rpT_rpT    , double *&&            , double *&&            , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rpT_rcpT   , double *&&            , double *const &&      , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rpT_rpcT   , double *&&            , const double *&&      , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rpT_rcpcT  , double *&&            , const double *const &&, SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rpcT_rpT   , const double *&&      , double *&&            , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rpcT_rcpT  , const double *&&      , double *const &&      , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rpcT_rpcT  , const double *&&      , const double *&&      , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rpcT_rcpcT , const double *&&      , const double *const &&, SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rcpT_rpT   , double *const &&      , double *&&            , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rcpT_rcpT  , double *const &&      , double *const &&      , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rcpT_rpcT  , double *const &&      , const double *&&      , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rcpT_rcpcT , double *const &&      , const double *const &&, SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rcpcT_rpT  , const double *const &&, double *&&            , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rcpcT_rcpT , const double *const &&, double *const &&      , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rcpcT_rpcT , const double *const &&, const double *&&      , SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rcpcT_rcpcT, const double *const &&, const double *const &&, SINGLE_ARG(mpirpc::pointer_wrapper<double>),false)




TEST_WRAPPED_FUNCTION_TYPE(basic                  , void(*)()                , void(*)()                                   )
TEST_WRAPPED_FUNCTION_TYPE(primitive              , void(*)(double)          , void(*)(double)                             )
TEST_WRAPPED_FUNCTION_TYPE(ref_to_primitive       , void(*)(double&)         , void(*)(double&)                            )
TEST_WRAPPED_FUNCTION_TYPE(rref_to_primitive      , void(*)(double&&)        , void(*)(double&&)                           )
TEST_WRAPPED_FUNCTION_TYPE(const_primitive        , void(*)(const double)    , void(*)(const double)                       )
TEST_WRAPPED_FUNCTION_TYPE(return_primitive       , int(*)()                 , int(*)()                                    )
TEST_WRAPPED_FUNCTION_TYPE(pointer                , void(*)(double*)         , void(*)(mpirpc::pointer_wrapper<double>)    )
TEST_WRAPPED_FUNCTION_TYPE(return_pointer         , int*(*)()                , int*(*)()                                   )
TEST_WRAPPED_FUNCTION_TYPE(array                  , void(*)(double[4])       , void(*)(mpirpc::pointer_wrapper<double>)    )
TEST_WRAPPED_FUNCTION_TYPE(pointer_to_array       , void(*)(double(*)[4])    , void(*)(mpirpc::pointer_wrapper<double[4]>) )
TEST_WRAPPED_FUNCTION_TYPE(ref_to_array           , void(*)(double(&)[4])    , void(*)(double(&)[4])                       )
TEST_WRAPPED_FUNCTION_TYPE(rref_to_array          , void(*)(double(&&)[4])   , void(*)(double(&&)[4])                      )
TEST_WRAPPED_FUNCTION_TYPE(return_pointer_to_array, int(*(*)())[4]           , int(*(*)())[4]                              )
TEST_WRAPPED_FUNCTION_TYPE(return_ref_to_array    , int(&(*)())[4]           , int(&(*)())[4]                              )
TEST_WRAPPED_FUNCTION_TYPE(return_rref_to_array   , int(&&(*)())[4]          , int(&&(*)())[4]                             )
TEST_WRAPPED_FUNCTION_TYPE(pchar                  , void(*)(char*)           , void(*)(char*)                              )
TEST_WRAPPED_FUNCTION_TYPE(pcchar                 , void(*)(const char*)     , void(*)(const char*)                        )
TEST_WRAPPED_FUNCTION_TYPE(Achar                  , void(*)(char(&)[4])      , void(*)(char(&)[4])                         )
TEST_WRAPPED_FUNCTION_TYPE(Acchar                 , void(*)(const char(&)[4]), void(*)(const char(&)[4])                   )



TEST_FORWARD_PARAMETER_PRVALUE(T_T  , double  , double  , double&&, 3.0)
TEST_FORWARD_PARAMETER_LVALUE (lT_T , double& , double  , double& , 3.0)
TEST_FORWARD_PARAMETER_XVALUE (rT_T , double&&, double  , double&&, 3.0)
TEST_FORWARD_PARAMETER_PRVALUE(T_U  , float   , double  , double  , 3.0)
TEST_FORWARD_PARAMETER_LVALUE (lT_U , float&  , double  , double  , 3.0)
TEST_FORWARD_PARAMETER_XVALUE (rT_U , float&& , double  , double  , 3.0)
TEST_FORWARD_PARAMETER_PRVALUE(T_lT , double  , double& , double&&, 3.0)
TEST_FORWARD_PARAMETER_LVALUE (lT_lT, double& , double& , double& , 3.0)
TEST_FORWARD_PARAMETER_XVALUE (rT_lT, double&&, double& , double&&, 3.0)
TEST_FORWARD_PARAMETER_PRVALUE(T_lU , float   , double& , double  , 3.0)
TEST_FORWARD_PARAMETER_LVALUE (lT_lU, float&  , double& , double  , 3.0)
TEST_FORWARD_PARAMETER_XVALUE (rT_lU, float&& , double& , double  , 3.0)
TEST_FORWARD_PARAMETER_PRVALUE(T_rT , double  , double&&, double&&, 3.0)
//TEST_FORWARD_PARAMETER_LVALUE (lT_rT, double& , double&&, double& , 3.0)
TEST_FORWARD_PARAMETER_XVALUE (rT_rT, double&&, double&&, double&&, 3.0)
TEST_FORWARD_PARAMETER_PRVALUE(T_rU , float   , double&&, double  , 3.0)
//TEST_FORWARD_PARAMETER_LVALUE (lT_rU, float&  , double&&, double  , 3.0)
TEST_FORWARD_PARAMETER_XVALUE (rT_rU, float&& , double&&, double  , 3.0)

template<typename T>
struct unmarshal_into_tuple_helper;

template<typename...Ts>
struct unmarshal_into_tuple_helper<std::tuple<Ts...>>
{
    template<typename Allocator, typename Stream>
    static std::tuple<Ts...> unmarshal(Allocator &a, Stream &s)
    {
        return mpirpc::internal::tuple_unmarshaller_remote<Ts...>::unmarshal(a,s);
    }
};


#define TEST_FN_TYPE_MARSHALLER(Name, FunctionType, Arguments...) \
    TEST(FnTypeMarshaller,Name) { \
        using F = FunctionType; \
        using StorageTupleType = typename ::mpirpc::internal::wrapped_function_parts<F>::storage_tuple_type; \
        auto argument_tuple = std::make_tuple(Arguments); \
        mpirpc::parameter_stream s; \
        mpirpc::internal::fn_type_marshaller<F>::marshal(s, Arguments); \
        std::allocator<void> a; \
        StorageTupleType st(unmarshal_into_tuple_helper<StorageTupleType>::unmarshal(a,s)); \
        /*std::cout << abi::__cxa_demangle(typeid(StorageTupleType).name(),0,0,0) << std::endl; \
        std::cout << abi::__cxa_demangle(typeid(argument_tuple).name(),0,0,0) << std::endl; */ \
        ASSERT_EQ(argument_tuple,st); \
    }

double test_double = 3.0;
float  test_float  = 3.0f;
int    test_int    = 3;
TEST_FN_TYPE_MARSHALLER(basic_d,void(*)(double)                , 3.0                                  )
TEST_FN_TYPE_MARSHALLER(basic_f,void(*)(float)                 , 3.0f                                 )
TEST_FN_TYPE_MARSHALLER(basic_i,void(*)(int)                   , 3                                    )
TEST_FN_TYPE_MARSHALLER(Cd_d   ,void(*)(const double)          , 3.0                                  )
TEST_FN_TYPE_MARSHALLER(Cd_Cd  ,void(*)(const double)          , (const double) 3.0                   )
TEST_FN_TYPE_MARSHALLER(order_2,void(*)(int,int)               , 3          , 4                       )
TEST_FN_TYPE_MARSHALLER(order_3,void(*)(int,int,int)           , 3          , 4          , 5          )
TEST_FN_TYPE_MARSHALLER(T_T    ,void(*)(double,float,int)      , 3.0        , 3.0f       , 3          )
TEST_FN_TYPE_MARSHALLER(lT_T   ,void(*)(double,float,int)      , test_double, test_float , test_int   )
TEST_FN_TYPE_MARSHALLER(T_U    ,void(*)(double,float,int)      , 3          , 3.0        , 3.0f       )
TEST_FN_TYPE_MARSHALLER(lT_U   ,void(*)(double,float,int)      , test_int   , test_double, test_float )
TEST_FN_TYPE_MARSHALLER(T_lT   ,void(*)(double&,float&,int&)   , 3.0        , 3.0f       , 3          )
TEST_FN_TYPE_MARSHALLER(lT_lT  ,void(*)(double&,float&,int&)   , test_double, test_float , test_int   )
TEST_FN_TYPE_MARSHALLER(T_lU   ,void(*)(double&,float&,int&)   , 3          , 3.0        , 3.0f       )
TEST_FN_TYPE_MARSHALLER(lT_lU  ,void(*)(double&,float&,int&)   , test_int   , test_double, test_float )
TEST_FN_TYPE_MARSHALLER(T_rT   ,void(*)(double&&,float&&,int&&), 3.0        , 3.0f       , 3          )
TEST_FN_TYPE_MARSHALLER(T_rU   ,void(*)(double&&,float&&,int&&), 3          , 3.0        , 3.0f       )

TEST(FnTypeMarshaller, p5T_pT)
{
    double* p1 = new double[5]{5.0,6.0,7.0,8.0,9.0};
    using F = void(*)(double*);
    using StorageTupleType = typename ::mpirpc::internal::wrapped_function_parts<F>::storage_tuple_type;
    mpirpc::parameter_stream s;
    mpirpc::internal::fn_type_marshaller<F>::marshal(s, mpirpc::pointer_wrapper<double>(p1,5));
    std::allocator<void> a;
    StorageTupleType st(mpirpc::internal::tuple_unmarshaller_remote<double*>::unmarshal(a,s));
    double *a1 = (double*) std::get<0>(st);
    for (std::size_t i = 0; i < 5; ++i)
        ASSERT_EQ(p1[i],a1[i]);
    using NewAllocatorType = typename std::allocator_traits<std::allocator<void>>::template rebind_alloc<double>;
    NewAllocatorType na(a);
    std::allocator_traits<std::allocator<double>>::deallocate(na,a1,5);
    delete[] p1;
}

TEST(FnTypeMarshaller, p5T_lpT)
{
    double* p1 = new double[5]{5.0,6.0,7.0,8.0,9.0};
    using F = void(*)(double*&);
    using StorageTupleType = typename ::mpirpc::internal::wrapped_function_parts<F>::storage_tuple_type;
    mpirpc::parameter_stream s;
    mpirpc::internal::fn_type_marshaller<F>::marshal(s, mpirpc::pointer_wrapper<double>(p1,5));
    std::allocator<void> a;
    StorageTupleType st(mpirpc::internal::tuple_unmarshaller_remote<double*&>::unmarshal(a,s));
    double *a1 = (double*) std::get<0>(st);
    for (std::size_t i = 0; i < 5; ++i)
        ASSERT_EQ(p1[i],a1[i]);
    using NewAllocatorType = typename std::allocator_traits<std::allocator<void>>::template rebind_alloc<double>;
    NewAllocatorType na(a);
    std::allocator_traits<std::allocator<double>>::deallocate(na,a1,5);
    delete[] p1;
}

TEST(FnTypeMarshaller, p5T_rpT)
{
    double* p1 = new double[5]{5.0,6.0,7.0,8.0,9.0};
    using F = void(*)(double*&&);
    using StorageTupleType = typename ::mpirpc::internal::wrapped_function_parts<F>::storage_tuple_type;
    mpirpc::parameter_stream s;
    mpirpc::internal::fn_type_marshaller<F>::marshal(s, mpirpc::pointer_wrapper<double>(p1,5));
    std::allocator<void> a;
    StorageTupleType st(mpirpc::internal::tuple_unmarshaller_remote<double*&&>::unmarshal(a,s));
    double *a1 = (double*) std::get<0>(st);
    for (std::size_t i = 0; i < 5; ++i)
        ASSERT_EQ(p1[i],a1[i]);
    using NewAllocatorType = typename std::allocator_traits<std::allocator<void>>::template rebind_alloc<double>;
    NewAllocatorType na(a);
    std::allocator_traits<std::allocator<double>>::deallocate(na,a1,5);
    delete[] p1;
}

TEST(FnTypeMarshaller, la5T_pT)
{
    using F = void(*)(double*);
    using StorageTupleType = typename ::mpirpc::internal::wrapped_function_parts<F>::storage_tuple_type;
    mpirpc::parameter_stream s;
    double p1[5]{5.0,6.0,7.0,8.0,9.0};
    //auto t = mpirpc::internal::autowrap(p1);
    //std::cout << t.size() << std::endl;
    //std::cout << "StorageTupleType: " << abi::__cxa_demangle(typeid(StorageTupleType).name(),0,0,0) << std::endl; \
    //std::cout << abi::__cxa_demangle(typeid(argument_tuple).name(),0,0,0) << std::endl;
    mpirpc::internal::fn_type_marshaller<F>::marshal(s, p1);
    std::allocator<void> a;
    StorageTupleType st(mpirpc::internal::tuple_unmarshaller_remote<mpirpc::pointer_wrapper<double>>::unmarshal(a,s));
    double *a1 = (double*) std::get<0>(st);
    for (std::size_t i = 0; i < 5; ++i)
        ASSERT_EQ(p1[i],a1[i]);
    using NewAllocatorType = typename std::allocator_traits<std::allocator<void>>::template rebind_alloc<double>;
    NewAllocatorType na(a);
    std::allocator_traits<std::allocator<double>>::deallocate(na,a1,5);
}

TEST(FnTypeMarshaller, la5T_lpT)
{
    using F = void(*)(double*&);
    using StorageTupleType = typename ::mpirpc::internal::wrapped_function_parts<F>::storage_tuple_type;
    mpirpc::parameter_stream s;
    double p1[5]{5.0,6.0,7.0,8.0,9.0};
    //auto t = mpirpc::internal::autowrap(p1);
    //std::cout << t.size() << std::endl;
    //std::cout << "StorageTupleType: " << abi::__cxa_demangle(typeid(StorageTupleType).name(),0,0,0) << std::endl; \
    //std::cout << abi::__cxa_demangle(typeid(argument_tuple).name(),0,0,0) << std::endl;
    mpirpc::internal::fn_type_marshaller<F>::marshal(s, p1);
    std::allocator<void> a;
    StorageTupleType st(mpirpc::internal::tuple_unmarshaller_remote<double*&>::unmarshal(a,s));
    double *a1 = (double*) std::get<0>(st);
    for (std::size_t i = 0; i < 5; ++i)
        ASSERT_EQ(p1[i],a1[i]);
    using NewAllocatorType = typename std::allocator_traits<std::allocator<void>>::template rebind_alloc<double>;
    NewAllocatorType na(a);
    std::allocator_traits<std::allocator<double>>::deallocate(na,a1,5);
}

/*TEST(FnTypeMarshaller, la5T_la5T)
{
    using F = void(*)(double(&)[5]);
    using StorageTupleType = typename ::mpirpc::internal::wrapped_function_parts<F>::storage_tuple_type;
    mpirpc::parameter_stream s;
    double p1[5]{5.0,6.0,7.0,8.0,9.0};
    mpirpc::internal::fn_type_marshaller<F>::marshal(s, p1);
    std::allocator<void> a;
    StorageTupleType st(mpirpc::internal::tuple_unmarshaller_remote<double(&)[5]>::unmarshal(a,s));
    std::tuple<double(&)[5]> test_tuple(p1);
    double *a1 = std::get<0>(st);
    for (std::size_t i = 0; i < 5; ++i)
        ASSERT_EQ(p1[i],a1[i]);
    using NewAllocatorType = typename std::allocator_traits<std::allocator<void>>::template rebind_alloc<double>;
    NewAllocatorType na(a);
    std::allocator_traits<std::allocator<double>>::deallocate(na,a1,5);
}

TEST(FnTypeMarshaller, ra5T_la5T)
{
    using F = void(*)(double(&)[5]);
    using StorageTupleType = typename ::mpirpc::internal::wrapped_function_parts<F>::storage_tuple_type;
    mpirpc::parameter_stream s;
    double p1[5]{5.0,6.0,7.0,8.0,9.0};
    mpirpc::internal::fn_type_marshaller<F>::marshal(s, std::move(p1));
    std::allocator<void> a;
    StorageTupleType st(mpirpc::internal::tuple_unmarshaller_remote<double(&)[5]>::unmarshal(a,s));
    double *a1 = std::get<0>(st);
    for (std::size_t i = 0; i < 5; ++i)
        ASSERT_EQ(p1[i],a1[i]);
    using NewAllocatorType = typename std::allocator_traits<std::allocator<void>>::template rebind_alloc<double>;
    NewAllocatorType na(a);
    std::allocator_traits<std::allocator<double>>::deallocate(na,a1,5);
}

TEST(FnTypeMarshaller, ra5T_ra5T)
{
    using F = void(*)(double(&&)[5]);
    using StorageTupleType = typename ::mpirpc::internal::wrapped_function_parts<F>::storage_tuple_type;
    mpirpc::parameter_stream s;
    double p1[5]{5.0,6.0,7.0,8.0,9.0};
    mpirpc::internal::fn_type_marshaller<F>::marshal(s, std::move(p1));
    std::allocator<void> a;
    StorageTupleType st(mpirpc::internal::tuple_unmarshaller_remote<double(&&)[5]>::unmarshal(a,s));
    double *a1 = std::get<0>(st);
    for (std::size_t i = 0; i < 5; ++i)
        ASSERT_EQ(p1[i],a1[i]);
    using NewAllocatorType = typename std::allocator_traits<std::allocator<void>>::template rebind_alloc<double>;
    NewAllocatorType na(a);
    std::allocator_traits<std::allocator<double>>::deallocate(na,a1,5);
}

TEST(FnTypeMarshaller, la5T_la5cT)
{
    using F = void(*)(const double(&)[5]);
    using StorageTupleType = typename ::mpirpc::internal::wrapped_function_parts<F>::storage_tuple_type;
    mpirpc::parameter_stream s;
    double p1[5]{5.0,6.0,7.0,8.0,9.0};
    mpirpc::internal::fn_type_marshaller<F>::marshal(s, p1);
    std::allocator<void> a;
    StorageTupleType st(mpirpc::internal::tuple_unmarshaller_remote<const double(&)[5]>::unmarshal(a,s));
    const double *a1 = std::get<0>(st);
    for (std::size_t i = 0; i < 5; ++i)
        ASSERT_EQ(p1[i],a1[i]);
    using NewAllocatorType = typename std::allocator_traits<std::allocator<void>>::template rebind_alloc<double>;
    NewAllocatorType na(a);
    std::allocator_traits<std::allocator<double>>::deallocate(na,(double*) a1,5);
}

TEST(FnTypeMarshaller, la5cT_la5cT)
{
    using F = void(*)(const double(&)[5]);
    using StorageTupleType = typename ::mpirpc::internal::wrapped_function_parts<F>::storage_tuple_type;
    mpirpc::parameter_stream s;
    const double p1[5]{5.0,6.0,7.0,8.0,9.0};
    mpirpc::internal::fn_type_marshaller<F>::marshal(s, p1);
    std::allocator<void> a;
    StorageTupleType st(mpirpc::internal::tuple_unmarshaller_remote<const double(&)[5]>::unmarshal(a,s));
    const double *a1 = std::get<0>(st);
    for (std::size_t i = 0; i < 5; ++i)
        ASSERT_EQ(p1[i],a1[i]);
    using NewAllocatorType = typename std::allocator_traits<std::allocator<void>>::template rebind_alloc<double>;
    NewAllocatorType na(a);
    std::allocator_traits<std::allocator<double>>::deallocate(na,(double*) a1,5);
}*/

void ordered_call_basic_test() {}

TEST(OrderedCallTest, basic)
{
    std::allocator<void> a;
    mpirpc::internal::ordered_call<void(*)(),std::allocator<void>> oc(&ordered_call_basic_test, a);
    oc();
}


class Foo
{
public:
    Foo() { ++ordered_call_class_Foo_value; }
    ~Foo() { ++ordered_call_class_Foo_value; }
    static int ordered_call_class_Foo_value;
};

int Foo::ordered_call_class_Foo_value = 0;

void ordered_call_class_Foo_test(Foo& a)
{
}

void ordered_call_class_pFoo_test(Foo* a)
{
}

TEST(OrderedCallTest, class_Foo)
{
    std::allocator<void> a;
    Foo p1;
    {
        mpirpc::internal::ordered_call<decltype(&ordered_call_class_Foo_test),std::allocator<void>> oc{&ordered_call_class_Foo_test, a, p1};
        oc();
    } //make ordered_call go out of scope
    ASSERT_EQ(2, Foo::ordered_call_class_Foo_value);
}

TEST(OrderedCallTest, class_pFoo)
{
    Foo::ordered_call_class_Foo_value = 0;
    std::allocator<void> a;
    Foo *p1 = new Foo();
    {
        mpirpc::pointer_wrapper<Foo> a1(p1,1);
        mpirpc::internal::ordered_call<decltype(&ordered_call_class_pFoo_test),std::allocator<void>> oc{&ordered_call_class_pFoo_test, a, std::move(a1)};
        oc();
    } //make ordered_call go out of scope
    ASSERT_EQ(2, Foo::ordered_call_class_Foo_value);
}

void ordered_call_ref_array(int(&v)[3][4])
{
    for (std::size_t i = 0; i < 3; ++i)
    {
        for (std::size_t j = 0; j < 4; ++j)
        {
            std:: cout << v[i][j] << " ";
            ++v[i][j];
        }
        std::cout << std::endl;
    }
}

/*TEST(OrderedCallTest, ref_array)
{
    using T = int(&)[3][4];
    int *p = new int[3*4]{1,2,3,4,5,6,7,8,9,10,11,12};
    T a1 = (T) *p;
    std::allocator<void> a;
    {
        mpirpc::internal::ordered_call<decltype(&ordered_call_ref_array),std::allocator<void>> oc{&ordered_call_ref_array, a, a1};
        oc();
        int(&v)[3][4] = std::get<0>(oc.args_tuple);
        ASSERT_EQ(2 ,v[0][0]);
        ASSERT_EQ(3 ,v[0][1]);
        ASSERT_EQ(4 ,v[0][2]);
        ASSERT_EQ(5 ,v[0][3]);
        ASSERT_EQ(6 ,v[1][0]);
        ASSERT_EQ(7 ,v[1][1]);
        ASSERT_EQ(8 ,v[1][2]);
        ASSERT_EQ(9 ,v[1][3]);
        ASSERT_EQ(10,v[2][0]);
        ASSERT_EQ(11,v[2][1]);
        ASSERT_EQ(12,v[2][2]);
        ASSERT_EQ(13,v[2][3]);
    } //make ordered_call go out of scope
}*/

void ordered_call_rref_array(int(&&v)[3][4])
{
    /*for (std::size_t i = 0; i < 3; ++i)
    {
        for (std::size_t j = 0; j < 4; ++j)
        {
            std:: cout << v[i][j] << " ";
            ++v[i][j];
        }
        std::cout << std::endl;
    }*/
    ASSERT_EQ(2 ,v[0][0]);
    ASSERT_EQ(3 ,v[0][1]);
    ASSERT_EQ(4 ,v[0][2]);
    ASSERT_EQ(5 ,v[0][3]);
    ASSERT_EQ(6 ,v[1][0]);
    ASSERT_EQ(7 ,v[1][1]);
    ASSERT_EQ(8 ,v[1][2]);
    ASSERT_EQ(9 ,v[1][3]);
    ASSERT_EQ(10,v[2][0]);
    ASSERT_EQ(11,v[2][1]);
    ASSERT_EQ(12,v[2][2]);
    ASSERT_EQ(13,v[2][3]);
}

class Bar
{
public:
    Bar() : moved(false) {}
    Bar(const Bar&) = delete;
    Bar(Bar&) = delete;
    Bar(Bar&&) { moved = true; }
    bool moved;
};

void ordered_call_rref_array_Bar(Bar(&&v)[1])
{
    //std::cout << v[0].moved << " " << v << std::endl;
    /*
     * This should be moving the array location, not the elements of the array itself
     */
    ASSERT_FALSE(v[0].moved);
}

/*TEST(OrderedCallTest, rref_array_Bar)
{
    using T = Bar(&&)[1];
    Bar *p = new Bar[1]();
    Bar t[1]{};
    std::cout << &t << std::endl;
    std::cout << p << std::endl;
    ordered_call_rref_array_Bar(std::move(t));
    //T a1 = std::move(t);
    T a1 = (T) *p;
    std::allocator<void> a;
    {
        mpirpc::internal::ordered_call<decltype(&ordered_call_rref_array_Bar),std::allocator<void>> oc{&ordered_call_rref_array_Bar, a, a1};
        oc();
    } //make ordered_call go out of scope
}*/

/*template<typename Int, Int...Is>
struct get_last_integer_sequence;

template<typename Int, Int I1, Int I2, Int...Is>
struct get_last_integer_sequence<Int,I1,I2,Is...>
{
    constexpr static Int last = get_last_integer_sequence<Int,I2,Is...>::last;
};

template<typename Int, Int I>
struct get_last_integer_sequence<Int,I>
{
    constexpr static Int last = I;
};*/

class Foo2
{
public:
    void foo(double d, int i, float f, bool b, int(&ai)[4], double(&ad)[3], float(&af)[5], double *pd)
    {
        int ai2[4]{2,4,6,8};
        double ad2[3]{4.6,8.2,9.1};
        float af2[5]{0.2f,2.4f,1.4f,8.7f,3.14f};

        ASSERT_EQ(2.3,d);
        ASSERT_EQ(4,i);
        ASSERT_EQ(1.2f,f);
        ASSERT_EQ(true, b);
        for(std::size_t index = 0; index < 4; ++index)
            ASSERT_EQ(ai2[index],ai[index]);
        for(std::size_t index = 0; index < 3; ++index)
            ASSERT_EQ(ad2[index],ad[index]);
        for(std::size_t index = 0; index < 5; ++index)
            ASSERT_EQ(af2[index],af[index]);
        ASSERT_EQ(3.14159,*pd);
        //std::cout << "ran Foo2::foo" << std::endl;
    }
};

using i128t = int __attribute__((aligned(128)));

class parameter_buffer;

template<typename Buffer>
struct aligned_binary_buffer_identifier : std::false_type {};

template<>
struct aligned_binary_buffer_identifier<parameter_buffer> : std::true_type {};

template<typename Buffer>
constexpr bool is_aligned_binary_buffer = aligned_binary_buffer_identifier<std::remove_reference_t<Buffer>>::value;

template<typename T, typename Buffer>
struct buildtype_helper
{
    constexpr static bool value = !(std::is_scalar<std::remove_reference_t<T>>::value && is_aligned_binary_buffer<Buffer> && !std::is_pointer<T>::value);
};

template<typename T, std::size_t N, typename Buffer>
struct buildtype_helper<T[N],Buffer>
{
    constexpr static bool value = !(std::is_scalar<std::remove_reference_t<T>>::value && is_aligned_binary_buffer<Buffer> && !std::is_pointer<T>::value);
};

template<typename T, typename Buffer>
constexpr bool is_buildtype = buildtype_helper<T,Buffer>::value;

template<typename Buffer, typename...Ts>
struct buildtype_tuple_helper;

template<typename Buffer, typename T, typename...Ts>
struct buildtype_tuple_helper<Buffer,T,Ts...>
{
    using prepend_type = std::conditional_t<is_buildtype<T,Buffer>,std::tuple<std::add_pointer_t<T>>,std::tuple<>>;
    using next_elems_type = typename buildtype_tuple_helper<Buffer,Ts...>::type;
    using type = mpirpc::internal::tuple_cat_type<prepend_type, next_elems_type>;
};

template<typename Buffer>
struct buildtype_tuple_helper<Buffer>
{
    using type = std::tuple<>;
};

template<typename...Ts>
using buildtype_tuple_type = typename buildtype_tuple_helper<Ts...>::type;

constexpr std::size_t default_alignment_padding(std::size_t addr, std::size_t alignment)
{
    return (addr % alignment) ? alignment - addr % alignment : 0;
}

template<typename Buffer, std::size_t addr, std::size_t alignment>
struct aligned_binary_buffer_delta_helper
{
    static constexpr std::size_t value = default_alignment_padding(addr,alignment);
};

template<typename Buffer, std::size_t addr, std::size_t alignment>
constexpr std::size_t aligned_binary_buffer_delta = aligned_binary_buffer_delta_helper<Buffer,addr,alignment>::value;

template<typename T, typename Buffer, std::size_t alignment, typename = void>
struct marshaller;

template<typename T, typename Buffer, std::size_t alignment, typename = void>
struct unmarshaller;

class parameter_buffer
{
public:
    parameter_buffer(std::vector<char> *buf) noexcept
        : m_buffer{buf}, m_position{}
    {}

    template<typename Allocator>
    parameter_buffer(Allocator&& a)
        : m_buffer{new std::vector<char>{std::forward<Allocator>(a)}}, m_position{}
    {}

    parameter_buffer()
        : m_buffer{new std::vector<char>()}, m_position{}
    {}

    char* data() noexcept { return m_buffer->data(); }
    const char* data() const noexcept { return m_buffer->data(); }
    char* data_here() noexcept { return &m_buffer->data()[m_position]; }
    const char* data_here() const noexcept { return &m_buffer->data()[m_position]; }
    std::size_t position() const noexcept { return m_position; }

    void seek(std::size_t pos) noexcept { m_position = pos; }

    template<typename T>
    T* reinterpret_and_advance(std::size_t size) noexcept { T* ret = reinterpret_cast<T*>(&m_buffer->data()[m_position]); m_position+=size; return ret; }

    template<std::size_t alignment>
    void append(const char * start, const char * end)
    {
        std::size_t padding = default_alignment_padding(m_position,alignment);
        std::size_t delta = padding + (end-start);
        std::size_t new_size = m_buffer->size() + delta;
        std::cout << "padding: " << padding << " " << alignment << std::endl;
        m_buffer->reserve(new_size);
        m_buffer->resize(m_buffer->size() + padding);
        m_buffer->insert(m_buffer->end(),start,end);
        m_position += delta;
    }

    template<std::size_t Alignment>
    void realign()
    {
        std::cout << "realigning by " << " " << ((m_position % Alignment) ? (Alignment - (m_position % Alignment)) : 0) << std::endl;// << " for " << abi::__cxa_demangle(typeid(T).name(),0,0,0) << std::endl;
        m_position += (m_position % Alignment) ? (Alignment - (m_position % Alignment)) : 0;
    }

    template<typename T, typename Allocator>
    decltype(auto) get(Allocator&& a)
    {
        realign<alignof(T)>();
        return unmarshaller<std::remove_cv_t<std::remove_reference_t<T>>,parameter_buffer,alignof(T)>::unmarshal(std::forward<Allocator>(a),*this);
    }

    template<typename T>
    void put(T&& t)
    {
        std::cout << "put alignment: " << alignof(T) << " of type " << abi::__cxa_demangle(typeid(T).name(),0,0,0) << std::endl;
        marshaller<std::remove_cv_t<std::remove_reference_t<T>>,parameter_buffer,alignof(T)>::marshal(*this,std::forward<T>(t));
    }

protected:
    std::size_t m_position;
    std::vector<char> *m_buffer;
};

template<typename T, typename Allocator>
inline decltype(auto) get(parameter_buffer& b, Allocator&& a)
{
    return b.template get<T>(std::forward<Allocator>(a));
}

template<typename T>
inline constexpr uintptr_t type_id()
{
    return reinterpret_cast<uintptr_t>(&typeid(T));
}

template<typename T>
inline uintptr_t type_id(T&& t)
{
    return reinterpret_cast<uintptr_t>(&typeid(t));
}

template<typename T>
struct type_identifier
{
    constexpr static uintptr_t id()
    {
        return reinterpret_cast<uintptr_t>(&type_identifier<T>::id);
    }
};

struct piecewise_construct_type {};

constexpr piecewise_construct_type piecewise_construct{};

template<typename Allocator>
struct piecewise_allocator_traits;

template<typename Allocator, typename Buffer>
struct polymorphic_factory_base
{
    virtual void* build(Allocator& a, Buffer& b, std::size_t count) = 0;
};

template<typename T, typename Allocator, typename Buffer>
struct polymorphic_factory : polymorphic_factory_base<Allocator,Buffer>
{
    virtual void* build(Allocator&a, Buffer& b, std::size_t count)
    {
        using AllocatorType = typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
        AllocatorType na(a);
        T *ptr = std::allocator_traits<AllocatorType>::allocate(na,count);
        for (std::size_t i = 0; i < count; ++i)
        {
            piecewise_allocator_traits<AllocatorType>::construct(na,&ptr[i],get<T>(b,na));
        }
        return static_cast<void*>(ptr);
    }
};

std::map<uintptr_t,std::type_index> safe_type_index_map;
std::map<std::type_index,polymorphic_factory_base<std::allocator<char>,parameter_buffer>*> polymorphic_map;

template<typename T>
void register_polymorphism()
{
    std::cout << "registering " << abi::__cxa_demangle(typeid(T).name(),0,0,0) << " " << type_identifier<T>::id() << std::endl;
    safe_type_index_map.insert({type_identifier<T>::id(),std::type_index{typeid(T)}});
    polymorphic_map.insert({std::type_index{typeid(T)},new polymorphic_factory<T,std::allocator<char>,parameter_buffer>()});
}

template<typename T, typename Buffer, std::size_t alignment>
struct marshaller<T,Buffer,alignment,std::enable_if_t<!buildtype_helper<std::remove_reference_t<T>,Buffer>::value && std::is_same<Buffer,parameter_buffer>::value>>
{
    template<typename U,std::enable_if_t<std::is_same<std::decay_t<T>,std::decay_t<U>>::value>* = nullptr>
    static void marshal(parameter_buffer& b, U&& val)
    {
        std::cout << "marshalling alignment: " << alignment << " for type " << abi::__cxa_demangle(typeid(T).name(),0,0,0) << std::endl;
        const char* p = reinterpret_cast<const char*>(&val);
        b.append<alignment>(p, p+sizeof(T));
    }
};

/*template<typename T, std::size_t alignment>
struct parameterbuffer_marshaller<T,alignment,std::enable_if_t<std::is_pointer<T>::value>>
{
    static void marshal(parameter_buffer& b, const T& t)
    {
        parameterbuffer_marshaller<mpirpc::pointer_wrapper<std::remove_pointer_t<T>>,alignof(std::remove_pointer_t<T>)>::marshal(b,mpirpc::pointer_wrapper<std::remove_pointer_t<T>>(t));
    }
};*/

template<typename T, typename Buffer, std::size_t Alignment>
struct unmarshaller<T,Buffer,Alignment,std::enable_if_t<!buildtype_helper<std::remove_reference_t<T>,Buffer>::value && std::is_same<Buffer,parameter_buffer>::value>>
{
    template<typename Allocator>
    static T unmarshal(Allocator&&, parameter_buffer& b)
    {
        return *b.reinterpret_and_advance<std::remove_reference_t<T>>(sizeof(std::remove_reference_t<T>));
    }
};

template<typename T, typename Buffer, std::size_t Alignment>
struct marshaller<mpirpc::pointer_wrapper<T>, Buffer, Alignment>
{
    template<typename U = T, std::enable_if_t<!std::is_polymorphic<U>::value>* = nullptr>
    static void marshal(Buffer& b, const mpirpc::pointer_wrapper<U>& val)
    {
        b.put(val.size());
        for (std::size_t i = 0; i < val.size(); ++i)
        {
            b.put(val[i]);
            //marshaller<U,Buffer,Alignment>::marshal(b,val[i]);
        }
    }

    template<typename U = T, std::enable_if_t<std::is_polymorphic<U>::value>* = nullptr>
    static void marshal(Buffer& b, const mpirpc::pointer_wrapper<U>& val)
    {
        std::cout << "marshalling polymorphic " << type_id(*val) << std::endl;
        b.put(val.size());
        b.put(type_identifier<U>::id());
        for (std::size_t i = 0; i < val.size(); ++i)
        {
            b.put(val[i]);
            //marshaller<U,Buffer,Alignment>::marshal(b,val[i]);
        }
    }
};

class A
{
public:
    A(int v) : a(v) {}
    A(const A&) = delete;
    A(A&&) = delete;
    A() : a(1) {}
    virtual void test() {}
    virtual ~A() { std::cout << "A destructor" << std::endl; }
    int a;
};

class B : public A {
public:
    B(int v, int v2) : A(v), b(v2) {}
    B(const B&) = delete;
    B(B&&) = delete;
    B() : A(), b(2) {}
    virtual void test() {}
    virtual ~B() { std::cout << "B destructor" << std::endl; }
    int b;
};

template<typename Buffer, std::size_t Alignment>
struct marshaller<A,Buffer,Alignment>
{
    static void marshal(Buffer& b, const A& val)
    {
        b.put(val.a);
    }
};

template<typename Buffer, std::size_t Alignment>
struct unmarshaller<A,Buffer,Alignment>
{
    template<typename Allocator>
    static std::tuple<piecewise_construct_type,int> unmarshal(Allocator& alloc, Buffer& buff)
    {
        int a = get<int>(buff,alloc);
        return std::tuple<piecewise_construct_type,int>(piecewise_construct,std::move(a));
    }
};

template<typename Buffer, std::size_t Alignment>
struct marshaller<B,Buffer,Alignment>
{
    static void marshal(Buffer& b, const B& val)
    {
        b.put(val.a);
        b.put(val.b);
    }
};

template<typename Buffer, std::size_t Alignment>
struct unmarshaller<B,Buffer,Alignment>
{
    template<typename Allocator>
    static std::tuple<piecewise_construct_type,int,int> unmarshal(Allocator& alloc, Buffer& buff)
    {
        int a = buff.template get<int>(alloc);
        int b = buff.template get<int>(alloc);
        return std::tuple<piecewise_construct_type,int,int>(piecewise_construct,std::move(a),std::move(b));
    }
};

template<std::size_t Alignment, typename T>
void get_pointer_from_buffer(parameter_buffer& b, T*& t)
{
    b.realign<Alignment>();
    t = b.reinterpret_and_advance<T>(sizeof(T));
}

template<typename T>
struct is_std_allocator;

template <template <typename> class Alloc, typename T>
struct is_std_allocator<Alloc<T>> : std::false_type{};

template <typename T>
struct is_std_allocator<std::allocator<T>> : std::true_type{};

template<typename T>
struct is_tuple : std::false_type{};

template<typename...Ts>
struct is_tuple<std::tuple<Ts...>> : std::true_type{};

template<typename Allocator>
struct piecewise_allocator_traits
{
    template<typename T, typename Tuple, std::size_t...Is>
    static void tuple_construct(Allocator& a, T* p, Tuple&& tup, std::index_sequence<Is...>)
    {
        std::allocator_traits<Allocator>::construct(a,p,std::move(std::get<Is+1>(tup))...);
    }

    template<typename T,
             typename... Args,
             std::enable_if_t<std::is_constructible<T,Args...>::value>* = nullptr>  //!is_tuple<typename std::allocator_traits<Allocator>::value_type>::value && is_tuple<U>::value>* = nullptr>
    static void construct(Allocator& a, T* p, std::tuple<piecewise_construct_type,Args...>&& val)
    {
        std::cout << "size: " << sizeof(std::tuple<Args...>) << " " << sizeof(std::tuple<piecewise_construct_type,Args...>) << std::endl;
        tuple_construct(a,p,std::move(val),std::make_index_sequence<sizeof...(Args)>{});
    }

    template<typename T,
             typename U,
             std::enable_if_t<std::is_constructible<T,U>::value>* = nullptr>
    static void construct(Allocator& a, T *p, U &&val)
    {
        std::allocator_traits<Allocator>::construct(a,p,std::forward<U>(val));
    }
};

template<typename T>
struct direct_initializer
{
    template<typename Allocator, typename Buffer>
    static void construct(Allocator& a, T* t, Buffer&& s)
    {
        using AllocatorType = typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
        AllocatorType na(a);
        piecewise_allocator_traits<AllocatorType>::construct(na,t,get<T>(s,a));
    }

    template<typename Allocator>
    static void destruct(Allocator& a, T* t)
    {
        using AllocatorType = typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
        AllocatorType na(a);
        std::allocator_traits<AllocatorType>::destroy(na, t);
    }

    template<typename Allocator, typename Buffer>
    static void placementnew_construct(Allocator& a, T* t, Buffer&& b)
    {
        std::cout << "default std::allocator constructing " << abi::__cxa_demangle(typeid(T).name(),0,0,0) << std:: endl;
        std::allocator<T> na;
        construct(na,t,std::forward<Buffer>(b));
    }

    template<typename Allocator>
    static void placementnew_destruct(const Allocator&, T* t)
    {
        t->~T();
    }
};

template<typename T, typename Buffer, std::size_t Alignment>
struct unmarshaller<mpirpc::pointer_wrapper<T>,Buffer,Alignment>
{
    template<typename Allocator>
    static std::tuple<piecewise_construct_type,T*,std::size_t,bool,bool> unmarshal(Allocator& a, Buffer &b)
    {
        T *ptr;
        std::size_t size = get<std::size_t>(b,a);
        bool pass_back = false, pass_ownership = false;

        if (pass_ownership || is_buildtype<T,Buffer>)
        {
            if (std::is_polymorphic<T>::value)
            {
                using VoidAllocatorType = typename std::allocator_traits<Allocator>::template rebind_alloc<char>;
                VoidAllocatorType va(a);
                uintptr_t type = get<uintptr_t>(b,a);
                ptr = static_cast<T*>(polymorphic_map.at(safe_type_index_map.at(type))->build(va,b,size));
            }
            else
            {
                using AllocatorType = typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
                AllocatorType na(a);
                ptr = std::allocator_traits<AllocatorType>::allocate(na,size);
                for (std::size_t i = 0; i < size; ++i)
                    direct_initializer<T>::construct(na,&ptr[i],b);
            }
        }
        else
            get_pointer_from_buffer<alignof(T)>(b,ptr);
        return std::tuple<piecewise_construct_type,T*,std::size_t,bool,bool>{piecewise_construct,ptr,size,pass_back,pass_ownership};
    }
};

template<typename T, std::size_t N>
struct direct_initializer<T[N]>
{
    template<typename Allocator, typename Buffer>
    static void placementnew_construct(const Allocator &a, T(*t)[N], Buffer&& s)
    {
        for (std::size_t i = 0; i < N; ++i)
        {
            direct_initializer<T>::placementnew_construct(a,&(*t)[i],s);
        }
    }

    template<typename Allocator>
    static void placementnew_destruct(const Allocator& a, T(*t)[N])
    {
        for (std::size_t i = 0; i < N; ++i)
        {
            direct_initializer<T>::placementnew_destroy(a,&(*t)[i]);
        }
    }

    template<typename Allocator, typename Buffer>
    static void construct(const Allocator &a, T(*t)[N], Buffer&& s)
    {
        for (std::size_t i = 0; i < N; ++i)
        {
            direct_initializer<T>::construct(a,&(*t)[i],s);
        }
    }

    template<typename Allocator>
    static void destruct(const Allocator& a, T(*t)[N])
    {
        for (std::size_t i = 0; i < N; ++i)
        {
            direct_initializer<T>::destroy(a,&(*t)[i]);
        }
    }
};

template<typename T, typename Buffer, std::size_t Alignment, typename = void>
struct remarshaller
{
    static void marshal(Buffer& s, std::remove_reference_t<T>&& val)
    {
        marshaller<T,Buffer,Alignment>::marshal(s,val);
    }
    
    static void marshal(Buffer& s, std::remove_reference_t<T>& val)
    {
        marshaller<T,Buffer,Alignment>::marshal(s,val);
    }
};

/*template<typename T, std::size_t N, std::size_t alignment>
struct parameterbuffer_remarshaller<T(&)[N],alignment>
{
    static void marshal(parameter_buffer& s, T(&val)[N])
    {
        parameterbuffer_marshaller<T[N],alignment>::marshal(s,val);
    }
};*/

template<std::size_t Alignment, typename Allocator, typename T, typename Buffer, std::enable_if_t<is_buildtype<T,Buffer>>* = nullptr>
void get_from_buffer(Allocator &a, T*& t, Buffer&& s)
{
    std::cout << "get from stream: " << abi::__cxa_demangle(typeid(T).name(),0,0,0) << " " << is_buildtype<T,Buffer> << std::endl;
    std::cout << "get from stream: " << abi::__cxa_demangle(typeid(t).name(),0,0,0) << std::endl;
    std::cout << std::is_scalar<std::remove_reference_t<T>>::value << " " << is_aligned_binary_buffer<Buffer> << std::endl;
    direct_initializer<T>::placementnew_construct(a,t,s);
}

template<std::size_t Alignment, typename Allocator, typename T, typename Buffer, std::enable_if_t<!is_buildtype<T,Buffer>>* = nullptr>
void get_from_buffer(Allocator &a, T*& t, Buffer&& s)
{
    get_pointer_from_buffer<Alignment>(s,t);
}

template<typename Buffer, typename Allocator, typename T, std::enable_if_t<is_buildtype<T,Buffer>>* = nullptr>
void cleanup(Allocator &a, T* t)
{
    direct_initializer<T>::placementnew_destruct(a,t);
}

template<typename Buffer, typename Allocator, typename T, std::enable_if_t<!is_buildtype<T,Buffer>>* = nullptr>
void cleanup(Allocator &a, T* t)
{}

template<typename T, std::size_t N, std::enable_if_t<!std::is_same<T,char>::value>* = nullptr>
std::ostream& operator<<(std::ostream& o, const T(&d)[N])
{
    o << "[";
    for (std::size_t i = 0; i < N-1; ++i)
        o << d[i] << ",";
    o << d[N-1] << "]";
    return o;
}

template<typename T>
std::ostream& operator<<(std::ostream& o, const mpirpc::pointer_wrapper<T>& v)
{
    o << *v;
    return o;
}

template<typename T>
void print(T&& t)
{
    //std::cout << abi::__cxa_demangle(typeid(void(*)(T)).name(),0,0,0) << ": " << t << std::endl;
    std::cout << t << std::endl;
}

template<typename Buffer,bool SkipBuildTypes, bool SkipNonBuildTypes, std::size_t Pos, typename...Ts>
struct alignment_padding_helper_impl
{
    using type = std::tuple_element_t<Pos,std::tuple<Ts...>>;
    static constexpr bool predicate = (is_buildtype<type,Buffer> && !SkipBuildTypes) || (!is_buildtype<type,Buffer> && !SkipNonBuildTypes);
    static constexpr std::size_t prev_end_address_offset = alignment_padding_helper_impl<Buffer,SkipBuildTypes,SkipNonBuildTypes,Pos-1,Ts...>::end_address_offset;
    static constexpr std::size_t delta = (predicate) ? aligned_binary_buffer_delta<Buffer,prev_end_address_offset,alignof(type)> : 0;
    static constexpr std::size_t start_address_offset = (predicate) ? prev_end_address_offset + delta : 0;
    static constexpr std::size_t end_address_offset = (predicate) ? start_address_offset + sizeof(type) : 0;
    static constexpr std::size_t total_padding = alignment_padding_helper_impl<Buffer,SkipBuildTypes,SkipNonBuildTypes,Pos-1,Ts...>::total_padding + delta;
    static constexpr std::size_t total_size = alignment_padding_helper_impl<Buffer,SkipBuildTypes,SkipNonBuildTypes,Pos-1,Ts...>::total_size + delta + sizeof(type);
};

template<typename Buffer, bool SkipBuildTypes, bool SkipNonBuildTypes, typename T, typename...Ts>
struct alignment_padding_helper_impl<Buffer,SkipBuildTypes,SkipNonBuildTypes,0,T,Ts...>
{
    using type = T;
    static constexpr bool predicate = (is_buildtype<type,Buffer> && !SkipBuildTypes) || (!is_buildtype<type,Buffer> && !SkipNonBuildTypes);
    static constexpr std::size_t prev_end_address_offset = 0;
    static constexpr std::size_t delta = 0;
    static constexpr std::size_t start_address_offset = 0;
    static constexpr std::size_t end_address_offset = (predicate) ? sizeof(type) : 0;
    static constexpr std::size_t total_padding = 0;
    static constexpr std::size_t total_size = sizeof(type);
};

template<typename Buffer, bool SkipBuildTypes, bool SkipNonBuildTypes>
struct alignment_padding_helper_impl<Buffer,SkipBuildTypes,SkipNonBuildTypes,0>
{
    using type = std::nullptr_t;
    static constexpr bool predicate = false;
    static constexpr std::size_t prev_end_address_offset = 0;
    static constexpr std::size_t delta = 0;
    static constexpr std::size_t start_address_offset = 0;
    static constexpr std::size_t end_address_offset = 0;
    static constexpr std::size_t total_padding = 0;
    static constexpr std::size_t total_size = 0;
};

template<typename...Ts,std::size_t...Is>
void test_alignment_padding(std::index_sequence<Is...>)
{
    using swallow = int[];
    std::cout << "-------" << std::endl;
    (void)swallow{( std::cout << "alignment padding: " << alignment_padding_helper_impl<parameter_buffer,false,false,Is,Ts...>::delta << std::endl, 0)...};
    std::cout << "-------" << std::endl;
}

template<typename Buffer, bool SkipBuildTypes, bool SkipNonBuildTypes, typename... Ts>
constexpr std::size_t alignment_padding = alignment_padding_helper_impl<Buffer, SkipBuildTypes, SkipNonBuildTypes, (sizeof...(Ts) > 0) ? (sizeof...(Ts)-1) : 0,Ts...>::total_padding;

template<typename Buffer, bool SkipBuildTypes, bool SkipNonBuildTypes, typename... Ts>
constexpr std::size_t align_buffer_size = alignment_padding_helper_impl<Buffer, SkipBuildTypes, SkipNonBuildTypes, (sizeof...(Ts) > 0) ? (sizeof...(Ts)-1) : 0,Ts...>::total_size;

template<typename Buffer, bool SkipBuildTypes, bool SkipNonBuildTypes, std::size_t index, typename...Ts>
constexpr std::size_t align_buffer_address_offset = alignment_padding_helper_impl<Buffer, SkipBuildTypes, SkipNonBuildTypes, index, Ts...>::start_address_offset;

template<typename F, typename Allocator, typename InBuffer, typename OutBuffer, typename...FArgs, typename...Ts, bool... PBs, std::size_t... Is, std::size_t... Alignments,
         std::enable_if_t<std::is_same<mpirpc::internal::function_return_type<F>,void>::value>* = nullptr>
void apply_impl(F&& f, Allocator&& a, InBuffer& s, OutBuffer& os, mpirpc::internal::type_pack<FArgs...>, mpirpc::internal::type_pack<Ts...>, mpirpc::internal::bool_template_list<PBs...>, std::index_sequence<Is...>, std::integer_sequence<std::size_t,Alignments...>)
{
    constexpr std::size_t buffer_size = align_buffer_size<InBuffer,false,true,Ts...>;
    char * buffer = (buffer_size > 0) ? static_cast<char*>(alloca(buffer_size)) : nullptr;
    std::tuple<std::add_pointer_t<Ts>...> t{((is_buildtype<Ts,InBuffer>) ? static_cast<std::add_pointer_t<Ts>>(static_cast<void*>(buffer + align_buffer_address_offset<InBuffer,false,true,Is,Ts...>)) : nullptr)...};
    using swallow = int[];
    (void)swallow{(get_from_buffer<Alignments>(std::forward<Allocator>(a),std::get<Is>(t),s), 0)...};
    std::forward<F>(f)(static_cast<FArgs>(*std::get<Is>(t))...);
    (void)swallow{((PBs) ? (remarshaller<mpirpc::internal::autowrapped_type<FArgs>,OutBuffer,Alignments>::marshal(os, mpirpc::internal::autowrap<mpirpc::internal::autowrapped_type<FArgs>,Ts>(*std::get<Is>(t))), 0) : 0)...};
    (void)swallow{(cleanup<InBuffer>(std::forward<Allocator>(a),std::get<Is>(t)), 0)...};
}

template<typename F, typename Allocator, typename InBuffer, typename OutBuffer, typename...FArgs, typename...Ts, bool... PBs, std::size_t...Is, std::size_t... Alignments,
         std::enable_if_t<!std::is_same<mpirpc::internal::function_return_type<F>,void>::value>* = nullptr>
void apply_impl(F&& f, Allocator&& a, InBuffer& s, OutBuffer& os, mpirpc::internal::type_pack<FArgs...>, mpirpc::internal::type_pack<Ts...>, mpirpc::internal::bool_template_list<PBs...>, std::index_sequence<Is...>, std::integer_sequence<std::size_t,Alignments...>)
{
    constexpr std::size_t buffer_size = align_buffer_size<InBuffer,false,true,Ts...>;
    using PointerTuple = std::tuple<std::add_pointer_t<Ts>...>;
    char * buffer = (buffer_size > 0) ? static_cast<char*>(alloca(buffer_size)) : nullptr;
    PointerTuple t{((is_buildtype<Ts,InBuffer>) ? static_cast<std::add_pointer_t<Ts>>(static_cast<void*>(buffer + align_buffer_address_offset<InBuffer,false,true,Is,Ts...>)) : nullptr)...};
    using swallow = int[];
    (void)swallow{(get_from_buffer<Alignments>(std::forward<Allocator>(a),std::get<Is>(t),s), 0)...};
    auto&& ret = std::forward<F>(f)(static_cast<FArgs>(*std::get<Is>(t))...);
    using R = mpirpc::internal::function_return_type<F>;
    remarshaller<R,OutBuffer,alignof(R)>::marshal(os,mpirpc::internal::autowrap<mpirpc::internal::function_return_type<F>,decltype(ret)>(std::move(ret)));
    (void)swallow{((PBs) ? (remarshaller<mpirpc::internal::autowrapped_type<FArgs>,OutBuffer,Alignments>::marshal(os, mpirpc::internal::autowrap<mpirpc::internal::autowrapped_type<FArgs>,Ts>(*std::get<Is>(t))), 0) : 0)...};
    (void)swallow{(cleanup<InBuffer>(std::forward<Allocator>(a),std::get<Is>(t)), 0)...};
}

template<typename F, class Class, typename Allocator, typename InBuffer, typename OutBuffer, typename...FArgs, typename...Ts, bool... PBs, std::size_t...Is, std::size_t... Alignments,
         std::enable_if_t<std::is_same<mpirpc::internal::function_return_type<F>,void>::value>* = nullptr>
void apply_impl(F&& f, Class *c, Allocator&& a, InBuffer& s, OutBuffer& os, mpirpc::internal::type_pack<FArgs...>, mpirpc::internal::type_pack<Ts...>, mpirpc::internal::bool_template_list<PBs...>, std::index_sequence<Is...>, std::integer_sequence<std::size_t,Alignments...>)
{
    constexpr std::size_t buffer_size = align_buffer_size<InBuffer,false,true,Ts...>;
    char * buffer = (buffer_size > 0) ? static_cast<char*>(alloca(buffer_size)) : nullptr;
    std::tuple<std::add_pointer_t<Ts>...> t{((is_buildtype<Ts,InBuffer>) ? static_cast<std::add_pointer_t<Ts>>(static_cast<void*>(buffer + align_buffer_address_offset<InBuffer,false,true,Is,Ts...>)) : nullptr)...};
    using swallow = int[];
    (void)swallow{(get_from_buffer<Alignments>(std::forward<Allocator>(a),std::get<Is>(t),s), 0)...};
    ((*c).*(std::forward<F>(f)))(static_cast<FArgs>(*std::get<Is>(t))...);
    (void)swallow{((PBs) ? (remarshaller<mpirpc::internal::autowrapped_type<FArgs>,OutBuffer,Alignments>::marshal(os, mpirpc::internal::autowrap<mpirpc::internal::autowrapped_type<FArgs>,Ts>(*std::get<Is>(t))), 0) : 0)...};
    (void)swallow{(cleanup<InBuffer>(a,std::get<Is>(t)), 0)...};
}

template<typename F, class Class, typename Allocator, typename InBuffer, typename OutBuffer, typename...FArgs, typename...Ts, bool... PBs, std::size_t...Is, std::size_t... Alignments,
         std::enable_if_t<!std::is_same<mpirpc::internal::function_return_type<F>,void>::value>* = nullptr>
void apply_impl(F&& f, Class *c, Allocator&& a, InBuffer& s, OutBuffer& os, mpirpc::internal::type_pack<FArgs...>, mpirpc::internal::type_pack<Ts...>, mpirpc::internal::bool_template_list<PBs...>, std::index_sequence<Is...>, std::integer_sequence<std::size_t,Alignments...>)
{
    constexpr std::size_t buffer_size = align_buffer_size<InBuffer,false,true,Ts...>;
    char * buffer = (buffer_size > 0) ? static_cast<char*>(alloca(buffer_size)) : nullptr;
    std::tuple<std::add_pointer_t<Ts>...> t{((is_buildtype<Ts,InBuffer>) ? static_cast<std::add_pointer_t<Ts>>(static_cast<void*>(buffer + align_buffer_address_offset<InBuffer,false,true,Is,Ts...>)) : nullptr)...};
    using swallow = int[];
    (void)swallow{(get_from_buffer<Alignments>(std::forward<Allocator>(a),std::get<Is>(t),s), 0)...};
    auto&& ret = ((*c).*(std::forward<F>(f)))(static_cast<FArgs>(*std::get<Is>(t))...);
    os.put(mpirpc::internal::autowrap<mpirpc::internal::function_return_type<F>,decltype(ret)>(std::move(ret)));
    (void)swallow{((PBs) ? (remarshaller<mpirpc::internal::autowrapped_type<FArgs>,OutBuffer,Alignments>::marshal(os, mpirpc::internal::autowrap<mpirpc::internal::autowrapped_type<FArgs>,Ts>(*std::get<Is>(t))), 0) : 0)...};
    (void)swallow{(cleanup<InBuffer>(std::forward<Allocator>(a),std::get<Is>(t)), 0)...};
}

template<typename T1, typename T2,typename = void>
struct choose_custom_alignment;

template<std::size_t...I1s,std::size_t...I2s>
struct choose_custom_alignment<std::integer_sequence<std::size_t,I1s...>,std::integer_sequence<std::size_t,I2s...>, std::enable_if_t<!!sizeof...(I2s)>>
{
    using type = std::integer_sequence<std::size_t,I2s...>;
};

template<std::size_t...I1s, std::size_t...I2s>
struct choose_custom_alignment<std::integer_sequence<std::size_t,I1s...>,std::integer_sequence<std::size_t,I2s...>, std::enable_if_t<!sizeof...(I2s)>>
{
    using type = std::integer_sequence<std::size_t,I1s...>;
};

template<typename T1, typename T2>
using custom_alignments = typename choose_custom_alignment<T1,T2>::type;


/**
 * @details
 * Mechanism:
 *
 * Applying a datagram as arguments with any type to a function is a non-trivial task. The naive approach would be to unpack
 * the stream into a tuple, then apply those tuple arguments to a function. However, this approach has some notable limitations
 * and inefficiencies. First of all, it is not possible to construct a std::tuple with a mix of any type. Take, for instance, a
 * std::tuple containing an array and an mpirpc::pointer_wrapper. An array type is not MoveConstructible or CopyConstructible.
 * The array could be initialized and then the values from the stream applied to it, but mpirpc::pointer_wrapper is not
 * DefaultConstructible. An array type also can't be returned by an unmarshalling function and references to an array can't be
 * used, ei,ther, unless it was initialized outside of the unmarshaller. Otherwise, the result would be a dangling reference. An
 * array type could be created from a std::initializer_list instead, but this would not solve the general case of types with
 * deleted default, copy, and move constructors. For these types, direct initialization is needed. Unfortunately, std::tuple
 * lacks direct initialization capabilities.
 *
 * A previous implementation attempt used two tuples, one created with default constructors and modified using the stream after
 * initialization and a second tuple for move-constructible types suitable for non-DefaultConstructible types. However, this
 * involved some complicated template metaprogramming to determine the parameter index, which tuple to use, and the index in the
 * tuple for each parameter index. It also still had the limitation that a type must be either DefaultConstructible or be
 * MoveConstructible. Types requiring direct initialization would still be unsupported.
 *
 * This implementation gets around this problem by creating a tuple of pointers to each parameter type. How this tuple is
 * initialized depends on the type. If is_buildtype\<T> is true for that type, then the std::tuple element will be initialized
 * to a pointer within a stack buffer which is properly aligned for an element of that type. If is_buildtype\<T> is false, then
 * the pointer points to the location of the data in the stream buffer. By default, is_buildtype\<T> is true if T is a scalar
 * type or an array of scalar types. Otherwise, it is false. Users may specialize is_buildtype\<T> to a boolean true value for
 * custom types that can be correctly accessed by a reinterpret_cast\<T*> on the buffer location (objects with standard layout
 * and no pointer/reference type member variables).
 *
 * Next, types for which is_buildtype\<T> is true call direct_initializer\<T>::placementnew_construct(Allocator,T*,Stream).
 * This constructs a T using placement new, which is suitable for constructing types in the stack buffer. Types which have pointers
 * of their own should use direct_initializer\<T>::construct(Allocator,T*,Buffer), which uses the (possibly custom) Allocator for
 * construction. If T is polymorphic, register_polymorphism\<T>() must be called before invocation of apply(). The arguments for
 * construction of T are provided by unmarshaller\<T,Buffer,Alignment>::unmarshal(Allocator,Buffer).
 *
 * Next, the function is run by unpacking each element of the tuple as each parameter. If the function has a non-void return
 * type, it is autowrapped and added to the output parameter buffer using remarshaller\< T, Buffer, Alignment >::marshal(Buffer,T).
 *
 * Then, passback types from the parameter tuple are appended to the parameter buffer using
 * remarshaller\<T,Buffer,Alignment>::marshal(Buffer,T).
 *
 * Finally, for each element of the parameter tuple for which the value of is_buildtype\< T > is true the tuple element is passed
 * to a cleanup function to run the destructor on the constructed types.
 *
 * @verbatim
 * Call graph:
 *                                                    apply()
 *                                                      |
 *                                                      v
 *                                                 apply_impl()
 *                                                      |
 *                                                      v
 *                                              get_from_buffer<Alignment>()
 *                                                      |
 *                                                      v
 *                                            is_buildtype<T,Buffer>?
 *                                            No /              \ Yes
 *                          get_pointer_from_buffer()          direct_initializer<T>::placementnew_construct()
 *                                              |                |
 *                                              v                v
 *                  Buffer::reinterpret_and_advance<T>()     unmarshaller<T,Buffer,Alignment,typename=void>::unmarshal()
 *                                              |                |
 *                                              |                v
 *                                              |             get<T>(Buffer,Allocator)
 *                                              |                |
 *                                              |                v
 *                                              |        piecewise_allocator_traits<Allocator>::construct()
 *                                               \_____________/
 *                                                      |
 *                                                      v
 *                                                    call f()
 *                                                      |
 *                                                      v
 *                                               (result == void)?
 *                                             No /            \ Yes
 *             remarshaller<T,Buffer,Alignment>::marshal()     |
 *                                                |            |
 *                                                v            |
 *               marshaller<T,Buffer,Alignment>::marshal()     |
 *                                                \____________/
 *                                                      |
 *                                                      v
 *                                                  cleanup()
 * @endverbatim
 *
 * The implementation of remarshaller\<T,Buffer,Alignment>::marshal() simply calls marshaller\<T,Buffer,Alignment>::marshal().
 * However, this struct can be specialized if different behavior is required when passing back (such as noting and skipping
 * unmodified parameters).
 *
 * is_buildtype\<T,Buffer> is used to determine if the type stored in the Buffer can be read simply by a reinterpret_cast\<T> at
 * the current location of the buffer. Therefore, is_buildtype\<T> should always be true for Buffer implementations that use
 * non-binary data or packed data structures.
 *
 * unmarshaller\<T,Buffer,Alignment> specializations: call parameterbuffer_unmarshaller\<T,Alignment> for non-pointer scalar types
 *                                                   unmarshaller\<pointer_wrapper\<T>,Buffer,Alignment,void>
 *                                                   user-defined unmarshallers
 *
 *
 *        unmarshaller\<pointer_wrapper\<T>,Buffer,Alignment,typename=void>
 *                                      |
 *                                      v
 *                            std::is_polymorphic\<T>?
 *                          No /                   \ Yes
 *   direct_initializer\<T>::construct              polymorphic_factory\<T>->build()
 *                      U = T |                     | U = from id -> polymorphic_factory map
 *                            v                     v
 *                  unmarshaller\<U,Buffer,Alignment>::unmarshal
 *
 * To add support for custom types, unmarshaller\<T,Buffer,Alignment> should be specialized. To avoid template ambiguity, only the first
 * template parameter should be specialized. If the unmarshaller is specific to a Buffer type or alignment, SFINAE should be used for the
 * last template parameter. unmarshaller\<T,Buffer,Alignment> should return either a single type to be passed as a single argument to the
 * constructor or a tuple of types which will be unpacked as constructor arguments.
 *
 */
template<typename F,typename Allocator, typename InBuffer, typename OutBuffer, std::size_t... Alignments,
         std::enable_if_t<std::is_function<std::remove_pointer_t<F>>::value && !std::is_member_function_pointer<F>::value>* = nullptr>
void apply(F&& f, Allocator&& a, InBuffer&& s, OutBuffer&& os, std::integer_sequence<std::size_t,Alignments...> = std::integer_sequence<std::size_t>{})
{
    using fargs = typename mpirpc::internal::function_parts<F>::arg_types;
    using ts = typename mpirpc::internal::wrapped_function_parts<F>::storage_types;
    using pass_backs = typename mpirpc::internal::wrapped_function_parts<F>::pass_backs;
    using default_alignments = custom_alignments<typename mpirpc::internal::function_parts<F>::default_alignments,std::integer_sequence<std::size_t,Alignments...>>;
    std::cout << "default alignments: " << abi::__cxa_demangle(typeid(default_alignments).name(),0,0,0) << std::endl;
    constexpr std::size_t num_args = mpirpc::internal::function_parts<F>::num_args;
    apply_impl(std::forward<F>(f),std::forward<Allocator>(a),std::forward<InBuffer>(s), std::forward<OutBuffer>(os), fargs{}, ts{}, pass_backs{}, std::make_index_sequence<num_args>{}, default_alignments{});
}

template<typename F, class Class, typename Allocator, typename InBuffer, typename OutBuffer,
         std::enable_if_t<std::is_member_function_pointer<F>::value>* = nullptr>
void apply(F&& f, Class *c, Allocator&& a, InBuffer&& s, OutBuffer&& os)
{
    using fargs = typename mpirpc::internal::function_parts<F>::arg_types;
    using ts = typename mpirpc::internal::wrapped_function_parts<F>::storage_types;
    using pass_backs = typename mpirpc::internal::wrapped_function_parts<F>::pass_backs;
    using default_alignments = typename mpirpc::internal::function_parts<F>::default_alignments;
    constexpr std::size_t num_args = mpirpc::internal::function_parts<F>::num_args;
    apply_impl(std::forward<F>(f),c,std::forward<Allocator>(a),std::forward<InBuffer>(s), std::forward<OutBuffer>(os), fargs{}, ts{}, pass_backs{}, std::make_index_sequence<num_args>{}, default_alignments{});
}

void foo(double d, int i, float f,  bool b, int(&ai)[4], double(&ad)[3], float(&af)[5], double *pd, B* a)
{
    int ai2[4]{2,4,6,8};
    double ad2[3]{4.6,8.2,9.1};
    float af2[5]{0.2f,2.4f,1.4f,8.7f,3.14f};

    ASSERT_EQ(2.3,d);
    ASSERT_EQ(4,i);
    ASSERT_EQ(1.2f,f);
    ASSERT_EQ(true, b);
    for(std::size_t index = 0; index < 4; ++index)
        ASSERT_EQ(ai2[index],ai[index]);
    for(std::size_t index = 0; index < 3; ++index)
        ASSERT_EQ(ad2[index],ad[index]);
    for(std::size_t index = 0; index < 5; ++index)
        ASSERT_EQ(af2[index],af[index]);
    ASSERT_EQ(3.14159,*pd);
    ASSERT_EQ(8,a->a);
    std::cout << "ran foo" << a->a << std::endl;
}

int foo3()
{
    return 3;
}

void foo4(bool b, i128t i128)
{
    std::cout << alignof(i128) << " " << alignof(i128t) << " " << abi::__cxa_demangle(typeid(i128).name(),0,0,0) << std::endl;
};

TEST(ArgumentUnpacking, test0)
{
    register_polymorphism<A>();
    register_polymorphism<B>();
    parameter_buffer p;
    int ai[4]{2,4,6,8};
    double ad[3]{4.6,8.2,9.1};
    float af[5]{0.2f,2.4f,1.4f,8.7f,3.14f};
    double pd = 3.14159;

    p.put(2.3);
    p.put(4);
    p.put(1.2f);
    p.put(true);
    p.put(ai);
    p.put(ad);
    p.put(af);
    p.put(mpirpc::pointer_wrapper<double>(&pd));
    B b(8,11);
    p.put(mpirpc::pointer_wrapper<B>(&b));
    std::cout << std::hex;
    for (std::size_t i = 0; i < p.position(); ++i)
        std::cout << (unsigned int) p.data()[i] << " ";
    std::cout << std::dec << std::endl;
    p.seek(0);
    std::allocator<char> a;
    parameter_buffer pout;
    apply(&foo,a,p,pout);
    parameter_buffer pout2;
    apply(&foo3,a,p,pout2);
    parameter_buffer p2;
    p2.put(true);
    //p2 << ((i128t) 5);
    p2.put<i128t>((i128t) 4);
    std::cout << std::hex;
    for (std::size_t i = 0; i < p2.position(); ++i)
        std::cout << (unsigned int) p2.data()[i] << " ";
    std::cout << std::dec << std::endl;
    p2.seek(0);
    apply(&foo4,a,p2,pout);
}

TEST(ArgumentUnpacking, test1)
{
    //apply<double,int,float,int[4],double[3],bool,float[5]>();
    using tup = std::tuple<double,int,float,int[4],double[3],bool,float[5]>;
    using type = typename ::mpirpc::internal::detail::argument_storage_info<tup>::mct_tuple;
    using type2 = typename ::mpirpc::internal::detail::argument_storage_info<tup>::nmct_tuple;
    using type3 = typename ::mpirpc::internal::detail::argument_storage_info<tup>::info;
    using type4 = typename ::mpirpc::internal::detail::argument_storage_info<tup>::mct_indexes;
    using type5 = typename ::mpirpc::internal::detail::argument_storage_info<tup>::nmct_indexes;
    using type7 = typename ::mpirpc::internal::detail::argument_storage_info<tup>::split_indexes;
    mpirpc::parameter_stream p;
    int ai[4]{2,4,6,8};
    double ad[3]{4.6,8.2,9.1};
    float af[5]{0.2f,2.4f,1.4f,8.7f,3.14f};
    p << 2.3 << 4 << 1.2f << true << ai << ad << af;
    std::allocator<char> a;
    Foo2 fo;

    type2 t;
    std::cout << abi::__cxa_demangle(typeid(buildtype_tuple_type<double,int,float,bool,int[4],double[3],float[5],mpirpc::pointer_wrapper<double>>).name(),0,0,0) << std::endl;
    alignas(128) int i128;
    using Ai128 = int __attribute__((aligned(32)))[64];
    std::cout << alignof(Ai128) << " " << alignof(int) << " " << abi::__cxa_demangle(typeid(Ai128).name(),0,0,0) << std::endl;
    foo4(true,i128);
    std::cout << alignment_padding<parameter_buffer,false,false,char,int,float,double> << std::endl;
    struct teststruct {
        bool b;
        int i;
        float f;
        float f2;
        double d;
    };
    std::cout << align_buffer_size<parameter_buffer,false,false,char,int,float,double> << " " << sizeof(teststruct) << " " << alignof(double) << " " << alignof(float) << std::endl;
    test_alignment_padding<char,int,float,double>(std::make_index_sequence<4>());
//    std::cout << abi::__cxa_demangle(typeid(tup).name(),0,0,0) << std::endl;
//    std::cout << abi::__cxa_demangle(typeid(type).name(),0,0,0) << std::endl;
//    std::cout << abi::__cxa_demangle(typeid(type2).name(),0,0,0) << std::endl;
//    std::cout << abi::__cxa_demangle(typeid(type3).name(),0,0,0) << std::endl;
//    std::cout << abi::__cxa_demangle(typeid(type4).name(),0,0,0) << std::endl;
//    std::cout << abi::__cxa_demangle(typeid(type5).name(),0,0,0) << std::endl;
//    std::cout << abi::__cxa_demangle(typeid(type7).name(),0,0,0) << std::endl;
//    std::cout << ::mpirpc::internal::get_clamped<0,std::size_t,8>(std::make_index_sequence<5>()) << std::endl;
//    std::cout << ::mpirpc::internal::get_clamped<2,std::size_t,8>(std::make_index_sequence<5>()) << std::endl;
//    std::cout << ::mpirpc::internal::get_clamped<4,std::size_t,8>(std::make_index_sequence<5>()) << std::endl;
//    std::cout << ::mpirpc::internal::get_clamped<6,std::size_t,8>(std::make_index_sequence<5>()) << std::endl;
//    std::cout << ::mpirpc::internal::get_clamped<4,std::size_t,8>(std::make_index_sequence<0>()) << std::endl;
//    std::cout << ::mpirpc::internal::get_clamped<0,std::size_t,6>(type4()) << std::endl;
//    std::cout << ::mpirpc::internal::get_clamped<2,std::size_t,6>(type4()) << std::endl;
//    std::cout << ::mpirpc::internal::get_clamped<3,std::size_t,6>(type4()) << std::endl;
//    std::cout << ::mpirpc::internal::get_clamped<5,std::size_t,6>(type4()) << std::endl;
    type res = ::mpirpc::internal::detail::unmarshal_tuples<std::tuple<double,int,float,bool,int[4],double[3],float[5]>>::unmarshal(a,p,t);
    ASSERT_EQ(2.3,std::get<0>(res));
    ASSERT_EQ(4,std::get<1>(res));
    ASSERT_EQ(1.2f, std::get<2>(res));
    ASSERT_EQ(true, std::get<3>(res));
    for(std::size_t i = 0; i < 4; ++i)
        ASSERT_EQ(ai[i],std::get<0>(t)[i]);
    for(std::size_t i = 0; i < 3; ++i)
        ASSERT_EQ(ad[i],std::get<1>(t)[i]);
    for(std::size_t i = 0; i < 5; ++i)
        ASSERT_EQ(af[i],std::get<2>(t)[i]);
    std::cout << typeid(type3).hash_code() << std::endl;
    std::cout << &typeid(type3) << std::endl;
    //::mpirpc::internal::detail::apply(&foo,res,t);
    //::mpirpc::internal::detail::apply(&Foo2::foo,&fo,res,t);

    //std::cout << abi::__cxa_demangle(typeid(blah).name(),0,0,0) << std::endl;
}

/*TEST(ArgumentUnpacking, test2)
{
    mpirpc::parameter_stream p;
    int ai[4]{2,4,6,8};
    double ad[3]{4.6,8.2,9.1};
    float af[5]{0.2f,2.4f,1.4f,8.7f,3.14f};
    double pd = 3.14159;
    p << 2.3 << 4 << 1.2f << true << ai << ad << af << mpirpc::pointer_wrapper<double>(&pd);
    std::allocator<char> a;
    mpirpc::parameter_stream pout;
    ::mpirpc::internal::apply_stream(&foo,a,p,pout);
}*/

TEST(ParameterBuffer,scalars)
{
    parameter_buffer b{};
    bool boolval = true;
    int intval = 4;
    float floatval = 2.17f;
    double doubleval = 3.14;
    mpirpc::pointer_wrapper<B> bval(new B(7,9));
    b.put(boolval);
    b.put(intval);
    b.put(floatval);
    b.put(doubleval);
    b.put(bval);
    std::cout << "---------------\n";
    //b.put(new B(5,10));
    std::cout << "---------------\n";
    std::cout << std::hex;
    for (std::size_t i = 0; i < b.position(); ++i)
        std::cout << (unsigned int) b.data()[i] << " ";
    std::cout << std::dec << std::endl;
    b.seek(0);
    std::allocator<void> a;
    ASSERT_EQ(true,b.get<bool>(a));
    ASSERT_EQ(4,b.get<int>(a));
    ASSERT_EQ(2.17f,b.get<float>(a));
    //ASSERT_EQ(3.14,b.get<double>(a));
    direct_initializer<double>::placementnew_construct(a,&doubleval,b);
    ASSERT_EQ(3.14,doubleval);
    //ASSERT_EQ((std::tuple<int,int>(7,9)),b.get<B>(a));
    mpirpc::pointer_wrapper<B> *bptr = (mpirpc::pointer_wrapper<B>*) malloc(sizeof(mpirpc::pointer_wrapper<B>));
    //direct_initializer<mpirpc::pointer_wrapper<A>>::placementnew_construct(a,bptr,b);
    //ASSERT_EQ(7,(*bptr)->a);
    //ASSERT_EQ(9,(*bptr)->b);
}



TEST(PolymorphicLookup,test)
{
    constexpr const std::type_info &i = typeid(A);
    register_polymorphism<A>();
    register_polymorphism<B>();
    parameter_buffer b{};
    mpirpc::pointer_wrapper<B> bval(new B(7,9));
    b.put(bval);
    //b.put(type_identifier<B>::id());
    //b.put(7);
    //b.put(9);
    std::cout << std::hex;
    for (std::size_t i = 0; i < b.position(); ++i)
        std::cout << (unsigned int) b.data()[i] << " ";
    std::cout << std::dec << std::endl;
    b.seek(0);
    std::allocator<char> a;
    std::size_t size = get<std::size_t>(b,a);
    uintptr_t id = get<uintptr_t>(b,a);
    B* test = static_cast<B*>(polymorphic_map.at(safe_type_index_map.at(id))->build(a,b,size));
    //constexpr const intptr_t id = i.hash_code();
    ASSERT_EQ(7,test->a);
    ASSERT_EQ(9,test->b);
}

int main(int argc, char **argv) {
    std::cout << std::boolalpha;
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
