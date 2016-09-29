/*
 * MPIRPC: MPI based invocation of functions on other ranks
 * Copyright (C) 2014  Colin MacLean <s0838159@sms.ed.ac.uk>
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

#ifndef ACTORWRAPPER_H
#define ACTORWRAPPER_H

#include <cinttypes>
#include "common.hpp"

namespace mpirpc {

/**
 * @brief The ObjectWrapperBase class
 *
 * Used to reference objects on remote ranks
 */

template<typename MessageInterface, template<typename> typename Allocator>
class manager;

class object_wrapper_base {
    template<typename MessageInterface, template<typename> typename Allocator>
    friend class manager;
public:
    object_wrapper_base(void* object = nullptr) : m_object(object), m_type(0) {
        m_id = ++objectIdCounter;
    }

    virtual ~object_wrapper_base() {}

    void* object() const { return m_object; }

    ObjectId id() const { return m_id; }
    TypeId type() const { return m_type; }
    int rank() const { return m_rank; }
protected:
    int m_rank;
    ObjectId m_id;
    void* m_object;
    TypeId m_type;
    static ObjectId objectIdCounter;
};

/**
 * @brief The ObjectWrapper<T> class
 *
 * Used to reference objects on the local rank.
 */
template<typename T>
class object_wrapper : public object_wrapper_base
{
    template<typename MessageInterface, template<typename> typename Allocator>
    friend class manager;
public:
    object_wrapper(T* object = nullptr) :  object_wrapper_base(object) {}

    T* object() const { return m_object; }

protected:
    T* m_object;

};


}

#endif // ACTORWRAPPER_H
