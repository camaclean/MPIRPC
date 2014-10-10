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
class ObjectWrapperBase {
    friend class Manager;
public:
    ObjectWrapperBase(void* object) : m_object(object), m_type(0) {
        m_id = ++objectIdCounter;
    }

    virtual ~ObjectWrapperBase() {}

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
class ObjectWrapper : public ObjectWrapperBase
{
    friend class Manager;
public:
    ObjectWrapper(T* object = 0) :  ObjectWrapperBase(object) {}

    T* object() const { return m_object; }

protected:
    T* m_object;

};


}

#endif // ACTORWRAPPER_H
