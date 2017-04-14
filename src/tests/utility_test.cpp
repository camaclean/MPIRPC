/*
 * MPIRPC: MPI based invocation of functions on other ranks
 * Copyright (C) 2014-2017 Colin MacLean <cmaclean@illinois.edu>
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

#include <gtest/gtest.h>
#include <iostream>
#include <string>
#include "../internal/utility.hpp"

decltype(auto) apply_test_fn(int a, double b, long c, const std::string& d)
{
    return std::tuple<int, double, long, std::string>{a+1, b+1, c+1, d+" test"};
}

/**
 * Tests mpirpc::internal::apply(F&& f, Tuple&& t)
 */
TEST(Utility, apply)
{
    std::tuple<int, double, long, std::string> args{1, 2.0, 3LL, "blah"};
    const auto& ret = mpirpc::internal::apply(&apply_test_fn, args);
    ASSERT_EQ(2, std::get<0>(ret));
    ASSERT_EQ(3.0,  std::get<1>(ret));
    ASSERT_EQ(4LL, std::get<2>(ret));
    ASSERT_EQ("blah test",  std::get<3>(ret));
}

class apply_test
{
public:
    apply_test(short m) : m_m{m} {};

    decltype(auto) apply_test_mem_fn(int a, double b, long c, const std::string& d)
    {
        return std::tuple<int, double, long, std::string, short>{a+1, b+1, c+1, d+" test", m_m+1};
    }

private:
    short m_m;
};

/**
 * Tests mpirpc::internal::apply(F&& f, Class *c, Tuple&& t)
 */
TEST(Utility, apply_mem_fn)
{
    std::tuple<int, double, long, std::string> args{1, 2.0, 3LL, "blah"};
    apply_test a(4);
    const auto& ret = mpirpc::internal::apply(&apply_test::apply_test_mem_fn, &a, args);
    ASSERT_EQ(2, std::get<0>(ret));
    ASSERT_EQ(3.0,  std::get<1>(ret));
    ASSERT_EQ(4LL, std::get<2>(ret));
    ASSERT_EQ("blah test",  std::get<3>(ret));
    ASSERT_EQ(5, std::get<4>(ret));
}

/**
 * Tests Int mpirpc::internal::get(std::integer_sequence\<Int,Is...\>)
 */
TEST(Utility, integer_sequence_get)
{
    {
        std::integer_sequence<int, 0, 1, 2, 3, 4, 5> is;
        ASSERT_EQ(0, mpirpc::internal::get<0>(is));
        ASSERT_EQ(3, mpirpc::internal::get<3>(is));
        ASSERT_EQ(5, mpirpc::internal::get<5>(is));
    }
    {
        std::integer_sequence<int, 0> is;
        ASSERT_EQ(0, mpirpc::internal::get<0>(is));
    }
}

/**
 * Tests mpirpc::internal::integer_sequence_element_v
 * Tests mpirpc::internal::extract_integer_pack_element_v
 */
TEST(Utility, integer_sequence_element_v)
{
    {
        using IS = std::integer_sequence<int, 0, 1, 2, 3, 4, 5>;
        ASSERT_EQ(0, (mpirpc::internal::integer_sequence_element_v<0, IS>));
        ASSERT_EQ(3, (mpirpc::internal::integer_sequence_element_v<3, IS>));
        ASSERT_EQ(5, (mpirpc::internal::integer_sequence_element_v<5, IS>));
    }
    {
        using IS = std::integer_sequence<std::size_t, 0>;
        ASSERT_EQ(0, (mpirpc::internal::integer_sequence_element_v<0, IS>));
    }
}

/**
 * Tests mpirpc::internal::integer_sequence_cat
 */
TEST(Utility, integer_sequence_cat)
{
    ASSERT_TRUE((std::is_same<std::index_sequence<0, 1, 2, 3, 4, 5>, mpirpc::internal::integer_sequence_cat_type<std::index_sequence<0, 1, 2>, std::index_sequence<3, 4, 5>>>::value));
    ASSERT_FALSE((std::is_same<std::index_sequence<0, 1, 2>, mpirpc::internal::integer_sequence_cat_type<std::index_sequence<0, 1, 2>, std::index_sequence<3, 4, 5>>>::value));
    ASSERT_TRUE((std::is_same<std::integer_sequence<bool, false, false, true, true, true>, mpirpc::internal::integer_sequence_cat_type<std::integer_sequence<bool, false, false, true>, std::integer_sequence<bool, true, true>>>::value));
    ASSERT_FALSE((std::is_same<std::integer_sequence<bool, false, false, true, true, false>, mpirpc::internal::integer_sequence_cat_type<std::integer_sequence<bool, false, false, true>, std::integer_sequence<bool, true, true>>>::value));
    ASSERT_TRUE((std::is_same<std::index_sequence<0, 1, 2, 3, 4, 5, 6, 7, 8>, mpirpc::internal::integer_sequence_cat_type<std::index_sequence<0, 1, 2>, std::index_sequence<3, 4, 5>, std::index_sequence<6, 7, 8>>>::value));
}

/**
 * Tests mpirpc::internal::integer_sequence_prepend
 */
TEST(Utility, integer_sequence_prepend)
{
    ASSERT_TRUE((std::is_same<std::index_sequence<0>, mpirpc::internal::integer_sequence_prepend_type<std::index_sequence<>, 0>>::value));
    ASSERT_TRUE((std::is_same<std::index_sequence<1, 0>, mpirpc::internal::integer_sequence_prepend_type<std::index_sequence<0>, 1>>::value));
    ASSERT_TRUE((std::is_same<std::integer_sequence<bool, true>, mpirpc::internal::integer_sequence_prepend_type<std::integer_sequence<bool>, true>>::value));
    ASSERT_TRUE((std::is_same<std::integer_sequence<bool, true, false>, mpirpc::internal::integer_sequence_prepend_type<std::integer_sequence<bool, false>, true>>::value));
}

/**
 * Tests mpirpc::internal::integer_sequence_prepend
 */
TEST(Utility, integer_sequence_append)
{
    ASSERT_TRUE((std::is_same<std::index_sequence<0>, mpirpc::internal::integer_sequence_append_type<std::index_sequence<>, 0>>::value));
    ASSERT_TRUE((std::is_same<std::index_sequence<0, 1>, mpirpc::internal::integer_sequence_append_type<std::index_sequence<0>, 1>>::value));
    ASSERT_TRUE((std::is_same<std::integer_sequence<bool, true>, mpirpc::internal::integer_sequence_append_type<std::integer_sequence<bool>, true>>::value));
    ASSERT_TRUE((std::is_same<std::integer_sequence<bool, false, true>, mpirpc::internal::integer_sequence_append_type<std::integer_sequence<bool, false>, true>>::value));
}

/**
 * Tests mpirpc::internal::conditional_integer_sequence_prepend
 */
TEST(Utility, conditional_integer_sequence_prepend)
{
    ASSERT_TRUE((std::is_same<std::index_sequence<0>, mpirpc::internal::conditional_integer_sequence_prepend_type<true, std::index_sequence<>, 0>>::value));
    ASSERT_TRUE((std::is_same<std::index_sequence<>, mpirpc::internal::conditional_integer_sequence_prepend_type<false, std::index_sequence<>, 0>>::value));
    ASSERT_TRUE((std::is_same<std::index_sequence<1, 0>, mpirpc::internal::conditional_integer_sequence_prepend_type<true, std::index_sequence<0>, 1>>::value));
    ASSERT_TRUE((std::is_same<std::index_sequence<0>, mpirpc::internal::conditional_integer_sequence_prepend_type<false, std::index_sequence<0>, 1>>::value));
    ASSERT_TRUE((std::is_same<std::integer_sequence<bool, true>, mpirpc::internal::conditional_integer_sequence_prepend_type<true, std::integer_sequence<bool>, true>>::value));
    ASSERT_TRUE((std::is_same<std::integer_sequence<bool>, mpirpc::internal::conditional_integer_sequence_prepend_type<false, std::integer_sequence<bool>, true>>::value));
    ASSERT_TRUE((std::is_same<std::integer_sequence<bool, true, false>, mpirpc::internal::conditional_integer_sequence_prepend_type<true, std::integer_sequence<bool, false>, true>>::value));
    ASSERT_TRUE((std::is_same<std::integer_sequence<bool, false>, mpirpc::internal::conditional_integer_sequence_prepend_type<false, std::integer_sequence<bool, false>, true>>::value));
}

/**
 * Tests mpirpc::internal::conditional_integer_sequence_prepend
 */
TEST(Utility, conditional_integer_sequence_append)
{
    ASSERT_TRUE((std::is_same<std::index_sequence<0>, mpirpc::internal::conditional_integer_sequence_append_type<true, std::index_sequence<>, 0>>::value));
    ASSERT_TRUE((std::is_same<std::index_sequence<>, mpirpc::internal::conditional_integer_sequence_append_type<false, std::index_sequence<>, 0>>::value));
    ASSERT_TRUE((std::is_same<std::index_sequence<0, 1>, mpirpc::internal::conditional_integer_sequence_append_type<true, std::index_sequence<0>, 1>>::value));
    ASSERT_TRUE((std::is_same<std::index_sequence<0>, mpirpc::internal::conditional_integer_sequence_append_type<false, std::index_sequence<0>, 1>>::value));
    ASSERT_TRUE((std::is_same<std::integer_sequence<bool, true>, mpirpc::internal::conditional_integer_sequence_append_type<true, std::integer_sequence<bool>, true>>::value));
    ASSERT_TRUE((std::is_same<std::integer_sequence<bool>, mpirpc::internal::conditional_integer_sequence_append_type<false, std::integer_sequence<bool>, true>>::value));
    ASSERT_TRUE((std::is_same<std::integer_sequence<bool, false, true>, mpirpc::internal::conditional_integer_sequence_append_type<true, std::integer_sequence<bool, false>, true>>::value));
    ASSERT_TRUE((std::is_same<std::integer_sequence<bool, false>, mpirpc::internal::conditional_integer_sequence_append_type<false, std::integer_sequence<bool, false>, true>>::value));
}

/**
 * Tests mpirpc::internal::tuple_type_cat
 */
TEST(Utility, tuple_type_cat)
{
    ASSERT_TRUE((std::is_same<std::tuple<int, double>, mpirpc::internal::tuple_type_cat_type<std::tuple<int>, std::tuple<double>>>::value));
    ASSERT_TRUE((std::is_same<std::tuple<int, short, double, float>, mpirpc::internal::tuple_type_cat_type<std::tuple<int, short>, std::tuple<double, float>>>::value));
    ASSERT_TRUE((std::is_same<std::tuple<int, short, double, float, long, long long>, mpirpc::internal::tuple_type_cat_type<std::tuple<int, short>, std::tuple<double, float>, std::tuple<long, long long>>>::value));
}

/**
 * Tests mpirpc::internal::tuple_type_prepend
 */
TEST(Utility, tuple_type_prepend)
{
    ASSERT_TRUE((std::is_same<std::tuple<int>, mpirpc::internal::tuple_type_prepend_type<int, std::tuple<>>>::value));
    ASSERT_TRUE((std::is_same<std::tuple<int, double>, mpirpc::internal::tuple_type_prepend_type<int, std::tuple<double>>>::value));
}

/**
 * Tests mpirpc::internal::tuple_type_append
 */
TEST(Utility, tuple_type_append)
{
    ASSERT_TRUE((std::is_same<std::tuple<int>, mpirpc::internal::tuple_type_append_type<std::tuple<>, int>>::value));
    ASSERT_TRUE((std::is_same<std::tuple<double, int>, mpirpc::internal::tuple_type_append_type<std::tuple<double>, int>>::value));
}

/**
 * Tests mpirpc::internal::conditional_tuple_type_prepend
 */
TEST(Utility, conditional_tuple_type_prepend)
{
    ASSERT_TRUE((std::is_same<std::tuple<int>, mpirpc::internal::conditional_tuple_type_prepend_type<true, int, std::tuple<>>>::value));
    ASSERT_TRUE((std::is_same<std::tuple<>, mpirpc::internal::conditional_tuple_type_prepend_type<false, int, std::tuple<>>>::value));
    ASSERT_TRUE((std::is_same<std::tuple<int, double>, mpirpc::internal::conditional_tuple_type_prepend_type<true, int, std::tuple<double>>>::value));
    ASSERT_TRUE((std::is_same<std::tuple<double>, mpirpc::internal::conditional_tuple_type_prepend_type<false, int, std::tuple<double>>>::value));
}

/**
 * Tests mpirpc::internal::conditional_tuple_type_append
 */
TEST(Utility, conditional_tuple_type_append)
{
    ASSERT_TRUE((std::is_same<std::tuple<int>, mpirpc::internal::conditional_tuple_type_append_type<true, std::tuple<>, int>>::value));
    ASSERT_TRUE((std::is_same<std::tuple<>, mpirpc::internal::conditional_tuple_type_append_type<false, std::tuple<>, int>>::value));
    ASSERT_TRUE((std::is_same<std::tuple<double, int>, mpirpc::internal::conditional_tuple_type_append_type<true, std::tuple<double>, int>>::value));
    ASSERT_TRUE((std::is_same<std::tuple<double>, mpirpc::internal::conditional_tuple_type_append_type<false, std::tuple<double>, int>>::value));
}

/**
 * Test mpirpc::internal::filter_tuple_types
 */
TEST(Utility, filter_tuple_types)
{
    ASSERT_TRUE((std::is_same<std::tuple<>, mpirpc::internal::filter_tuple<std::tuple<>, std::tuple<>>>::value));
    ASSERT_TRUE((
        std::is_same<
            std::tuple<int, const int, double, const double, const long, long, short>,
            mpirpc::internal::filter_tuple<
                std::tuple<std::true_type, std::true_type, std::true_type, std::true_type, std::true_type, std::true_type, std::true_type>,
                std::tuple<int, const int, double, const double, const long, long, short>
              >
          >::value
      ));
    ASSERT_TRUE((
        std::is_same<
            std::tuple<>,
            mpirpc::internal::filter_tuple<
                std::tuple<std::false_type, std::false_type, std::false_type, std::false_type, std::false_type, std::false_type, std::false_type>,
                std::tuple<int, const int, double, const double, const long, long, short>
              >
          >::value
      ));
    ASSERT_TRUE((
        std::is_same<
            std::tuple<const int, const double, const long>,
            mpirpc::internal::filter_tuple<
                std::integer_sequence<bool, false, true, false, true, true, false, false>,
                std::tuple<int, const int, double, const double, const long, long, short>
              >
          >::value));
}

/**
 * Test mpirpc::internal::count_trues
 */
TEST(Utility, count_trues)
{
    ASSERT_EQ(0, (mpirpc::internal::count_trues_v<>));
    ASSERT_EQ(5, (mpirpc::internal::count_trues_v<true, true, true, true, true>));
    ASSERT_EQ(5, (mpirpc::internal::count_trues_v<false, false, true, true, true, false, true, true, false>));
    ASSERT_EQ(0, (mpirpc::internal::count_trues_v<false, false, false, false>));
}

/**
 * Test mpirpc::internal::count_true_types
 */
TEST(Utility, count_true_types)
{
    ASSERT_EQ(5, (mpirpc::internal::count_true_types_v<std::tuple<std::true_type, std::true_type, std::true_type, std::true_type, std::true_type>>));
    ASSERT_EQ(5, (mpirpc::internal::count_true_types_v<std::tuple<std::false_type, std::false_type, std::true_type, std::true_type, std::true_type, std::false_type, std::true_type, std::true_type, std::false_type>>));
    ASSERT_EQ(0, (mpirpc::internal::count_true_types_v<std::tuple<std::false_type, std::false_type, std::false_type, std::false_type>>));
}

/**
 * Test mpirpc::internal::filtered_indexes
 */
TEST(Utility, filtered_indexes)
{
    ASSERT_TRUE((std::is_same<std::tuple<>, mpirpc::internal::filtered_indexes_type<>>::value));

    ASSERT_TRUE((
        std::is_same<
            std::tuple<
                std::integral_constant<std::size_t, 0>
              >,
            mpirpc::internal::filtered_indexes_type<true>
          >::value
      ));

    ASSERT_TRUE((
        std::is_same<
            std::tuple<
                mpirpc::internal::invalid_index_type
              >,
            mpirpc::internal::filtered_indexes_type<false>
          >::value
      ));

    ASSERT_TRUE((
        std::is_same<
            std::tuple<
                mpirpc::internal::invalid_index_type,
                mpirpc::internal::invalid_index_type,
                mpirpc::internal::invalid_index_type,
                mpirpc::internal::invalid_index_type,
                mpirpc::internal::invalid_index_type,
                mpirpc::internal::invalid_index_type,
                mpirpc::internal::invalid_index_type
              >,
            mpirpc::internal::filtered_indexes_type<false, false, false, false, false, false, false>
          >::value
      ));

    ASSERT_TRUE((
        std::is_same<
            std::tuple<
                std::integral_constant<std::size_t, 0>,
                std::integral_constant<std::size_t, 1>,
                std::integral_constant<std::size_t, 2>,
                std::integral_constant<std::size_t, 3>,
                std::integral_constant<std::size_t, 4>,
                std::integral_constant<std::size_t, 5>,
                std::integral_constant<std::size_t, 6>
              >,
            mpirpc::internal::filtered_indexes_type<true, true, true, true, true, true, true>
          >::value
      ));

    ASSERT_TRUE((
        std::is_same<
            std::tuple<
                mpirpc::internal::invalid_index_type,
                std::integral_constant<std::size_t, 0>,
                mpirpc::internal::invalid_index_type,
                mpirpc::internal::invalid_index_type,
                std::integral_constant<std::size_t, 1>,
                std::integral_constant<std::size_t, 2>,
                mpirpc::internal::invalid_index_type
              >,
            mpirpc::internal::filtered_indexes_type<false, true, false, false, true, true, false>
          >::value
      ));

    ASSERT_TRUE((
        std::is_same<
            std::tuple<
                std::integral_constant<std::size_t, 0>,
                mpirpc::internal::invalid_index_type,
                std::integral_constant<std::size_t, 1>,
                std::integral_constant<std::size_t, 2>,
                mpirpc::internal::invalid_index_type,
                mpirpc::internal::invalid_index_type,
                std::integral_constant<std::size_t, 3>
              >,
            mpirpc::internal::filtered_indexes_type<true, false, true, true, false, false, true>
          >::value
      ));
}

TEST(Utility, unfiltered_indexes)
{
    ASSERT_TRUE((std::is_same<std::tuple<>, mpirpc::internal::unfiltered_indexes_type<>>::value));

    ASSERT_TRUE((
        std::is_same<
            std::tuple<
                std::integral_constant<std::size_t, 0>
              >,
            mpirpc::internal::unfiltered_indexes_type<true>
          >::value
      ));

    ASSERT_TRUE((
        std::is_same<
            std::tuple<>,
            mpirpc::internal::unfiltered_indexes_type<false>
          >::value
      ));

    ASSERT_TRUE((
        std::is_same<
            std::tuple<>,
            mpirpc::internal::unfiltered_indexes_type<false, false, false, false, false, false, false>
          >::value
      ));

    ASSERT_TRUE((
        std::is_same<
            std::tuple<
                std::integral_constant<std::size_t, 0>,
                std::integral_constant<std::size_t, 1>,
                std::integral_constant<std::size_t, 2>,
                std::integral_constant<std::size_t, 3>,
                std::integral_constant<std::size_t, 4>,
                std::integral_constant<std::size_t, 5>,
                std::integral_constant<std::size_t, 6>
              >,
            mpirpc::internal::unfiltered_indexes_type<true, true, true, true, true, true, true>
          >::value
      ));

    ASSERT_TRUE((
        std::is_same<
            std::tuple<
                // false 0
                std::integral_constant<std::size_t, 1>,
                // false 2
                // false 3
                std::integral_constant<std::size_t, 4>,
                std::integral_constant<std::size_t, 5>
                // false 6
              >,
            mpirpc::internal::unfiltered_indexes_type<false, true, false, false, true, true, false>
          >::value
      ));

    ASSERT_TRUE((
        std::is_same<
            std::tuple<
                std::integral_constant<std::size_t, 0>,
                // false 1
                std::integral_constant<std::size_t, 2>,
                std::integral_constant<std::size_t, 3>,
                // false 4
                // false 5
                std::integral_constant<std::size_t, 6>
              >,
            mpirpc::internal::unfiltered_indexes_type<true, false, true, true, false, false, true>
          >::value
      ));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
