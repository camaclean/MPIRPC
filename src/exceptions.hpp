#ifndef MPIRPC_EXCEPTIONS_H
#define MPIRPC_EXCEPTIONS_H

namespace mpirpc
{

struct unregistered_function_exception : std::exception
{
    const char* what() const noexcept override
    {
        return "Unregistered Function\n";
    }
};

struct unregistered_object_exception : std::exception
{
    const char* what() const noexcept override
    {
        return "Unregistered Object\n";
    }
};

}

#endif /* MPIRPC_EXCEPTIONS_H */
