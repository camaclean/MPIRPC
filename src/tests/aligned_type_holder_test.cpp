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
#include <cxxabi.h>
#include "../internal/reconstruction/aligned_type_holder.hpp"
#include "../parameter_buffer.hpp"

class C
{
public:
    C() : m_a{}, m_b{} {}
    C (int a, double b) : m_a{a}, m_b{b} {}

    int a() const { return m_a; }
    double b() const { return m_b; }

private:
    int m_a;
    double m_b;
};

namespace mpirpc
{

template<typename Buffer, typename Alignment>
struct unmarshaller<C,Buffer,Alignment>
{
    using type = mpirpc::construction_info<C,
        std::tuple<int, double>,
        std::tuple<int, double>,
        std::tuple<std::false_type, std::false_type>
    >;
    template<typename Allocator>
    static type unmarshal(Allocator&& a, Buffer& b)
    {
        int avar = mpirpc::get<int>(b,a);
        double bvar = mpirpc::get<double>(b,a);
        std::cout << "unmarshalling: C(" << avar << "," <<  bvar << ")" << std::endl;
        return type{avar, bvar};
    }
};

template<typename Buffer, typename Alignment>
struct marshaller<C,Buffer,Alignment>
{
    static void marshal(Buffer& buf, const C& v)
    {
        buf.template push<int>(v.a());
        buf.template push<double>(v.b());
        std::cout << "marshalling: C(" << v.a() << "," <<  v.b() << ")" << std::endl;
    }
};

}

TEST(aligned_type_holder, basic_from_basic)
{
    std::tuple<double> test_args(5.4);
    mpirpc::internal::reconstruction::aligned_type_holder<
        double
      , std::integral_constant<std::size_t, alignof(double)>
      , std::tuple<double>
      , std::tuple<double>
      , std::tuple<std::false_type>
      , std::tuple<std::integral_constant<std::size_t, alignof(double)>>
      > test(test_args);
    ASSERT_EQ(5.4, test.value());
}

TEST(aligned_type_holder, const_basic_from_basic)
{
    std::tuple<double> test_args(5.4);
    mpirpc::internal::reconstruction::aligned_type_holder<
        double
      , std::integral_constant<std::size_t, alignof(double)>
      , std::tuple<double>
      , std::tuple<double>
      , std::tuple<std::false_type>
      , std::tuple<std::integral_constant<std::size_t, alignof(double)>>
      > test(test_args);
    ASSERT_EQ(5.4, test.value());
}

TEST(aligned_type_holder, basic_from_const_basic)
{

    std::tuple<const double> test_args(5.4);
    mpirpc::internal::reconstruction::aligned_type_holder<
        double
      , std::integral_constant<std::size_t, alignof(double)>
      , std::tuple<const double>
      , std::tuple<const double>
      , std::tuple<std::false_type>
      , std::tuple<std::integral_constant<std::size_t, alignof(double)>>
      > test(test_args);
    ASSERT_EQ(5.4, test.value());

}

TEST(aligned_type_holder, const_basic_from_const_basic)
{
    std::tuple<const double> test_args(5.4);
    mpirpc::internal::reconstruction::aligned_type_holder<
        const double
      , std::integral_constant<std::size_t, alignof(const double)>
      , std::tuple<const double>
      , std::tuple<const double>
      , std::tuple<std::false_type>
      , std::tuple<std::integral_constant<std::size_t, alignof(const double)>>
      > test(test_args);
    ASSERT_EQ(5.4, test.value());
}

TEST(aligned_type_holder, basic_ref_from_basic)
{
    std::tuple<double> test_args(5.4);
    mpirpc::internal::reconstruction::aligned_type_holder<
        double&
      , std::integral_constant<std::size_t, alignof(double)>
      , std::tuple<double>
      , std::tuple<double>
      , std::tuple<std::true_type>
      , std::tuple<std::integral_constant<std::size_t, alignof(double)>>
      > test(test_args);
    ASSERT_EQ(5.4, test.value());
    ASSERT_TRUE((std::is_same<double&, decltype(test)::type>::value));
}

TEST(aligned_type_holder, misc)
{
    std::cout << abi::__cxa_demangle(typeid(typename
        mpirpc::internal::reconstruction::aligned_type_holder<
            double[5]
          , std::integral_constant<std::size_t, alignof(double[5])>
          , std::tuple<double(&)[5]>
          , std::tuple<double(&)[5]>
          , std::tuple<std::true_type>
          , std::tuple<std::integral_constant<std::size_t, alignof(double[5])>>
      >::base_type_holder).name(), 0, 0, 0)
      <<  std::endl;
    std::cout << abi::__cxa_demangle(typeid(typename
        mpirpc::internal::reconstruction::aligned_type_holder<
            double[5]
          , std::integral_constant<std::size_t, alignof(double[5])>
          , std::tuple<double(&)[5]>
          , std::tuple<double(&)[5]>
          , std::tuple<std::true_type>
          , std::tuple<std::integral_constant<std::size_t, alignof(double[5])>>
      >::aligned_storage_tuple_type).name(), 0, 0, 0)
      <<  std::endl;
    std::cout << abi::__cxa_demangle(typeid(typename
        mpirpc::internal::reconstruction::aligned_type_holder<
            double[5]
          , std::integral_constant<std::size_t, alignof(double[5])>
          , std::tuple<double(&)[5]>
          , std::tuple<double(&)[5]>
          , std::tuple<std::true_type>
          , std::tuple<std::integral_constant<std::size_t, alignof(double[5])>>
      >::type_storage).name(), 0, 0, 0)
      <<  std::endl;
    std::cout << abi::__cxa_demangle(typeid(typename
        mpirpc::internal::reconstruction::aligned_type_holder<
            double[5][7]
          , std::integral_constant<std::size_t, alignof(double[5][7])>
          , std::tuple<double(&)[5][7]>
          , std::tuple<double(&)[5][7]>
          , std::tuple<std::true_type>
          , std::tuple<std::integral_constant<std::size_t, alignof(double[5][7])>>
      >::base_type_holder).name(), 0, 0, 0)
      <<  std::endl;
    std::cout << abi::__cxa_demangle(typeid(typename
        mpirpc::internal::reconstruction::aligned_type_holder<
            double[5][7]
          , std::integral_constant<std::size_t, alignof(double[5][7])>
          , std::tuple<double(&)[5][7]>
          , std::tuple<double(&)[5][7]>
          , std::tuple<std::true_type>
          , std::tuple<std::integral_constant<std::size_t, alignof(double[5][7])>>
      >::aligned_storage_tuple_type).name(), 0, 0, 0)
      <<  std::endl;
    std::cout << abi::__cxa_demangle(typeid(typename
        mpirpc::internal::reconstruction::aligned_type_holder<
            double[5][7]
          , std::integral_constant<std::size_t, alignof(double[5][7])>
          , std::tuple<double(&)[5][7]>
          , std::tuple<double(&)[5][7]>
          , std::tuple<std::true_type>
          , std::tuple<std::integral_constant<std::size_t, alignof(double[5][7])>>
      >::type_storage).name(), 0, 0, 0)
      <<  std::endl;

    std::cout << abi::__cxa_demangle(typeid(std::tuple<mpirpc::unmarshaller_type<double[5][7], mpirpc::parameter_buffer<>, std::allocator<char>>>).name(), 0, 0, 0) <<  std::endl;
    std::cout << "-----------------" << std::endl;
    std::cout << abi::__cxa_demangle(typeid(std::tuple<mpirpc::unmarshaller_type<C[5][7], mpirpc::parameter_buffer<>, std::allocator<char>>>).name(), 0, 0, 0) <<  std::endl;
    std::cout << "-----------------" << std::endl;
    std::cout << abi::__cxa_demangle(typeid(std::remove_all_extents_t<double[5][7]>).name(), 0, 0, 0) <<  std::endl;
    std::cout << abi::__cxa_demangle(typeid(mpirpc::retype_array_type<C[5][7], mpirpc::unmarshaller_type<C, mpirpc::parameter_buffer<>, std::allocator<char>>>).name(), 0, 0, 0) <<  std::endl;
    std::cout <<  mpirpc::array_total_elements_v<C[5][7]> <<  std::endl;
    mpirpc::parameter_buffer<> buff;
    C (*blah)[3] = new C[2][3];
    for (std::size_t i = 0; i < 2; ++i)
    {
        for (std::size_t j = 0; j < 3; ++j)
        {
            blah[i][j] = C(i, j);
            mpirpc::marshaller<C, mpirpc::parameter_buffer<>, std::integral_constant<std::size_t, alignof(C)>>::marshal(buff, blah[i][j]);
        }
    }
    buff.seek(0);
    auto ret = mpirpc::unmarshaller<C[2][3], mpirpc::parameter_buffer<>, std::integral_constant<std::size_t, alignof(C)>>::unmarshal(std::allocator<char>(), buff);
    for (std::size_t i = 0; i < 2; ++i)
    {
        for (std::size_t j = 0; j < 3; ++j)
        {
            std::cout << std::get<0>(ret[i][j].args()) << " " << std::get<1>(ret[i][j].args()) << std::endl;
        }
    }
     std::cout << abi::__cxa_demangle(typeid(ret).name(), 0, 0, 0) <<  std::endl;
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
