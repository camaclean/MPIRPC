#ifndef MPIRPC_EXCEPTIONS_H
#define MPIRPC_EXCEPTIONS_H

namespace mpirpc
{

struct UnregisteredFunctionException : std::exception
{
    const char* what() const noexcept override
    {
        return "Unregistered Function\n";
    }
};

struct UnregisteredObjectException : std::exception
{
    const char* what() const noexcept override
    {
        return "Unregistered Object\n";
    }
};

}

#endif /* MPIRPC_EXCEPTIONS_H */
