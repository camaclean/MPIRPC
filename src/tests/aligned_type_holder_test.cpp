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
#include <memory>
#include <cxxabi.h>
#include "../internal/reconstruction/aligned_type_holder.hpp"
#include "../parameter_buffer.hpp"

double b_global = 5.4;

class C
{
public:
    C () : m_a{}, m_b{b_global} {}
    C (int a, double& b) : m_a{a}, m_b{b} { b += 3.4;}

    C& operator = (C&& c) { m_a = c.a();  m_b = c.m_b; }

    int a() const { return m_a; }
    double b() const { return m_b; }

private:
    int m_a;
    double& m_b;
};

class D
{
public:
    D() : m_a{} {}
    D(float a) : m_a{a} {}
    float a() const { return m_a; }

private:
    float m_a;
};

namespace mpirpc
{

template<typename Buffer, typename Alignment, typename Options>
struct unmarshaller<C,Buffer,Alignment,Options>
{
    using type = mpirpc::construction_info<C,
        std::tuple<int, double&>,
        std::tuple<int, double>,
        std::tuple<mpirpc::constructor_storage_duration_tag_type, mpirpc::function_group_shared_storage_duration_tag_type>
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

template<typename Buffer, typename Alignment, typename Options>
struct marshaller<C,Buffer,Alignment,Options>
{
    static void marshal(Buffer& buf, const C& v)
    {
        buf.template push<int>(v.a());
        buf.template push<double>(v.b());
        std::cout << "marshalling: C(" << v.a() << "," <<  v.b() << ")" << std::endl;
    }
};

template<typename Buffer, typename Alignment, typename Options>
struct unmarshaller<D, Buffer, Alignment,Options>
{
    template<typename Allocator>
    static D unmarshal(Allocator&& a, Buffer& b)
    {
        return {mpirpc::get<float>(b, a)};
    }
};

template<typename Buffer, typename Alignment, typename Options>
struct marshaller<D,Buffer,Alignment,Options>
{
    static void marshal(Buffer& buf, const D& v)
    {
        buf.template push<float>(v.a());
    }
};

}

std::ostream& operator<<(std::ostream& os, const D& d)
{
    os << "D(" << d.a() << ")";
}

template<typename... Ts, std::size_t... Is>
void output_tuple(std::ostream& os, const std::tuple<Ts...>& v, std::index_sequence<Is...>)
{
    using swallow = int[];
    (void)swallow{(os << std::get<Is>(v) << ' ', 0)...};
}

template<typename...Ts>
std::ostream& operator<<(std::ostream& os, const std::tuple<Ts...>& v)
{
    os <<  "tuple( ";
    output_tuple(os, v, std::index_sequence_for<Ts...>{});
    os << ')';
    return os;
}

template<typename T, typename... ConstructorArgumentTypes, typename... ArgumentTypes, typename... StoredArguments>
std::ostream& operator<<(std::ostream& os, const mpirpc::construction_info<T,std::tuple<ConstructorArgumentTypes...>,std::tuple<ArgumentTypes...>,std::tuple<StoredArguments...>>& v)
{
    os << "construction_info(" << v.args() << ')';
    return os;
}

TEST(aligned_type_holder, basic_from_basic)
{
    std::tuple<double> test_args(5.4);
    mpirpc::internal::reconstruction::aligned_type_holder<
        double
      , std::integral_constant<std::size_t, alignof(double)>
      , std::tuple<double>
      , std::tuple<double>
      , std::tuple<mpirpc::constructor_storage_duration_tag_type>
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
      , std::tuple<mpirpc::constructor_storage_duration_tag_type>
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
      , std::tuple<mpirpc::constructor_storage_duration_tag_type>
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
      , std::tuple<mpirpc::constructor_storage_duration_tag_type>
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
      , std::tuple<mpirpc::function_storage_duration_tag_type>
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
          , std::tuple<mpirpc::function_storage_duration_tag_type>
          , std::tuple<std::integral_constant<std::size_t, alignof(double[5])>>
      >::base_type_holder).name(), 0, 0, 0)
      <<  std::endl;
    std::cout << abi::__cxa_demangle(typeid(typename
        mpirpc::internal::reconstruction::aligned_type_holder<
            double[5]
          , std::integral_constant<std::size_t, alignof(double[5])>
          , std::tuple<double(&)[5]>
          , std::tuple<double(&)[5]>
          , std::tuple<mpirpc::function_storage_duration_tag_type>
          , std::tuple<std::integral_constant<std::size_t, alignof(double[5])>>
      >::aligned_storage_tuple_type).name(), 0, 0, 0)
      <<  std::endl;
    std::cout << abi::__cxa_demangle(typeid(typename
        mpirpc::internal::reconstruction::aligned_type_holder<
            double[5]
          , std::integral_constant<std::size_t, alignof(double[5])>
          , std::tuple<double(&)[5]>
          , std::tuple<double(&)[5]>
          , std::tuple<mpirpc::function_storage_duration_tag_type>
          , std::tuple<std::integral_constant<std::size_t, alignof(double[5])>>
      >::type_storage).name(), 0, 0, 0)
      <<  std::endl;
    std::cout << abi::__cxa_demangle(typeid(typename
        mpirpc::internal::reconstruction::aligned_type_holder<
            double[5][7]
          , std::integral_constant<std::size_t, alignof(double[5][7])>
          , std::tuple<double(&)[5][7]>
          , std::tuple<double(&)[5][7]>
          , std::tuple<mpirpc::function_storage_duration_tag_type>
          , std::tuple<std::integral_constant<std::size_t, alignof(double[5][7])>>
      >::base_type_holder).name(), 0, 0, 0)
      <<  std::endl;
    std::cout << abi::__cxa_demangle(typeid(typename
        mpirpc::internal::reconstruction::aligned_type_holder<
            double[5][7]
          , std::integral_constant<std::size_t, alignof(double[5][7])>
          , std::tuple<double(&)[5][7]>
          , std::tuple<double(&)[5][7]>
          , std::tuple<mpirpc::function_storage_duration_tag_type>
          , std::tuple<std::integral_constant<std::size_t, alignof(double[5][7])>>
      >::aligned_storage_tuple_type).name(), 0, 0, 0)
      <<  std::endl;
    std::cout << abi::__cxa_demangle(typeid(typename
        mpirpc::internal::reconstruction::aligned_type_holder<
            double[5][7]
          , std::integral_constant<std::size_t, alignof(double[5][7])>
          , std::tuple<double(&)[5][7]>
          , std::tuple<double(&)[5][7]>
          , std::tuple<mpirpc::function_storage_duration_tag_type>
          , std::tuple<std::integral_constant<std::size_t, alignof(double[5][7])>>
      >::type_storage).name(), 0, 0, 0)
      <<  std::endl;

    /*C c(1, b_global);
    mpirpc::parameter_buffer<> buff;
    mpirpc::marshaller<C, mpirpc::parameter_buffer<>, std::integral_constant<std::size_t, alignof(C)>>::marshal(buff, c);
    buff.seek(0);
    using UT = mpirpc::unmarshaller_type<C,mpirpc::parameter_buffer<>,std::integral_constant<std::size_t, alignof(C)>>;
    using ATH = typename UT::template aligned_type_holder<std::integral_constant<std::size_t, alignof(C)>>;
    using IASTT = typename ATH::individual_aligned_storage_tuple_type;
    using SASTT = typename ATH::shared_aligned_storage_tuple_type;
    IASTT id;
    SASTT sd;
    std::allocator<char> a;
    std::aligned_storage<sizeof(C), alignof(C)>  cs;
    C* cp = (C*) &cs;
    ATH::template construct<true>(cp, id, sd, mpirpc::unmarshaller<C, mpirpc::parameter_buffer<>, std::integral_constant<std::size_t, alignof(C)>, void>::unmarshal(std::allocator<char>(), buff).args());
    std::cout << cp->a() <<  " " << cp->b() <<  std::endl;
    */

    std::cout << abi::__cxa_demangle(typeid(std::tuple<mpirpc::unmarshaller_type<double[5][7], mpirpc::parameter_buffer<>, std::allocator<char>>>).name(), 0, 0, 0) <<  std::endl;
    std::cout << "-----------------" << std::endl;
    std::cout << abi::__cxa_demangle(typeid(std::tuple<mpirpc::unmarshaller_type<C[5][7], mpirpc::parameter_buffer<>, std::allocator<char>>>).name(), 0, 0, 0) <<  std::endl;
    std::cout << "-----------------" << std::endl;
    std::cout << abi::__cxa_demangle(typeid(std::remove_all_extents_t<double[5][7]>).name(), 0, 0, 0) <<  std::endl;
    std::cout << abi::__cxa_demangle(typeid(mpirpc::internal::retype_array_type<C[5][7], mpirpc::unmarshaller_type<C, mpirpc::parameter_buffer<>, std::allocator<char>>>).name(), 0, 0, 0) <<  std::endl;
    std::cout <<  mpirpc::internal::array_total_elements_v<C[5][7]> <<  std::endl;
    std::cout << "blah: " <<  (mpirpc::is_construction_info_v<mpirpc::unmarshaller_type<std::remove_all_extents_t<C[5][7]>,mpirpc::parameter_buffer<>,std::integral_constant<std::size_t, alignof(C)>>>) << std::endl;
    {
        mpirpc::parameter_buffer<> buff;
        C (*blah)[3] = new C[2][3];
        for (std::size_t i = 0; i < 2; ++i)
        {
            for (std::size_t j = 0; j < 3; ++j)
            {
                blah[i][j] = C(i+10, b_global);
                mpirpc::marshaller<C, mpirpc::parameter_buffer<>, std::integral_constant<std::size_t, alignof(C)>>::marshal(buff, blah[i][j]);
            }
        }
        buff.seek(0);
        auto ret = mpirpc::unmarshaller<C[2][3], mpirpc::parameter_buffer<>, std::integral_constant<std::size_t, alignof(C)>, void>::unmarshal(std::allocator<char>(), buff);
        for (std::size_t i = 0; i < 2; ++i)
        {
            for (std::size_t j = 0; j < 3; ++j)
            {
                std::cout << ret.pointer()[i][j].a() << " " << ret.pointer()[i][j].b() << std::endl;
            }
        }
        std::cout << abi::__cxa_demangle(typeid(ret).name(), 0, 0, 0) <<  std::endl;
    }
    std::cout <<  "Uknown bounds:" <<  std::endl;
    {
        mpirpc::parameter_buffer<> buff;
        C (*blah)[3] = new C[2][3];
        buff.template push<std::size_t>((std::size_t) 2);
        for (std::size_t i = 0; i < 2; ++i)
        {
            for (std::size_t j = 0; j < 3; ++j)
            {
                blah[i][j] = C(i, b_global);
                mpirpc::marshaller<C, mpirpc::parameter_buffer<>, std::integral_constant<std::size_t, alignof(C)>>::marshal(buff, blah[i][j]);
            }
        }
        buff.seek(0);
        std::cout <<  "here" <<  std::endl;
        auto tmp =  mpirpc::unmarshaller<C[][3], mpirpc::parameter_buffer<>, std::integral_constant<std::size_t, alignof(C)>, void>::unmarshal(std::allocator<char>(), buff);
        std::cout <<  "here2" <<  std::endl;
        //auto ret = std::move(tmp.pointer());
        std::cout << abi::__cxa_demangle(typeid(tmp).name(), 0, 0, 0) <<  std::endl;
        for (std::size_t i = 0; i < 2; ++i)
        {
            for (std::size_t j = 0; j < 3; ++j)
            {
                std::cout << tmp[i][j].a() << " " << tmp[i][j].b() << std::endl;
            }
        }

    }
    std::cout << "D array" << std::endl;
    {
        mpirpc::parameter_buffer<> buff;
        D (*blah)[3] = new D[2][3];
        for (std::size_t i = 0; i < 2; ++i)
        {
            for (std::size_t j = 0; j < 3; ++j)
            {
                blah[i][j] = D(i*3+j);
                mpirpc::marshaller<D, mpirpc::parameter_buffer<>, std::integral_constant<std::size_t, alignof(D)>>::marshal(buff, blah[i][j]);
            }
        }
        buff.seek(0);
        auto ret = mpirpc::unmarshaller<D[2][3], mpirpc::parameter_buffer<>, std::integral_constant<std::size_t, alignof(D)>,void>::unmarshal(std::allocator<char>(), buff);
        for (std::size_t i = 0; i < 2; ++i)
        {
            for (std::size_t j = 0; j < 3; ++j)
            {
                std::cout << ret[i][j].a() << std::endl;
            }
        }
        std::cout << abi::__cxa_demangle(typeid(ret).name(), 0, 0, 0) <<  std::endl;
    }
    std::cout <<  "Uknown bounds with D:" <<  std::endl;
    {
        mpirpc::parameter_buffer<> buff;
        D (*blah)[3] = new D[2][3];
        buff.template push<std::size_t>((std::size_t) 2);
        for (std::size_t i = 0; i < 2; ++i)
        {
            for (std::size_t j = 0; j < 3; ++j)
            {
                blah[i][j] = D(i*3+j);
                mpirpc::marshaller<D, mpirpc::parameter_buffer<>, std::integral_constant<std::size_t, alignof(D)>>::marshal(buff, blah[i][j]);
            }
        }
        buff.seek(0);
        std::cout <<  "here" <<  std::endl;
        auto tmp =  mpirpc::unmarshaller<D[][3], mpirpc::parameter_buffer<>, std::integral_constant<std::size_t, alignof(D)>,void>::unmarshal(std::allocator<char>(), buff);
        std::cout <<  "here2" <<  std::endl;
        //auto ret = std::move(tmp.pointer());
        std::cout << abi::__cxa_demangle(typeid(tmp).name(), 0, 0, 0) <<  std::endl;
        for (std::size_t i = 0; i < 2; ++i)
        {
            for (std::size_t j = 0; j < 3; ++j)
            {
                std::cout << tmp[i][j].a() << std::endl;
            }
        }

    }
}

template<typename T>
class get_side_effects_wrapper;

template<typename T>
class get_side_effects_wrapper<T&>
{
public:
    get_side_effects_wrapper(T& t) : m_value{t} {}
    T& get() { return m_value; }

private:
    T& m_value;
};

template<typename T>
class get_side_effects_wrapper<T&&>
{
public:
    get_side_effects_wrapper(T&& t) : m_value{std::move(t)} {}
    T&& get() { return m_value; }

private:
    T&& m_value;
};

template<typename Tuple>
struct tuple_index_sequence_helper;

template<typename... Ts>
struct tuple_index_sequence_helper<std::tuple<Ts...>>
{
    using type = std::index_sequence_for<Ts...>;
};

template<typename Tuple>
using tuple_index_sequence = typename tuple_index_sequence_helper<Tuple>::type;

template<typename T,  typename... Tags>
class tagged_parameter_wrapper;

template<typename T>
struct is_tagged_parameter_wrapper :  std::false_type {};

template<typename T,  typename... Tags>
struct is_tagged_parameter_wrapper<tagged_parameter_wrapper<T, Tags...>> :  std::true_type {};

template<typename T>
using is_tagged_parameter_wrapper_type = typename is_tagged_parameter_wrapper<T>::type;

template<typename T>
constexpr bool is_tagged_parameter_wrapper_v = is_tagged_parameter_wrapper<T>::value;

template<typename T,  typename... Tags>
class tagged_parameter_wrapper<T&, Tags...>
{
private:
    template<typename NewTag, std::size_t... Is, std::enable_if_t<std::is_empty<NewTag>::value>* = nullptr>
    tagged_parameter_wrapper<T&, Tags..., NewTag> append_tag(std::index_sequence<Is...>)
    {
        return {m_value, std::get<Is>(m_runtime_data)...};
    }

    template<typename NewTag, std::size_t... Is, std::enable_if_t<!std::is_empty<NewTag>::value>* = nullptr>
    tagged_parameter_wrapper<T&, Tags..., NewTag> append_tag(std::index_sequence<Is...>, NewTag&& tag)
    {
        return {m_value, std::get<Is>(m_runtime_data)..., std::forward<NewTag>(tag)};
    }

public:
    using runtime_data_tuple_type = mpirpc::internal::filter_tuple<std::integer_sequence<bool, !std::is_empty<Tags>::value...>, std::tuple<Tags...>>;
    using type = T&;

    template<typename... RTD>
    constexpr tagged_parameter_wrapper(T& t, RTD&&... rtd) noexcept : m_value(t), m_runtime_data{std::forward<RTD>(rtd)...} {}
    T& get() const { return m_value; }

    template<typename NewTag, std::enable_if_t<std::is_empty<NewTag>::value>* = nullptr>
    tagged_parameter_wrapper<T&, Tags..., NewTag> append_tag()
    {
        return append_tag<NewTag>(tuple_index_sequence<runtime_data_tuple_type>{});
    }

    template<typename NewTag, std::enable_if_t<!std::is_empty<NewTag>::value>* = nullptr>
    tagged_parameter_wrapper<T&, Tags..., NewTag> append_tag(NewTag&& tag)
    {
        return append_tag<NewTag>(tuple_index_sequence<runtime_data_tuple_type>{}, std::forward<NewTag>(tag));
    }

private:
    T& m_value;
    runtime_data_tuple_type m_runtime_data;
};

template<typename T,  typename... Tags>
class tagged_parameter_wrapper<T&&, Tags...>
{
private:
    template<typename NewTag, std::size_t... Is, std::enable_if_t<std::is_empty<NewTag>::value>* = nullptr>
    tagged_parameter_wrapper<T&&, Tags..., NewTag> append_tag(std::index_sequence<Is...>)
    {
        return {std::move(m_value), std::get<Is>(m_runtime_data)...};
    }

    template<typename NewTag, std::size_t... Is, std::enable_if_t<!std::is_empty<NewTag>::value>* = nullptr>
    tagged_parameter_wrapper<T&&, Tags..., NewTag> append_tag(std::index_sequence<Is...>, NewTag&& tag)
    {
        return {std::move(m_value), std::get<Is>(m_runtime_data)..., std::forward<NewTag>(tag)};
    }

public:
    using runtime_data_tuple_type = mpirpc::internal::filter_tuple<std::integer_sequence<bool, !std::is_empty<Tags>::value...>, std::tuple<Tags...>>;
    using type = T&&;

    template<typename... RTD>
    constexpr tagged_parameter_wrapper(T&& t, RTD&&... rtd) noexcept : m_value(std::move(t)), m_runtime_data{std::forward<RTD>(rtd)...} {}
    T&& get() const { return m_value; }

    template<typename NewTag, std::enable_if_t<std::is_empty<NewTag>::value>* = nullptr>
    tagged_parameter_wrapper<T&&, Tags..., NewTag> append_tag()
    {
        return append_tag<NewTag>(tuple_index_sequence<runtime_data_tuple_type>{});
    }

    template<typename NewTag, std::enable_if_t<!std::is_empty<NewTag>::value>* = nullptr>
    tagged_parameter_wrapper<T&&, Tags..., NewTag> append_tag(NewTag&& tag)
    {
        return append_tag<NewTag>(tuple_index_sequence<runtime_data_tuple_type>{}, std::forward<NewTag>(tag));
    }

private:
    T&& m_value;
    runtime_data_tuple_type m_runtime_data;
};

struct side_effects_tag_type {};

template<typename T, std::enable_if_t<!is_tagged_parameter_wrapper_v<std::remove_reference_t<T>>>* = nullptr>
auto side_effects(T&& t)
    -> tagged_parameter_wrapper<T&&, side_effects_tag_type>
{
    return tagged_parameter_wrapper<T&&, side_effects_tag_type>(std::forward<T>(t));
}

template<typename T, std::enable_if_t<is_tagged_parameter_wrapper_v<std::remove_reference_t<T>>>* = nullptr>
auto side_effects(T&& t)
{
    return t.template append<side_effects_tag_type>();
}

template<typename T>
class decayed_array_tag_type;

template<typename T, std::size_t N>
class decayed_array_tag_type<T[N]>
{
public:
    constexpr static std::size_t size() noexcept { return N; }
};

template<typename T>
class decayed_array_tag_type<T[]>
{
public:
    constexpr decayed_array_tag_type(std::size_t n) noexcept : m_size{n} {}
    constexpr std::size_t size() const noexcept { return m_size; }

private:
    std::size_t m_size;
};

template<std::size_t N,  typename T, std::enable_if_t<std::is_pointer<std::remove_reference_t<T>>::value>* = nullptr>
decltype(auto) decayed_array(T&& t)
{
    using tag_type = decayed_array_tag_type<std::remove_pointer_t<std::remove_reference_t<T>>[N]>;
    return tagged_parameter_wrapper<T&&, tag_type>(std::forward<T>(t));
};

// template<typename T, std::enable_if_t<false>* = nullptr>
// decltype(auto) decayed_array(T&&t, std::size_t size);

template<typename T, std::enable_if_t<std::is_pointer<std::remove_reference_t<T>>::value>* = nullptr>
decltype(auto) decayed_array(T&& t, std::size_t size)
{
    using tag_type = decayed_array_tag_type<std::remove_pointer_t<std::remove_reference_t<T>>[]>;
    return tagged_parameter_wrapper<T&&, tag_type>(std::forward<T>(t), tag_type(size));
};

template<std::size_t N,  typename T, std::enable_if_t<is_tagged_parameter_wrapper_v<std::remove_reference_t<T>>>* = nullptr>
decltype(auto) decayed_array(T&& t)
{
    using tag_type = decayed_array_tag_type<std::remove_pointer_t<std::remove_reference_t<typename T::type>>[N]>;
    return t.template append_tag<tag_type>();
};

template<typename T, std::enable_if_t<is_tagged_parameter_wrapper_v<std::remove_reference_t<T>>>* = nullptr>
decltype(auto) decayed_array(T&& t, std::size_t size)
{
    using tag_type = decayed_array_tag_type<std::remove_pointer_t<std::remove_reference_t<typename T::type>>[]>;
    return t.template append_tag<tag_type>(tag_type(size));
};



TEST(aligned_type_holder, wrapper)
{
    int i = 5;
    auto lv = side_effects(i);
    std::cout << abi::__cxa_demangle(typeid(lv).name(), 0, 0, 0) <<  std::endl;
    auto rv = side_effects(6);
    std::cout << abi::__cxa_demangle(typeid(rv).name(), 0, 0, 0) <<  std::endl;

    const int i2 = 8;
    auto cv = side_effects(i2);
    std::cout << abi::__cxa_demangle(typeid(cv).name(), 0, 0, 0) <<  std::endl;


    int *t = new int(7);
    {
        auto test = decayed_array<5>(t);
        std::cout << abi::__cxa_demangle(typeid(test).name(), 0, 0, 0) <<  std::endl;
        tagged_parameter_wrapper<int*&> test2{t};
        auto test3 = decayed_array<5>(std::move(test2));
        std::cout << abi::__cxa_demangle(typeid(test3).name(), 0, 0, 0) <<  std::endl;
        auto test4 = decayed_array(t, 5);
        std::cout << abi::__cxa_demangle(typeid(test4).name(), 0, 0, 0) <<  std::endl;
        auto test5 = decayed_array(std::move(test2), 5);
        std::cout << abi::__cxa_demangle(typeid(test5).name(), 0, 0, 0) <<  std::endl;
        auto test6 = decayed_array(side_effects(t), 5);
        std::cout << abi::__cxa_demangle(typeid(test6).name(), 0, 0, 0) <<  std::endl;
    }
    {
        auto test = decayed_array<5>(std::move(t));
        std::cout << abi::__cxa_demangle(typeid(test).name(), 0, 0, 0) <<  std::endl;
        tagged_parameter_wrapper<int*&&> test2{std::move(t)};
        auto test3 = decayed_array<5>(std::move(test2));
        std::cout << abi::__cxa_demangle(typeid(test3).name(), 0, 0, 0) <<  std::endl;
        auto test4 = decayed_array(std::move(t), 5);
        std::cout << abi::__cxa_demangle(typeid(test4).name(), 0, 0, 0) <<  std::endl;
        auto test5 = decayed_array(std::move(test2), 5);
        std::cout << abi::__cxa_demangle(typeid(test5).name(), 0, 0, 0) <<  std::endl;
        auto test6 = decayed_array(side_effects(std::move(t)), 5);
        std::cout << abi::__cxa_demangle(typeid(test6).name(), 0, 0, 0) <<  std::endl;
    }

    int (*t2)[5] = new int[3][5]();
    auto test = decayed_array(t2, 3);
    std::cout << abi::__cxa_demangle(typeid(test).name(), 0, 0, 0) <<  std::endl;

    auto test3 = decayed_array<3>(t2);
    std::cout << abi::__cxa_demangle(typeid(test3).name(), 0, 0, 0) <<  std::endl;

    const int (*t3)[5] = new int[3][5]();
    auto test2 = decayed_array(t3, 3);
    std::cout << abi::__cxa_demangle(typeid(test2).name(), 0, 0, 0) <<  std::endl;
    //std::cout << abi::__cxa_demangle(typeid(tagged_parameter_wrapper<int*&, pointer_elements<0>>::runtime_data_tuple_type).name(), 0, 0, 0) <<  std::endl;
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
