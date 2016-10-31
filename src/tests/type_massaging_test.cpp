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

template<typename Element, typename... NextElements>
class piecewise_tuple_holder
{
    using element_type = Element;
    using next_type = piecewise_tuple_holder<NextElements...>;

public:
protected:
    element_type m_element;
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

template<typename T, typename Stream>
void get_pointer_from_stream(Stream&& s, T*& t)
{
    t = reinterpret_cast<T*>(&(s.data()[s.pos()]));
    s.advance(sizeof(T));
}

template<typename T>
struct stream_constructor
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
struct unmarshal_allocator
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
struct stream_constructor<mpirpc::pointer_wrapper<T>>
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
                unmarshal_allocator<T>::construct(na,&ptr[i],s);
        }
        else
        {
            get_pointer_from_stream(s,ptr);
        }
        new (t) mpirpc::pointer_wrapper<U>(ptr,size,pass_back,true);
    }

    template<typename Allocator, typename Stream,
             typename U = T, std::enable_if_t<!std::is_scalar<U>::value>* = nullptr>
    static void construct(const Allocator&a, mpirpc::pointer_wrapper<U>* t, Stream&&s)
    {
        std::size_t size;
        s >> size;
        bool pass_back = false;
        bool pass_ownership = false;
        using AllocatorType = typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
        AllocatorType na(a);
        U* ptr = std::allocator_traits<AllocatorType>::allocate(na,size);
        for (std::size_t i = 0; i < size; ++i)
            unmarshal_allocator<T>::construct(na,&ptr[i],s);
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
                stream_constructor<T>::destruct(ta, &ptr[i]);
            std::allocator_traits<AllocatorType>::destroy(na,t);
        }
    }
};

template<typename T, std::size_t N>
struct stream_constructor<T[N]>
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
            stream_constructor<T>::construct(a,&(*t)[i],s);
        }
    }

    template<typename Allocator, typename Stream, typename U = T, std::size_t M,
             std::enable_if_t<std::is_array<U>::value>* = nullptr>
    static void construct(const Allocator &a, U(*t)[M], Stream&& s)
    {
        for(std::size_t i = 0; i < M; ++i)
            stream_constructor<U[M]>::construct(a,&(*t)[i],s);
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

template<typename Allocator, typename T, typename Stream, std::enable_if_t<is_buildtype<T>>* = nullptr>
void get_from_stream(const Allocator &a, T*& t, Stream&& s)
{
    std::cout << "get from stream: " << abi::__cxa_demangle(typeid(T).name(),0,0,0) << std::endl;
    std::cout << "get from stream: " << abi::__cxa_demangle(typeid(t).name(),0,0,0) << std::endl;
    stream_constructor<T>::construct(a,t,s);
}

template<typename Allocator, typename T, typename Stream, std::enable_if_t<!is_buildtype<T>>* = nullptr>
void get_from_stream(const Allocator &a, T*& t, Stream&& s)
{
    get_pointer_from_stream(s,t);
}

template<typename Allocator, typename T, std::enable_if_t<is_buildtype<T>>* = nullptr>
void cleanup(const Allocator &a, T* t)
{
    stream_constructor<T>::destruct(a,t);
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
    (void)swallow{((PBs) ? (mpirpc::marshal(os, *std::get<Is>(t)), 0) : 0)...};
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
    (void)swallow{((PBs) ? mpirpc::marshal(os, *std::get<Is>(t)), 0 : 0)...};
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
    (void)swallow{((PBs) ? (mpirpc::marshal(os, *std::get<Is>(t)), 0) : 0)...};
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
    (void)swallow{((PBs) ? mpirpc::marshal(os, *std::get<Is>(t)), 0 : 0)...};
    (void)swallow{(cleanup(a,std::get<Is>(t)), 0)...};
}

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
