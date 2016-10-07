#include <gtest/gtest.h>
#include <tuple>
#include <iostream>
#include "../internal/function_attributes.hpp"
#include "../internal/marshalling.hpp"
#include "../internal/parameterstream.hpp"
#include "../internal/unmarshalling.hpp"

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

TEST_STORAGE_TUPLE_TYPE(none    ,void(*)()           , std::tuple<>         )
TEST_STORAGE_TUPLE_TYPE(scalar  ,void(*)(int)        , std::tuple<int>      )
TEST_STORAGE_TUPLE_TYPE(cscalar ,void(*)(const int)  , std::tuple<int>      )
TEST_STORAGE_TUPLE_TYPE(pscalar ,void(*)(int*)       , std::tuple<int*>     )
TEST_STORAGE_TUPLE_TYPE(lscalar ,void(*)(int&)       , std::tuple<int>      )
TEST_STORAGE_TUPLE_TYPE(rscalar ,void(*)(int&&)      , std::tuple<int>      )
TEST_STORAGE_TUPLE_TYPE(Ascalar ,void(*)(int[4])     , std::tuple<int*>     )
TEST_STORAGE_TUPLE_TYPE(Alscalar,void(*)(int(&)[4])  , std::tuple<int[4]>   )
TEST_STORAGE_TUPLE_TYPE(Arscalar,void(*)(int(&&)[4]) , std::tuple<int[4]>   )
TEST_STORAGE_TUPLE_TYPE(Achar   ,void(*)(char[4])    , std::tuple<char*>    )
TEST_STORAGE_TUPLE_TYPE(Alchar  ,void(*)(char(&)[4]) , std::tuple<char[4]>  )
TEST_STORAGE_TUPLE_TYPE(Archar  ,void(*)(char(&&)[4]), std::tuple<char[4]>  )


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

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lANT_pT    , double(&)[5]       , double *              , SINGLE_ARG(mpirpc::pointer_wrapper<double,5,false,true >),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lANT_cpT   , double(&)[5]       , double *const         , SINGLE_ARG(mpirpc::pointer_wrapper<double,5,false,true >),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lANT_pcT   , double(&)[5]       , const double *        , SINGLE_ARG(mpirpc::pointer_wrapper<double,5,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lANT_cpcT  , double(&)[5]       , const double *const   , SINGLE_ARG(mpirpc::pointer_wrapper<double,5,false,false>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lANT_lpT   , double(&)[5]       , double *&             , SINGLE_ARG(mpirpc::pointer_wrapper<double,5,false,true >),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lANT_lcpT  , double(&)[5]       , double *const &       , SINGLE_ARG(mpirpc::pointer_wrapper<double,5,false,true >),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lANT_lpcT  , double(&)[5]       , const double *&       , SINGLE_ARG(mpirpc::pointer_wrapper<double,5,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lANT_lcpcT , double(&)[5]       , const double *const & , SINGLE_ARG(mpirpc::pointer_wrapper<double,5,false,false>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lANT_rpT   , double(&)[5]       , double *&&            , SINGLE_ARG(mpirpc::pointer_wrapper<double,5,true ,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lANT_rcpT  , double(&)[5]       , double *const &&      , SINGLE_ARG(mpirpc::pointer_wrapper<double,5,true ,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lANT_rpcT  , double(&)[5]       , const double *&&      , SINGLE_ARG(mpirpc::pointer_wrapper<double,5,true ,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lANT_rcpcT , double(&)[5]       , const double *const &&, SINGLE_ARG(mpirpc::pointer_wrapper<double,5,true ,false>),false)


TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lANcT_pT   , const double(&)[5] , double *              , SINGLE_ARG(mpirpc::pointer_wrapper<double,5,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lANcT_cpT  , const double(&)[5] , double *const         , SINGLE_ARG(mpirpc::pointer_wrapper<double,5,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lANcT_pcT  , const double(&)[5] , const double *        , SINGLE_ARG(mpirpc::pointer_wrapper<double,5,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lANcT_cpcT , const double(&)[5] , const double *const   , SINGLE_ARG(mpirpc::pointer_wrapper<double,5,false,false>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lANcT_lpT  , const double(&)[5] , double *&             , SINGLE_ARG(mpirpc::pointer_wrapper<double,5,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lANcT_lcpT , const double(&)[5] , double *const &       , SINGLE_ARG(mpirpc::pointer_wrapper<double,5,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lANcT_lpcT , const double(&)[5] , const double *&       , SINGLE_ARG(mpirpc::pointer_wrapper<double,5,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lANcT_lcpcT, const double(&)[5] , const double *const & , SINGLE_ARG(mpirpc::pointer_wrapper<double,5,false,false>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lANcT_rpT  , const double(&)[5] , double *&&            , SINGLE_ARG(mpirpc::pointer_wrapper<double,5,true ,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lANcT_rcpT , const double(&)[5] , double *const &&      , SINGLE_ARG(mpirpc::pointer_wrapper<double,5,true ,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lANcT_rpcT , const double(&)[5] , const double *&&      , SINGLE_ARG(mpirpc::pointer_wrapper<double,5,true ,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lANcT_rcpcT, const double(&)[5] , const double *const &&, SINGLE_ARG(mpirpc::pointer_wrapper<double,5,true ,false>),false)



TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rANT_pT    , double(&&)[5]      , double *              , SINGLE_ARG(mpirpc::pointer_wrapper<double,5,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rANT_cpT   , double(&&)[5]      , double *const         , SINGLE_ARG(mpirpc::pointer_wrapper<double,5,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rANT_pcT   , double(&&)[5]      , const double *        , SINGLE_ARG(mpirpc::pointer_wrapper<double,5,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rANT_cpcT  , double(&&)[5]      , const double *const   , SINGLE_ARG(mpirpc::pointer_wrapper<double,5,false,false>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rANT_lpT   , double(&&)[5]      , double *&             , SINGLE_ARG(mpirpc::pointer_wrapper<double,5,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rANT_lcpT  , double(&&)[5]      , double *const &       , SINGLE_ARG(mpirpc::pointer_wrapper<double,5,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rANT_lpcT  , double(&&)[5]      , const double *&       , SINGLE_ARG(mpirpc::pointer_wrapper<double,5,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rANT_lcpcT , double(&&)[5]      , const double *const & , SINGLE_ARG(mpirpc::pointer_wrapper<double,5,false,false>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rANT_rpT   , double(&&)[5]      , double *&&            , SINGLE_ARG(mpirpc::pointer_wrapper<double,5,true ,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rANT_rcpT  , double(&&)[5]      , double *const &&      , SINGLE_ARG(mpirpc::pointer_wrapper<double,5,true ,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rANT_rpcT  , double(&&)[5]      , const double *&&      , SINGLE_ARG(mpirpc::pointer_wrapper<double,5,true ,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rANT_rcpcT , double(&&)[5]      , const double *const &&, SINGLE_ARG(mpirpc::pointer_wrapper<double,5,true ,false>),false)


TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rANcT_pT   , const double(&&)[5], double *              , SINGLE_ARG(mpirpc::pointer_wrapper<double,5,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rANcT_cpT  , const double(&&)[5], double *const         , SINGLE_ARG(mpirpc::pointer_wrapper<double,5,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rANcT_pcT  , const double(&&)[5], const double *        , SINGLE_ARG(mpirpc::pointer_wrapper<double,5,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rANcT_cpcT , const double(&&)[5], const double *const   , SINGLE_ARG(mpirpc::pointer_wrapper<double,5,false,false>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rANcT_lpT  , const double(&&)[5], double *&             , SINGLE_ARG(mpirpc::pointer_wrapper<double,5,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rANcT_lcpT , const double(&&)[5], double *const &       , SINGLE_ARG(mpirpc::pointer_wrapper<double,5,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rANcT_lpcT , const double(&&)[5], const double *&       , SINGLE_ARG(mpirpc::pointer_wrapper<double,5,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rANcT_lcpcT, const double(&&)[5], const double *const & , SINGLE_ARG(mpirpc::pointer_wrapper<double,5,false,false>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rANcT_rpT  , const double(&&)[5], double *&&            , SINGLE_ARG(mpirpc::pointer_wrapper<double,5,true ,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rANcT_rcpT , const double(&&)[5], double *const &&      , SINGLE_ARG(mpirpc::pointer_wrapper<double,5,true ,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rANcT_rpcT , const double(&&)[5], const double *&&      , SINGLE_ARG(mpirpc::pointer_wrapper<double,5,true ,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rANcT_rcpcT, const double(&&)[5], const double *const &&, SINGLE_ARG(mpirpc::pointer_wrapper<double,5,true ,false>),false)





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


TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, pT_pT      , double *              , double *              , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,true >),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, pT_cpT     , double *              , double *const         , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,true >),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, pT_pcT     , double *              , const double *        , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, pT_cpcT    , double *              , const double *const   , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, pcT_pT     , const double *        , double *              , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, pcT_cpT    , const double *        , double *const         , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, pcT_pcT    , const double *        , const double *        , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, pcT_cpcT   , const double *        , const double *const   , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, cpT_pT     , double *const         , double *              , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,true >),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, cpT_cpT    , double *const         , double *const         , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,true >),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, cpT_pcT    , double *const         , const double *        , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, cpT_cpcT   , double *const         , const double *const   , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, cpcT_pT    , const double *const   , double *              , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, cpcT_cpT   , const double *const   , double *const         , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, cpcT_pcT   , const double *const   , const double *        , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, cpcT_cpcT  , const double *const   , const double *const   , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)


TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lpT_pT     , double *&             , double *              , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,true >),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lpT_cpT    , double *&             , double *const         , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,true >),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lpT_pcT    , double *&             , const double *        , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lpT_cpcT   , double *&             , const double *const   , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lpcT_pT    , const double *&       , double *              , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lpcT_cpT   , const double *&       , double *const         , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lpcT_pcT   , const double *&       , const double *        , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lpcT_cpcT  , const double *&       , const double *const   , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lcpT_pT    , double *const &       , double *              , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,true >),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lcpT_cpT   , double *const &       , double *const         , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,true >),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lcpT_pcT   , double *const &       , const double *        , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lcpT_cpcT  , double *const &       , const double *const   , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lcpcT_pT   , const double *const & , double *              , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lcpcT_cpT  , const double *const & , double *const         , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lcpcT_pcT  , const double *const & , const double *        , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lcpcT_cpcT , const double *const & , const double *const   , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)


TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rpT_pT     , double *&&            , double *              , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,true >),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rpT_cpT    , double *&&            , double *const         , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,true >),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rpT_pcT    , double *&&            , const double *        , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rpT_cpcT   , double *&&            , const double *const   , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rpcT_pT    , const double *&&      , double *              , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rpcT_cpT   , const double *&&      , double *const         , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rpcT_pcT   , const double *&&      , const double *        , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rpcT_cpcT  , const double *&&      , const double *const   , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rcpT_pT    , double *const &&      , double *              , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,true >),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rcpT_cpT   , double *const &&      , double *const         , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,true >),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rcpT_pcT   , double *const &&      , const double *        , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rcpT_cpcT  , double *const &&      , const double *const   , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rcpcT_pT   , const double *const &&, double *              , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rcpcT_cpT  , const double *const &&, double *const         , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rcpcT_pcT  , const double *const &&, const double *        , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rcpcT_cpcT , const double *const &&, const double *const   , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)



TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, pT_lpT     , double *              , double *&             , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,true >),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, pT_lcpT    , double *              , double *const &       , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,true >),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, pT_lpcT    , double *              , const double *&       , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, pT_lcpcT   , double *              , const double *const & , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, pcT_lpT    , const double *        , double *&             , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, pcT_lcpT   , const double *        , double *const &       , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, pcT_lpcT   , const double *        , const double *&       , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, pcT_lcpcT  , const double *        , const double *const & , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, cpT_lpT    , double *const         , double * &            , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,true >),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, cpT_lcpT   , double *const         , double *const &       , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,true >),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, cpT_lpcT   , double *const         , const double * &      , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, cpT_lcpcT  , double *const         , const double *const & , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, cpcT_lpT   , const double *const   , double *&             , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, cpcT_lcpT  , const double *const   , double *const &       , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, cpcT_lpcT  , const double *const   , const double *&       , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, cpcT_lcpcT , const double *const   , const double *const & , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)


TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lpT_lpT    , double *&             , double *&             , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,true >),true )
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lpT_lcpT   , double *&             , double *const &       , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,true >),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lpT_lpcT   , double *&             , const double *&       , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),true )
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lpT_lcpcT  , double *&             , const double *const & , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lpcT_lpT   , const double *&       , double *&             , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),true )
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lpcT_lcpT  , const double *&       , double *const &       , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lpcT_lpcT  , const double *&       , const double *&       , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),true )
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lpcT_lcpcT , const double *&       , const double *const & , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lcpT_lpT   , double *const &       , double *&             , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,true >),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lcpT_lcpT  , double *const &       , double *const &       , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,true >),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lcpT_lpcT  , double *const &       , const double *&       , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lcpT_lcpcT , double *const &       , const double *const & , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lcpcT_lpT  , const double *const & , double *&             , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lcpcT_lcpT , const double *const & , double *const &       , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lcpcT_lpcT , const double *const & , const double *&       , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, lcpcT_lcpcT, const double *const & , const double *const & , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)


TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rpT_lpT    , double *&&            , double *&             , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,true >),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rpT_lcpT   , double *&&            , double *const &       , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,true >),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rpT_lpcT   , double *&&            , const double *&       , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rpT_lcpcT  , double *&&            , const double *const & , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rpcT_lpT   , const double *&&      , double *&             , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rpcT_lcpT  , const double *&&      , double *const &       , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rpcT_lpcT  , const double *&&      , const double *&       , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rpcT_lcpcT , const double *&&      , const double *const & , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rcpT_lpT   , double *const &&      , double *&             , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,true >),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rcpT_lcpT  , double *const &&      , double *const &       , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,true >),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rcpT_lpcT  , double *const &&      , const double *&       , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rcpT_lcpcT , double *const &&      , const double *const & , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rcpcT_lpT  , const double *const &&, double *&             , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rcpcT_lcpT , const double *const &&, double *const &       , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rcpcT_lpcT , const double *const &&, const double *&       , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rcpcT_lcpcT, const double *const &&, const double *const & , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,false,false>),false)



TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, pT_rpT     , double *              , double *&&            , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,true ,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, pT_rcpT    , double *              , double *const &&      , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,true ,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, pT_rpcT    , double *              , const double *&&      , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,true ,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, pT_rcpcT   , double *              , const double *const &&, SINGLE_ARG(mpirpc::pointer_wrapper<double,1,true ,false>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, pcT_rpT    , const double *        , double *&&            , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,true ,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, pcT_rcpT   , const double *        , double *const &&      , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,true ,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, pcT_rpcT   , const double *        , const double *&&      , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,true ,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, pcT_rcpcT  , const double *        , const double *const &&, SINGLE_ARG(mpirpc::pointer_wrapper<double,1,true ,false>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, cpT_rpT    , double *const         , double *&&            , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,true ,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, cpT_rcpT   , double *const         , double *const &&      , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,true ,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, cpT_rpcT   , double *const         , const double *&&      , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,true ,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, cpT_rcpcT  , double *const         , const double *const &&, SINGLE_ARG(mpirpc::pointer_wrapper<double,1,true ,false>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, cpcT_rpT   , const double *const   , double *&&            , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,true ,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, cpcT_rcpT  , const double *const   , double *const &&      , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,true ,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, cpcT_rpcT  , const double *const   , const double *&&      , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,true ,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, cpcT_rcpcT , const double *const   , const double *const &&, SINGLE_ARG(mpirpc::pointer_wrapper<double,1,true ,false>),false)


TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rpT_rpT    , double *&&            , double *&&            , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,true ,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rpT_rcpT   , double *&&            , double *const &&      , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,true ,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rpT_rpcT   , double *&&            , const double *&&      , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,true ,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rpT_rcpcT  , double *&&            , const double *const &&, SINGLE_ARG(mpirpc::pointer_wrapper<double,1,true ,false>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rpcT_rpT   , const double *&&      , double *&&            , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,true ,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rpcT_rcpT  , const double *&&      , double *const &&      , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,true ,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rpcT_rpcT  , const double *&&      , const double *&&      , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,true ,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rpcT_rcpcT , const double *&&      , const double *const &&, SINGLE_ARG(mpirpc::pointer_wrapper<double,1,true ,false>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rcpT_rpT   , double *const &&      , double *&&            , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,true ,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rcpT_rcpT  , double *const &&      , double *const &&      , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,true ,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rcpT_rpcT  , double *const &&      , const double *&&      , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,true ,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rcpT_rcpcT , double *const &&      , const double *const &&, SINGLE_ARG(mpirpc::pointer_wrapper<double,1,true ,false>),false)

TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rcpcT_rpT  , const double *const &&, double *&&            , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,true ,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rcpcT_rcpT , const double *const &&, double *const &&      , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,true ,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rcpcT_rpcT , const double *const &&, const double *&&      , SINGLE_ARG(mpirpc::pointer_wrapper<double,1,true ,false>),false)
TEST_REFERENCE_TYPE(ChooseReferenceTypeTest, rcpcT_rcpcT, const double *const &&, const double *const &&, SINGLE_ARG(mpirpc::pointer_wrapper<double,1,true ,false>),false)




TEST_WRAPPED_FUNCTION_TYPE(basic                  , void(*)()                , void(*)()                 )
TEST_WRAPPED_FUNCTION_TYPE(primitive              , void(*)(double)          , void(*)(double)           )
TEST_WRAPPED_FUNCTION_TYPE(ref_to_primitive       , void(*)(double&)         , void(*)(double&)          )
TEST_WRAPPED_FUNCTION_TYPE(rref_to_primitive      , void(*)(double&&)        , void(*)(double&&)         )
TEST_WRAPPED_FUNCTION_TYPE(const_primitive        , void(*)(const double)    , void(*)(const double)     )
TEST_WRAPPED_FUNCTION_TYPE(return_primitive       , int(*)()                 , int(*)()                  )
TEST_WRAPPED_FUNCTION_TYPE(pointer                , void(*)(double*)         , void(*)(double*)          )
TEST_WRAPPED_FUNCTION_TYPE(return_pointer         , int*(*)()                , int*(*)()                 )
TEST_WRAPPED_FUNCTION_TYPE(array                  , void(*)(double[4])       , void(*)(double*)          )
TEST_WRAPPED_FUNCTION_TYPE(pointer_to_array       , void(*)(double(*)[4])    , void(*)(double((*))[4])   )
TEST_WRAPPED_FUNCTION_TYPE(ref_to_array           , void(*)(double(&)[4])    , void(*)(double(&)[4])     )
TEST_WRAPPED_FUNCTION_TYPE(rref_to_array          , void(*)(double(&&)[4])   , void(*)(double(&&)[4])    )
TEST_WRAPPED_FUNCTION_TYPE(return_pointer_to_array, int(*(*)())[4]           , int(*(*)())[4]            )
TEST_WRAPPED_FUNCTION_TYPE(return_ref_to_array    , int(&(*)())[4]           , int(&(*)())[4]            )
TEST_WRAPPED_FUNCTION_TYPE(return_rref_to_array   , int(&&(*)())[4]          , int(&&(*)())[4]           )
TEST_WRAPPED_FUNCTION_TYPE(pchar                  , void(*)(char*)           , void(*)(char*)            )
TEST_WRAPPED_FUNCTION_TYPE(pcchar                 , void(*)(const char*)     , void(*)(const char*)      )
TEST_WRAPPED_FUNCTION_TYPE(Achar                  , void(*)(char(&)[4])      , void(*)(char(&)[4])       )
TEST_WRAPPED_FUNCTION_TYPE(Acchar                 , void(*)(const char(&)[4]), void(*)(const char(&)[4]) )



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
        std::cout << abi::__cxa_demangle(typeid(StorageTupleType).name(),0,0,0) << std::endl; \
        std::cout << abi::__cxa_demangle(typeid(argument_tuple).name(),0,0,0) << std::endl; \
        ASSERT_EQ(argument_tuple,st); \
    }

double test_double = 3.0;
float  test_float  = 3.0f;
int    test_int    = 3;
TEST_FN_TYPE_MARSHALLER(T_T  ,void(*)(double,float,int),3.0,3.0f,3)
TEST_FN_TYPE_MARSHALLER(lT_T ,void(*)(double,float,int),test_double,test_float,test_int)
TEST_FN_TYPE_MARSHALLER(T_U  ,void(*)(double,float,int),3,3.0,3.0f)
TEST_FN_TYPE_MARSHALLER(lT_U ,void(*)(double,float,int),test_int,test_double,test_float)
TEST_FN_TYPE_MARSHALLER(T_lT ,void(*)(double&,float&,int&),3.0,3.0f,3)
TEST_FN_TYPE_MARSHALLER(lT_lT,void(*)(double&,float&,int&),test_double,test_float,test_int)
TEST_FN_TYPE_MARSHALLER(T_lU ,void(*)(double&,float&,int&),3,3.0,3.0f)
TEST_FN_TYPE_MARSHALLER(lT_lU,void(*)(double&,float&,int&),test_int,test_double,test_float)
TEST_FN_TYPE_MARSHALLER(T_rT ,void(*)(double&&,float&&,int&&),3.0,3.0f,3)
TEST_FN_TYPE_MARSHALLER(T_rU ,void(*)(double&&,float&&,int&&),3,3.0,3.0f)


TEST(FnTypeMarshaller, p5T_pT)
{
    double* p1 = new double[5]{5.0,6.0,7.0,8.0,9.0};
    using F = void(*)(double*);
    using StorageTupleType = typename ::mpirpc::internal::wrapped_function_parts<F>::storage_tuple_type;
    std::cout << abi::__cxa_demangle(typeid(StorageTupleType).name(),0,0,0) << std::endl;


    mpirpc::parameter_stream s;
    mpirpc::internal::fn_type_marshaller<F>::marshal(s, mpirpc::pointer_wrapper<double>(p1,5));
    std::allocator<void> a;
    StorageTupleType st(mpirpc::internal::tuple_unmarshaller_remote<double*>::unmarshal(a,s));
    double *a1 = std::get<0>(st);
    for (std::size_t i = 0; i < 5; ++i)
        ASSERT_EQ(p1[i],a1[i]);
}

TEST(FnTypeMarshaller, p5T_lpT)
{
    double* p1 = new double[5]{5.0,6.0,7.0,8.0,9.0};
    using F = void(*)(double*&);
    using StorageTupleType = typename ::mpirpc::internal::wrapped_function_parts<F>::storage_tuple_type;
    std::cout << abi::__cxa_demangle(typeid(StorageTupleType).name(),0,0,0) << std::endl;

    mpirpc::parameter_stream s;
    mpirpc::internal::fn_type_marshaller<F>::marshal(s, mpirpc::pointer_wrapper<double>(p1,5));
    std::allocator<void> a;
    StorageTupleType st(mpirpc::internal::tuple_unmarshaller_remote<double*&>::unmarshal(a,s));
    double *a1 = std::get<0>(st);
    for (std::size_t i = 0; i < 5; ++i)
        ASSERT_EQ(p1[i],a1[i]);
}

TEST(FnTypeMarshaller, p5T_rpT)
{
    double* p1 = new double[5]{5.0,6.0,7.0,8.0,9.0};
    using F = void(*)(double*&&);
    using StorageTupleType = typename ::mpirpc::internal::wrapped_function_parts<F>::storage_tuple_type;
    std::cout << abi::__cxa_demangle(typeid(StorageTupleType).name(),0,0,0) << std::endl;

    mpirpc::parameter_stream s;
    mpirpc::internal::fn_type_marshaller<F>::marshal(s, mpirpc::pointer_wrapper<double>(p1,5));
    std::allocator<void> a;
    StorageTupleType st(mpirpc::internal::tuple_unmarshaller_remote<double*&&>::unmarshal(a,s));
    double *a1 = std::get<0>(st);
    for (std::size_t i = 0; i < 5; ++i)
        ASSERT_EQ(p1[i],a1[i]);
}

TEST(FnTypeMarshaller, la5T_pT)
{
    using F = void(*)(double*);
    using StorageTupleType = typename ::mpirpc::internal::wrapped_function_parts<F>::storage_tuple_type;

    mpirpc::parameter_stream s;
    double p1[5]{5.0,6.0,7.0,8.0,9.0};
    mpirpc::internal::fn_type_marshaller<F>::marshal(s, p1);
    std::allocator<void> a;
    StorageTupleType st(mpirpc::internal::tuple_unmarshaller_remote<double*>::unmarshal(a,s));
    double *a1 = std::get<0>(st);
    for (std::size_t i = 0; i < 5; ++i)
        ASSERT_EQ(p1[i],a1[i]);
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
    double *a1 = std::get<0>(st);
    for (std::size_t i = 0; i < 5; ++i)
        ASSERT_EQ(p1[i],a1[i]);
}*/

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
