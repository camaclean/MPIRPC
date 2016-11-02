#include <gtest/gtest.h>
#include <tuple>
#include <iostream>
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

/*TEST(OrderedCallTest, rref_array)
{
    //int t[3][4]{1,2,3,4,5,6,7,8,9,10,11,12};
    using T = int(&&)[3][4];
    int *p = new int[3*4]{1,2,3,4,5,6,7,8,9,10,11,12};
    //T a1 = std::move(t);
    T a1 = (T) *p;
    std::allocator<void> a;
    {
        mpirpc::internal::ordered_call<decltype(&ordered_call_rref_array),std::allocator<void>> oc{&ordered_call_rref_array, a, a1};
        oc();
    } //make ordered_call go out of scope
}*/

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

void foo(double d, int i, float f, int(&ai)[4], double(&ad)[3], bool b, float(&af)[5], double *pd)
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
    std::cout << "ran foo" << std::endl;
}

class Foo2
{
public:
    void foo(double d, int i, float f, int(&ai)[4], double(&ad)[3], bool b, float(&af)[5], double *pd)
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

template<typename T>
struct buildtype_helper
{
    constexpr static bool value = !std::is_scalar<std::remove_reference_t<T>>::value;
};

template<typename T, std::size_t N>
struct buildtype_helper<T[N]>
{
    constexpr static bool value = !std::is_scalar<std::remove_reference_t<T>>::value;
};

template<typename T>
constexpr bool is_buildtype = buildtype_helper<T>::value;

template<typename...Ts>
struct buildtype_tuple_helper;

template<typename T, typename...Ts>
struct buildtype_tuple_helper<T,Ts...>
{
    using prepend_type = std::conditional_t<is_buildtype<T>,std::tuple<std::add_pointer_t<T>>,std::tuple<>>;
    using next_elems_type = typename buildtype_tuple_helper<Ts...>::type;
    using type = mpirpc::internal::tuple_cat_type<prepend_type, next_elems_type>;
};

template<>
struct buildtype_tuple_helper<>
{
    using type = std::tuple<>;
};

template<typename...Ts>
using buildtype_tuple_type = typename buildtype_tuple_helper<Ts...>::type;

template<typename T, typename = void>
struct marshaller;

template<typename T, typename = void>
struct unmarshaller;

class parameter_buffer
{
public:
    parameter_buffer(std::vector<char> *buf) noexcept
        : m_buffer{buf}, m_position{}
    {}

    template<typename Allocator>
    parameter_buffer(const Allocator& a)
        : m_buffer{new std::vector<char>(a)}, m_position{}
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

    template<typename T>
    void append(const char * start, const char * end)
    {
        std::size_t padding = ((m_position % alignof(T)) ? (alignof(T) - (m_position % alignof(T))) : 0);
        std::size_t delta = padding + (end-start);
        std::size_t new_size = m_buffer->size() + delta;
        std::cout << "padding: " << padding << std::endl;
        m_buffer->reserve(new_size);
        m_buffer->resize(m_buffer->size() + padding);
        m_buffer->insert(m_buffer->end(),start,end);
        m_position += delta;
    }

    template<typename T>
    void realign()
    {
        m_position += (m_position % alignof(T)) ? (alignof(T) - (m_position % alignof(T))) : 0;
    }

    template<typename T, typename Allocator>
    decltype(auto) get(const Allocator& a)
    {
        realign<T>();
        return unmarshaller<T>::unmarshal(a,*this);
    }

    template<typename T>
    void put(T&& t)
    {
        marshaller<T>::marshal(*this,std::forward<T>(t));
    }

protected:
    std::size_t m_position;
    std::vector<char> *m_buffer;
};

template<typename T>
struct marshaller<T,std::enable_if_t<std::is_arithmetic<std::decay_t<T>>::value>>
{
    static void marshal(parameter_buffer& b, const T& val)
    {
        const char* p = reinterpret_cast<const char*>(&val);
        b.append<T>(p, p+sizeof(val));
    }
};

template<typename T>
struct unmarshaller<T,std::enable_if_t<std::is_arithmetic<std::decay_t<T>>::value>>
{
    template<typename Allocator>
    static std::tuple<T> unmarshal(Allocator&&, parameter_buffer& b)
    {
        return std::tuple<std::remove_reference_t<T>>(*b.reinterpret_and_advance<std::remove_reference_t<T>>(sizeof(std::remove_reference_t<T>)));
    }
};

// CRC32 Table (zlib polynomial)
static constexpr uint32_t crc_table[256] = {
    0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L,
    0x706af48fL, 0xe963a535L, 0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L,
    0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L,
    0x90bf1d91L, 0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
    0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L, 0x136c9856L,
    0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L,
    0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L,
    0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
    0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L,
    0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L, 0x26d930acL, 0x51de003aL,
    0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L,
    0xb8bda50fL, 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
    0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 0x76dc4190L,
    0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL,
    0x9fbfe4a5L, 0xe8b8d433L, 0x7807c9a2L, 0x0f00f934L, 0x9609a88eL,
    0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
    0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL,
    0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L, 0x65b0d9c6L, 0x12b7e950L,
    0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L,
    0xfbd44c65L, 0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
    0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL,
    0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L,
    0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713cL, 0x270241aaL, 0xbe0b1010L,
    0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
    0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L,
    0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L,
    0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L,
    0x73dc1683L, 0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
    0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L, 0xf00f9344L,
    0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL,
    0x196c3671L, 0x6e6b06e7L, 0xfed41b76L, 0x89d32be0L, 0x10da7a5aL,
    0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
    0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L,
    0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL, 0xd80d2bdaL, 0xaf0a1b4cL,
    0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL,
    0x4669be79L, 0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
    0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 0xc5ba3bbeL,
    0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L,
    0x2cd99e8bL, 0x5bdeae1dL, 0x9b64c2b0L, 0xec63f226L, 0x756aa39cL,
    0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
    0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL,
    0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 0x86d3d2d4L, 0xf1d4e242L,
    0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L,
    0x18b74777L, 0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
    0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L, 0xa00ae278L,
    0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L,
    0x4969474dL, 0x3e6e77dbL, 0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L,
    0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
    0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L,
    0xcdd70693L, 0x54de5729L, 0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L,
    0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL,
    0x2d02ef8dL
};
template<size_t idx>
constexpr uint32_t crc32(const char * str)
{
    return (crc32<idx-1>(str) >> 8) ^ crc_table[(crc32<idx-1>(str) ^ str[idx]) & 0x000000FF];
}

// This is the stop-recursion function
template<>
constexpr uint32_t crc32<size_t(-1)>(const char * str)
{
    return 0xFFFFFFFF;
}

template<class T>
struct class_identifier
{
    constexpr static const char* name() { return __PRETTY_FUNCTION__; }
    //constexpr static uint32_t id_impl() {
};


class A
{
public:
    A(int v) : a(v) {}
    A(const A&) = delete;
    A(A&&) = delete;
    A() : a(1) {}
    virtual ~A() {}
    int a;
};

class B : public A {
public:
    B(int v, int v2) : A(v), b(v2) {}
    B(const B&) = delete;
    B(B&&) = delete;
    B() : A(), b(2) {}
    virtual ~B() {}
    int b;
};

template<>
struct marshaller<B,void>
{
    static void marshal(parameter_buffer& b, const B& val)
    {
        b.put(1);
        b.put(val.a);
        b.put(val.b);
    }
};

template<>
struct unmarshaller<B,void>
{
    template<typename Allocator>
    static std::tuple<int,int> unmarshal(Allocator&&, parameter_buffer& b)
    {

    }
};

TEST(ParameterBuffer,sc alars)
{
    parameter_buffer b{};
    bool boolval = true;
    int intval = 4;
    float floatval = 2.17f;
    double doubleval = 3.14;
    b.put(boolval);
    b.put(intval);
    b.put(floatval);
    b.put(doubleval);
    std::cout << std::hex;
    for (std::size_t i = 0; i < b.position(); ++i)
        std::cout << (unsigned int) b.data()[i] << " ";
    std::cout << std::dec << std::endl;
    b.seek(0);
    std::allocator<void> a;
    std::cout << class_identifier<Foo>::name() << " " << sizeof(__PRETTY_FUNCTION__) << std::endl;
    ASSERT_EQ(true,std::get<0>(b.get<bool>(a)));
    ASSERT_EQ(std::tuple<int>(4),b.get<int>(a));
    ASSERT_EQ(std::tuple<float>(2.17f),b.get<float>(a));
    ASSERT_EQ(std::tuple<double>(3.14),b.get<double>(a));
}

template<typename T, typename Stream>
void get_pointer_from_stream(Stream&& s, T*& t)
{
    t = reinterpret_cast<T*>(&(s.data()[s.pos()]));
    s.advance(sizeof(T));
}

template<typename T>
struct direct_initializer
{
    template<typename Allocator, typename Stream>
    static void construct(const Allocator& a, T* t, Stream&& s)
    {
        new (t) T();
        s >> *t;
    }

    template<typename Allocator>
    static void destruct(const Allocator& a, T* t)
    {
        t->~T();
    }
};

template<typename T>
struct is_std_allocator;

template <template <typename> class Alloc, typename T>
struct is_std_allocator<Alloc<T>> : std::false_type{};

template <typename T>
struct is_std_allocator<std::allocator<T>> : std::true_type{};

template<typename T>
struct direct_initializer_allocator
{
    template<typename Allocator, typename Stream>
    static void construct(const Allocator& a, T* t, Stream&& s)
    {
        using AllocatorType = typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
        AllocatorType na(a);
        std::allocator_traits<AllocatorType>::construct(na,t);
        s >> *t;
    }
};

template<typename T>
struct direct_initializer<mpirpc::pointer_wrapper<T>>
{
    template<typename Allocator, typename Stream,
             typename U = T, std::enable_if_t<std::is_scalar<U>::value>* = nullptr>
    static void construct(const Allocator&a, mpirpc::pointer_wrapper<U>* t, Stream&&s)
    {
        std::size_t size;
        s >> size;
        bool pass_ownership = false;
        bool pass_back = false;
        U* ptr;
        if (pass_ownership)
        {
            using AllocatorType = typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
            AllocatorType na(a);
            ptr = std::allocator_traits<AllocatorType>::allocate(na,size);
            for (std::size_t i = 0; i < size; ++i)
                direct_initializer_allocator<T>::construct(na,&ptr[i],s);
        }
        else
        {
            get_pointer_from_stream(s,ptr);
        }
        new (t) mpirpc::pointer_wrapper<U>(ptr,size,pass_back,true);
    }

    template<typename Allocator, typename Stream,
             typename U = T, std::enable_if_t<!std::is_scalar<U>::value>* = nullptr>
    static void construct(const Allocator&a, mpirpc::pointer_wrapper<U>* t, Stream&& s)
    {
        std::size_t size;
        s >> size;
        bool pass_back = false;
        bool pass_ownership = false;
        using AllocatorType = typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
        AllocatorType na(a);
        U* ptr = std::allocator_traits<AllocatorType>::allocate(na,size);
        for (std::size_t i = 0; i < size; ++i)
            direct_initializer_allocator<T>::construct(na,&ptr[i],s);
        new (t) mpirpc::pointer_wrapper<U>(ptr,size,pass_back,pass_ownership);
    }

    template<typename Allocator>
    static void destruct(const Allocator& a, mpirpc::pointer_wrapper<T>* t)
    {
        using AllocatorType = typename std::allocator_traits<Allocator>::template rebind_alloc<mpirpc::pointer_wrapper<T>>;
        using AllocatorTypeT = typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
        AllocatorType na(a);
        AllocatorTypeT ta(a);
        if (!t->is_pass_ownership())
        {
            T* ptr = (T*) t;
            for (std::size_t i = 0; i < t->size(); ++i)
                direct_initializer<T>::destruct(ta, &ptr[i]);
            std::allocator_traits<AllocatorType>::destroy(na,t);
        }
    }
};

template<typename T, std::size_t N>
struct direct_initializer<T[N]>
{
    template<typename Allocator, typename Stream, typename U = T, std::size_t M,
             std::enable_if_t<!std::is_array<U>::value>* = nullptr>
    static void construct(const Allocator &a, U(*t)[M], Stream&& s)
    {
        using AllocatorType = typename std::allocator_traits<Allocator>::template rebind_alloc<U>;
        AllocatorType na(a);
        std::cout << "array constructor: " << abi::__cxa_demangle(typeid(na).name(),0,0,0) << std::endl;
        std::cout << "array constructor: " << abi::__cxa_demangle(typeid(T).name(),0,0,0) << std::endl;
        std::cout << "array constructor: " << abi::__cxa_demangle(typeid(t).name(),0,0,0) << std::endl;
        std::cout << "array constructor: " << abi::__cxa_demangle(typeid(&(*t)[2]).name(),0,0,0) << std::endl;
        for (std::size_t i = 0; i < M; ++i)
        {
            direct_initializer<T>::construct(a,&(*t)[i],s);
        }
    }

    template<typename Allocator, typename Stream, typename U = T, std::size_t M,
             std::enable_if_t<std::is_array<U>::value>* = nullptr>
    static void construct(const Allocator &a, U(*t)[M], Stream&& s)
    {
        for(std::size_t i = 0; i < M; ++i)
            direct_initializer<U[M]>::construct(a,&(*t)[i],s);
    }

    template<typename Allocator>
    static void destruct(const Allocator& a, T(*t)[N])
    {
        using AllocatorType = typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
        AllocatorType na(a);
        for (std::size_t i = 0; i < N; ++i)
        {
            std::allocator_traits<AllocatorType>::destroy(na,&(*t)[i]);
        }
    }
};

template<typename T>
struct remarshaller
{
    template<typename Stream, typename U>
    static void marshal(Stream&& s, U&& val)
    {
        mpirpc::marshal(s, val);
    }
};

template<typename Allocator, typename T, typename Stream, std::enable_if_t<is_buildtype<T>>* = nullptr>
void get_from_stream(const Allocator &a, T*& t, Stream&& s)
{
    std::cout << "get from stream: " << abi::__cxa_demangle(typeid(T).name(),0,0,0) << std::endl;
    std::cout << "get from stream: " << abi::__cxa_demangle(typeid(t).name(),0,0,0) << std::endl;
    direct_initializer<T>::construct(a,t,s);
}

template<typename Allocator, typename T, typename Stream, std::enable_if_t<!is_buildtype<T>>* = nullptr>
void get_from_stream(const Allocator &a, T*& t, Stream&& s)
{
    get_pointer_from_stream(s,t);
}

template<typename Allocator, typename T, std::enable_if_t<is_buildtype<T>>* = nullptr>
void cleanup(const Allocator &a, T* t)
{
    direct_initializer<T>::destruct(a,t);
}

template<typename Allocator, typename T, std::enable_if_t<!is_buildtype<T>>* = nullptr>
void cleanup(const Allocator &a, T* t)
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

template<typename F, typename Allocator, typename InStream, typename OutStream, typename...FArgs, typename...Ts, bool... PBs, std::size_t...Is,
         std::enable_if_t<std::is_same<mpirpc::internal::function_return_type<F>,void>::value>* = nullptr>
void apply_impl(F&& f, const Allocator& a, InStream& s, OutStream& os, mpirpc::internal::type_pack<FArgs...>, mpirpc::internal::type_pack<Ts...>, mpirpc::internal::bool_template_list<PBs...>, std::index_sequence<Is...>)
{
    std::tuple<std::add_pointer_t<Ts>...> t{((is_buildtype<Ts>) ? static_cast<std::add_pointer_t<Ts>>(alloca(sizeof(Ts))) : nullptr)...};
    using swallow = int[];
    (void)swallow{(get_from_stream(a,std::get<Is>(t),s), 0)...};
    std::forward<F>(f)(static_cast<FArgs>(*std::get<Is>(t))...);
    (void)swallow{((PBs) ? (remarshaller<FArgs>::marshal(os, *std::get<Is>(t)), 0) : 0)...};
    (void)swallow{(cleanup(a,std::get<Is>(t)), 0)...};
}

template<typename F, typename Allocator, typename InStream, typename OutStream, typename...FArgs, typename...Ts, bool... PBs, std::size_t...Is,
         std::enable_if_t<!std::is_same<mpirpc::internal::function_return_type<F>,void>::value>* = nullptr>
void apply_impl(F&& f, const Allocator& a, InStream& s, OutStream& os, mpirpc::internal::type_pack<FArgs...>, mpirpc::internal::type_pack<Ts...>, mpirpc::internal::bool_template_list<PBs...>, std::index_sequence<Is...>)
{
    std::tuple<std::add_pointer_t<Ts>...> t{((is_buildtype<Ts>) ? static_cast<std::add_pointer_t<Ts>>(alloca(sizeof(Ts))) : nullptr)...};
    using swallow = int[];
    (void)swallow{(get_from_stream(a,std::get<Is>(t),s), 0)...};
    auto&& ret = std::forward<F>(f)(static_cast<FArgs>(*std::get<Is>(t))...);
    os << mpirpc::internal::autowrap<mpirpc::internal::function_return_type<F>,decltype(ret)>(std::move(ret));
    (void)swallow{((PBs) ? (remarshaller<FArgs>::marshal(os, *std::get<Is>(t)), 0) : 0)...};
    (void)swallow{(cleanup(a,std::get<Is>(t)), 0)...};
}

template<typename F,typename Allocator, typename InStream, typename OutStream,
         std::enable_if_t<std::is_function<std::remove_pointer_t<F>>::value && !std::is_member_function_pointer<F>::value>* = nullptr>
void apply(F&& f, const Allocator& a, InStream&& s, OutStream&& os)
{
    using fargs = typename mpirpc::internal::function_parts<F>::arg_types;
    using ts = typename mpirpc::internal::wrapped_function_parts<F>::storage_types;
    using pass_backs = typename mpirpc::internal::wrapped_function_parts<F>::pass_backs;
    constexpr std::size_t num_args = mpirpc::internal::function_parts<F>::num_args;
    apply_impl(std::forward<F>(f),a,std::forward<InStream>(s), std::forward<OutStream>(os), fargs{}, ts{}, pass_backs{}, std::make_index_sequence<num_args>{});
}

template<typename F, class Class, typename Allocator, typename InStream, typename OutStream, typename...FArgs, typename...Ts, bool... PBs, std::size_t...Is,
         std::enable_if_t<std::is_same<mpirpc::internal::function_return_type<F>,void>::value>* = nullptr>
void apply_impl(F&& f, Class *c, const Allocator& a, InStream& s, OutStream& os, mpirpc::internal::type_pack<FArgs...>, mpirpc::internal::type_pack<Ts...>, mpirpc::internal::bool_template_list<PBs...>, std::index_sequence<Is...>)
{
    std::tuple<std::add_pointer_t<Ts>...> t{((is_buildtype<Ts>) ? static_cast<std::add_pointer_t<Ts>>(alloca(sizeof(Ts))) : nullptr)...};
    using swallow = int[];
    (void)swallow{(get_from_stream(a,std::get<Is>(t),s), 0)...};
    ((*c).*(std::forward<F>(f)))(static_cast<FArgs>(*std::get<Is>(t))...);
    (void)swallow{((PBs) ? (remarshaller<FArgs>::marshal(os, *std::get<Is>(t)), 0) : 0)...};
    (void)swallow{(cleanup(a,std::get<Is>(t)), 0)...};
}

template<typename F, class Class, typename Allocator, typename InStream, typename OutStream, typename...FArgs, typename...Ts, bool... PBs, std::size_t...Is,
         std::enable_if_t<!std::is_same<mpirpc::internal::function_return_type<F>,void>::value>* = nullptr>
void apply_impl(F&& f, Class *c, const Allocator& a, InStream& s, OutStream& os, mpirpc::internal::type_pack<FArgs...>, mpirpc::internal::type_pack<Ts...>, mpirpc::internal::bool_template_list<PBs...>, std::index_sequence<Is...>)
{
    std::tuple<std::add_pointer_t<Ts>...> t{((is_buildtype<Ts>) ? static_cast<std::add_pointer_t<Ts>>(alloca(sizeof(Ts))) : nullptr)...};
    using swallow = int[];
    (void)swallow{(get_from_stream(a,std::get<Is>(t),s), 0)...};
    auto&& ret = ((*c).*(std::forward<F>(f)))(static_cast<FArgs>(*std::get<Is>(t))...);
    os << mpirpc::internal::autowrap<mpirpc::internal::function_return_type<F>,decltype(ret)>(std::move(ret));
    (void)swallow{((PBs) ? (remarshaller<FArgs>::marshal(os, *std::get<Is>(t)), 0) : 0)...};
    (void)swallow{(cleanup(a,std::get<Is>(t)), 0)...};
}

template<bool SkipBuildTypes, bool SkipNonBuildTypes, std::size_t Pos, typename...Ts>
struct alignment_padding_helper_impl
{
    using type = std::tuple_element_t<Pos,std::tuple<Ts...>>;
    static constexpr bool predicate = (is_buildtype<type> && !SkipBuildTypes) || (!is_buildtype<type> && !SkipNonBuildTypes);
    static constexpr std::size_t prev_end_address_offset = alignment_padding_helper_impl<SkipBuildTypes,SkipNonBuildTypes,Pos-1,Ts...>::end_address_offset;
    static constexpr std::size_t delta = (predicate && (prev_end_address_offset % alignof(type))) ? alignof(type) - prev_end_address_offset % alignof(type) : 0;
    static constexpr std::size_t start_address_offset = (predicate) ? prev_end_address_offset + delta : 0;
    static constexpr std::size_t end_address_offset = (predicate) ? start_address_offset + sizeof(type) : 0;
    static constexpr std::size_t total_padding = alignment_padding_helper_impl<SkipBuildTypes,SkipNonBuildTypes,Pos-1,Ts...>::total_padding + delta;
    static constexpr std::size_t total_size = alignment_padding_helper_impl<SkipBuildTypes,SkipNonBuildTypes,Pos-1,Ts...>::total_size + delta + sizeof(type);
};

template<bool SkipBuildTypes, bool SkipNonBuildTypes, typename T, typename...Ts>
struct alignment_padding_helper_impl<SkipBuildTypes,SkipNonBuildTypes,0,T,Ts...>
{
    using type = T;
    static constexpr bool predicate = (is_buildtype<type> && !SkipBuildTypes) || (!is_buildtype<type> && !SkipNonBuildTypes);
    static constexpr std::size_t prev_end_address_offset = 0;
    static constexpr std::size_t delta = 0;
    static constexpr std::size_t start_address_offset = 0;
    static constexpr std::size_t end_address_offset = (predicate) ? sizeof(type) : 0;
    static constexpr std::size_t total_padding = 0;
    static constexpr std::size_t total_size = sizeof(type);
};

template<typename...Ts,std::size_t...Is>
void test_alignment_padding(std::index_sequence<Is...>)
{
    using swallow = int[];
    std::cout << "-------" << std::endl;
    (void)swallow{( std::cout << alignment_padding_helper_impl<false,false,Is,Ts...>::delta << std::endl, 0)...};
    std::cout << "-------" << std::endl;
}

template<bool SkipBuildTypes, bool SkipNonBuildTypes, typename... Ts>
constexpr std::size_t alignment_padding = alignment_padding_helper_impl<SkipBuildTypes, SkipNonBuildTypes, sizeof...(Ts)-1,Ts...>::total_padding;

template<bool SkipBuildTypes, bool SkipNonBuildTypes, typename... Ts>
constexpr std::size_t align_buffer_size = alignment_padding_helper_impl<SkipBuildTypes, SkipNonBuildTypes, sizeof...(Ts)-1,Ts...>::total_size;

/**
 * Mechanism:
 *
 * Applying a datagram as arguments with any type to a function is a non-trivial task. The naive approach would be to unpack
 * the stream into a tuple, then apply those tuple arguments to a function. However, this approach has some notable limitations
 * and inefficiencies. First of all, it is not possible to construct a std::tuple with a mix of any type. Take, for instance, a
 * std::tuple containing an array and an mpirpc::pointer_wrapper. An array type is not MoveConstructible or CopyConstructible.
 * The array could be initialized and then the values from the stream applied to it, but mpirpc::pointer_wrapper is not
 * DefaultConstructible. An array type also can't be returned by an unmarshalling function and references to an array can't be
 * used, either, unless it was initialized outside of the unmarshaller. Otherwise, the result would be a dangling reference. An
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
 * initialized depends on the type. If is_buildtype<T> is true for that type, then the std::tuple element will be initialized
 * to a pointer within a stack buffer which is properly aligned for an element of that type. If is_buildtype<T> is false, then
 * the pointer points to the location of the data in the stream buffer. By default, is_buildtype<T> is true if T is a scalar
 * type or an array of scalar types. Otherwise, it is false. Users may specialize is_buildtype<T> to a boolean true value for
 * custom types that can be correctly accessed by a reinterpret_cast<T*> on the buffer location (objects with standard layout
 * and no pointer/reference type member variables).
 *
 * Next, types for which is_buildtype<T> is true call direct_initializer<T>::construct(Allocator,T*,Stream). The generic
 * implementation is to default construct the type using placement new, then use the stream operator>> to set the variable state.
 * Alternatively, direct_initializer<T> may be specialized by the user so that the data read from the stream is used to directly initialize T using
 * operator new.
 *
 */
template<typename F, class Class, typename Allocator, typename InStream, typename OutStream,
         std::enable_if_t<std::is_member_function_pointer<F>::value>* = nullptr>
void apply(F&& f, Class *c, const Allocator& a, InStream&& s, OutStream&& os)
{
    using fargs = typename mpirpc::internal::function_parts<F>::arg_types;
    using ts = typename mpirpc::internal::wrapped_function_parts<F>::storage_types;
    using pass_backs = typename mpirpc::internal::wrapped_function_parts<F>::pass_backs;
    constexpr std::size_t num_args = mpirpc::internal::function_parts<F>::num_args;
    apply_impl(std::forward<F>(f),c,a,std::forward<InStream>(s), std::forward<OutStream>(os), fargs{}, ts{}, pass_backs{}, std::make_index_sequence<num_args>{});
}

int foo3()
{
    return 3;
}

TEST(ArgumentUnpacking, test0)
{
    mpirpc::parameter_stream p;
    int ai[4]{2,4,6,8};
    double ad[3]{4.6,8.2,9.1};
    float af[5]{0.2f,2.4f,1.4f,8.7f,3.14f};
    double pd = 3.14159;
    p << 2.3 << 4 << 1.2f << ai << ad << true << af << mpirpc::pointer_wrapper<double>(&pd);
    std::allocator<void> a;
    mpirpc::parameter_stream pout;
    apply(&foo,a,p,pout);
    mpirpc::parameter_stream pout2;
    apply(&foo3,a,p,pout2);
}

using i128t = int __attribute__((aligned(128)));

void foo4(i128t i128)
{
    std::cout << alignof(i128) << " " << alignof(i128t) << " " << abi::__cxa_demangle(typeid(i128).name(),0,0,0) << std::endl;
};

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
    p << 2.3 << 4 << 1.2f << ai << ad << true << af;
    std::allocator<void> a;
    Foo2 fo;

    type2 t;
    std::cout << abi::__cxa_demangle(typeid(buildtype_tuple_type<double,int,float,int[4],double[3],bool, float[5],mpirpc::pointer_wrapper<double>>).name(),0,0,0) << std::endl;
    alignas(128) int i128;
    std::cout << alignof(i128) << " " << alignof(int) << " " << abi::__cxa_demangle(typeid(i128).name(),0,0,0) << std::endl;
    foo4(i128);
    std::cout << alignment_padding<false,false,char,int,float,double> << std::endl;
    struct teststruct {
        bool b;
        int i;
        float f;
        float f2;
        double d;
    };
    std::cout << align_buffer_size<false,false,char,int,float,double> << " " << sizeof(teststruct) << " " << alignof(double) << " " << alignof(float) << std::endl;
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
    type res = ::mpirpc::internal::detail::unmarshal_tuples<std::tuple<double,int,float,int[4],double[3],bool,float[5]>>::unmarshal(a,p,t);
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

TEST(ArgumentUnpacking, test2)
{
    mpirpc::parameter_stream p;
    int ai[4]{2,4,6,8};
    double ad[3]{4.6,8.2,9.1};
    float af[5]{0.2f,2.4f,1.4f,8.7f,3.14f};
    double pd = 3.14159;
    p << 2.3 << 4 << 1.2f << ai << ad << true << af << mpirpc::pointer_wrapper<double>(&pd);
    std::allocator<void> a;
    mpirpc::parameter_stream pout;
    ::mpirpc::internal::apply_stream(&foo,a,p,pout);
}

int main(int argc, char **argv) {
    std::cout << std::boolalpha;
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
