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

#ifndef MPIRPCTEST_H
#define MPIRPCTEST_H

#include <QtCore/QObject>
#include <QtTest/QTest>
#include <string>

class MpirpcTest : public QObject
{
Q_OBJECT
private slots:
    void stream_uint8_t_test();
    void stream_uint8_t_test_data();

    void stream_uint16_t_test();
    void stream_uint16_t_test_data();

    void stream_uint32_t_test();
    void stream_uint32_t_test_data();

    void stream_uint64_t_test();
    void stream_uint64_t_test_data();

    void stream_int8_t_test();
    void stream_int8_t_test_data();

    void stream_int16_t_test();
    void stream_int16_t_test_data();

    void stream_int32_t_test();
    void stream_int32_t_test_data();

    void stream_int64_t_test();
    void stream_int64_t_test_data();

    void stream_double_test();
    void stream_double_test_data();

    void stream_string_test();
    void stream_string_test_data();

    void stream_charp_test();
    void stream_charp_test_data();

    void stream_combo();
};

Q_DECLARE_METATYPE(std::string)

#endif /* MPIRPCTEST_H */

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle;
