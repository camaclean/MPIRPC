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

#include "parameterstream.hpp"
#include <cstring>

namespace mpirpc {

parameter_stream::parameter_stream(std::vector<char>* buffer)
    : m_data(buffer), m_pos(0)
{
}

void parameter_stream::seek(std::size_t pos)
{
    m_pos = pos;
}

char* parameter_stream::data()
{
    return m_data->data();
}

const char* parameter_stream::constData() const
{
    return m_data->data();
}

std::vector<char>* parameter_stream::dataVector() const
{
    return m_data;
}

size_t parameter_stream::size() const
{
    return m_data->size();
}

parameter_stream& parameter_stream::operator<<(int8_t val)
{
    const char* p = reinterpret_cast<const char*>(&val);
    m_data->insert(m_data->end(), p, p+sizeof(val));
    return *this;
}

parameter_stream& parameter_stream::operator<<(int16_t val)
{
    const char* p = reinterpret_cast<const char*>(&val);
    m_data->insert(m_data->end(), p, p+sizeof(val));
    return *this;
}

parameter_stream& parameter_stream::operator<<(int32_t val)
{
    const char* p = reinterpret_cast<const char*>(&val);
    m_data->insert(m_data->end(), p, p+sizeof(val));
    return *this;
}

parameter_stream& parameter_stream::operator<<(int64_t val)
{
    const char* p = reinterpret_cast<const char*>(&val);
    m_data->insert(m_data->end(), p, p+sizeof(val));
    return *this;
}

parameter_stream& parameter_stream::operator<<(uint8_t val)
{
    const char* p = reinterpret_cast<const char*>(&val);
    m_data->insert(m_data->end(), p, p+sizeof(val));
    return *this;
}

parameter_stream& parameter_stream::operator<<(uint16_t val)
{
    const char* p = reinterpret_cast<const char*>(&val);
    m_data->insert(m_data->end(), p, p+sizeof(val));
    return *this;
}

parameter_stream& parameter_stream::operator<<(uint32_t val)
{
    const char* p = reinterpret_cast<const char*>(&val);
    m_data->insert(m_data->end(), p, p+sizeof(val));
    return *this;
}

parameter_stream& parameter_stream::operator<<(uint64_t val)
{
    const char* p = reinterpret_cast<const char*>(&val);
    m_data->insert(m_data->end(), p, p+sizeof(val));
    return *this;
}

parameter_stream& parameter_stream::operator<<(long long unsigned int val)
{
    const char* p = reinterpret_cast<const char*>(&val);
    m_data->insert(m_data->end(), p, p+sizeof(val));
    return *this;
}

parameter_stream& parameter_stream::operator<<(long long int val)
{
    const char* p = reinterpret_cast<const char*>(&val);
    m_data->insert(m_data->end(), p, p+sizeof(val));
    return *this;
}

parameter_stream& parameter_stream::operator>>(int8_t& val)
{
    val = *reinterpret_cast<int8_t*>(&(*m_data)[m_pos]);
    m_pos += sizeof(int8_t);
    return *this;
}

parameter_stream& parameter_stream::operator>>(int16_t& val)
{
    val = *reinterpret_cast<int16_t*>(&(*m_data)[m_pos]);
    m_pos += sizeof(int16_t);
    return *this;
}

parameter_stream& parameter_stream::operator>>(int32_t& val)
{
    val = *reinterpret_cast<int32_t*>(&(*m_data)[m_pos]);
    m_pos += sizeof(int32_t);
    return *this;
}

parameter_stream& parameter_stream::operator>>(int64_t& val)
{
    val = *reinterpret_cast<int64_t*>(&(*m_data)[m_pos]);
    m_pos += sizeof(int64_t);
    return *this;
}

parameter_stream& parameter_stream::operator>>(uint8_t& val)
{
    val = *reinterpret_cast<int8_t*>(&(*m_data)[m_pos]);
    m_pos += sizeof(uint8_t);
    return *this;
}

parameter_stream& parameter_stream::operator>>(uint16_t& val)
{
    val = *reinterpret_cast<uint16_t*>(&(*m_data)[m_pos]);
    m_pos += sizeof(uint16_t);
    return *this;
}

parameter_stream& parameter_stream::operator>>(uint32_t& val)
{
    val = *reinterpret_cast<uint32_t*>(&(*m_data)[m_pos]);
    m_pos += sizeof(uint32_t);
    return *this;
}

parameter_stream& parameter_stream::operator>>(uint64_t& val)
{
    val = *reinterpret_cast<uint64_t*>(&(*m_data)[m_pos]);
    m_pos += sizeof(uint64_t);
    return *this;
}

parameter_stream& parameter_stream::operator>>(long long unsigned int& val)
{
    val = *reinterpret_cast<long long unsigned int*>(&(*m_data)[m_pos]);
    m_pos += sizeof(long long unsigned int);
    return *this;
}

parameter_stream& parameter_stream::operator>>(long long int& val)
{
    val = *reinterpret_cast<long long int*>(&(*m_data)[m_pos]);
    m_pos += sizeof(long long int);
    return *this;
}

parameter_stream& parameter_stream::operator<<(float val)
{
    const char* p = reinterpret_cast<const char*>(&val);
    m_data->insert(m_data->end(), p, p+sizeof(val));
    return *this;
}

parameter_stream& parameter_stream::operator<<(double val)
{
    const char* p = reinterpret_cast<const char*>(&val);
    m_data->insert(m_data->end(), p, p+sizeof(val));
    return *this;
}

parameter_stream& parameter_stream::operator>>(float& val)
{
    val = *reinterpret_cast<float*>(&(*m_data)[m_pos]);
    m_pos += sizeof(float);
    return *this;
}

parameter_stream& parameter_stream::operator>>(double& val)
{
    val = *reinterpret_cast<double*>(&(*m_data)[m_pos]);
    m_pos += sizeof(double);
    return *this;
}

parameter_stream& parameter_stream::operator<<(bool val)
{
    const char* p = reinterpret_cast<const char*>(&val);
    m_data->insert(m_data->end(), p, p+sizeof(val));
    return *this;
}

parameter_stream& parameter_stream::operator>>(bool& val)
{
    val = *reinterpret_cast<bool*>(&(*m_data)[m_pos]);
    m_pos += sizeof(bool);
    return *this;
}

parameter_stream& parameter_stream::operator<<(const char* s)
{
    size_t length = strlen(s)+1;
    m_data->insert(m_data->end(), s, s+length);
    return *this;
}

parameter_stream& parameter_stream::operator>>(char *& s)
{
    size_t length = strlen(&(*m_data)[m_pos])+1;
    s = new char[length];
    std::copy(&(*m_data)[m_pos], &(*m_data)[m_pos+length], s);
    m_pos += length;
    return *this;
}

parameter_stream& parameter_stream::operator<<(const std::string& val)
{
    uint64_t length = val.size();
    *this << length;
    m_data->insert(m_data->end(), val.c_str(), val.c_str()+length);
    return *this;
}

parameter_stream& parameter_stream::operator>>(std::string& val)
{
    uint64_t length;
    *this >> length;
    val.assign(&(*m_data)[m_pos], length);
    m_pos += length;
    return *this;
}

void parameter_stream::writeBytes(const char* b, size_t length)
{
    m_data->insert(m_data->end(), b, b+length);
}

void parameter_stream::readBytes(char *& b, size_t length)
{
    std::copy(&(*m_data)[m_pos], &(*m_data)[m_pos+length], b);
    m_pos += length;
}

}

