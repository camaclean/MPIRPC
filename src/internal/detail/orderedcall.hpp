/*
 * MPIRPC: MPI based invocation of functions on other ranks
 * Copyright (C) 2014-2016 Colin MacLean <cmaclean@illinois.edu>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef MPIRPC__INTERNAL__DETAIL__ORDEREDCALL_HPP
#define MPIRPC__INTERNAL__DETAIL__ORDEREDCALL_HPP

namespace mpirpc
{

namespace internal
{

namespace detail
{

template<typename T>
struct arg_cleanup
{
    static void apply(typename std::remove_reference<T>::type& t) { std::cout << "blank clean up for " << typeid(t).name() << std::endl; }
    static void apply(typename std::remove_reference<T>::type&& t) { std::cout << "generic rvalue " << typeid(t).name() << std::endl; }
};

template<typename T, std::size_t N, bool PassOwnership, bool PassBack, typename Allocator>
struct arg_cleanup<pointer_wrapper<T,N,PassOwnership,PassBack,Allocator>>
{
    static void apply(pointer_wrapper<T,N,PassOwnership,PassBack,Allocator>&& t) { std::cout << "cleaned up C Array of " << typeid(T).name() << std::endl; t.free(); }
};

template<typename T, std::size_t N>
struct arg_cleanup<T(&)[N]>
{
    static void apply(T(&v)[N])
    {
        std::cout << "cleaning up reference to array" << std::endl;
        std::allocator<std::remove_cv_t<T>> a;
        a.deallocate((std::remove_cv_t<T>*)&v,N);
    }
};

template<>
struct arg_cleanup<char*>
{
    static void apply(char*&& s) { delete[] s; std::cout << "deleted char*" << std::endl; }
};

template<>
struct arg_cleanup<const char*>
{
    static void apply(const char*&& s) { delete[] s; std::cout << "deleted const char*" << std::endl; }
};

template<typename... Args>
void do_post_exec(Args&&... args)
{
    [](...){}( (arg_cleanup<Args>::apply(std::forward<Args>(args)), 0)...);
}

}

}

}

#endif /* MPIRPC__INTERNAL__DETAIL__ORDEREDCALL_HPP */
