#ifndef COMMON_HPP
#define COMMON_HPP

//#include <QtGlobal>

#ifndef NDEBUG
#   define ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            std::cerr << "Assertion `" #condition "` failed in " << __FILE__  << " line " << __LINE__ << ": " << message << std::endl; \
            std::exit(ERR_ASSERT); \
        } \
    } while (false)
#else /* NDEBUG */
#define ASSERT(condition, message) do {} while(false)
#endif

namespace mpirpc
{

/**
 * Passer can be used along with uniform initialization to unpack parameter packs
 * and execute the parameters in the order in which they appear. This is necessary
 * for correctness when side effects are important.
 */
struct Passer {
    Passer(...) {}
};

template<typename F>
struct FunctionParts;

template<typename R, class Class, typename... Args>
struct FunctionParts<R(Class::*)(Args...)>
{
    using return_type = R;
    using class_type = Class;
    using function_type = R(Class::*)(Args...);
};

template<typename R, typename... Args>
struct FunctionParts<R(*)(Args...)>
{
    using return_type = R;
    using function_type = R(*)(Args...);
};

using FunctionHandle = unsigned long;
using TypeId = unsigned long;
using ObjectId = unsigned long;

}

#endif // COMMON_HPP
