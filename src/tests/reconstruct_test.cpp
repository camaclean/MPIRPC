#include <gtest/gtest.h>
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

#include <gtest/gtest.h>
#include <vector>
#include "../parameter_buffer.hpp"
#include "../construction_info.hpp"
#include "../internal/reconstruction/parameter_container.hpp"
#include "../unmarshaller.hpp"
#include "../common.hpp"

TEST(reconstruct_types, is_construction_info)
{
    using CI1 = mpirpc::construction_info<std::tuple<int>,std::tuple<int>,std::tuple<int>,std::tuple<mpirpc::constructor_storage_duration_tag_type>>;
    using CI2 = mpirpc::construction_info<std::tuple<int,std::vector<double>>,std::tuple<int,std::vector<double>>,std::tuple<int,const std::vector<double>&>,std::tuple<mpirpc::constructor_storage_duration_tag_type,mpirpc::constructor_storage_duration_tag_type>>;
    ASSERT_EQ(false, mpirpc::internal::reconstruction::is_construction_info_v<void>);
    ASSERT_EQ(false, mpirpc::internal::reconstruction::is_construction_info_v<double>);
    ASSERT_EQ(false, mpirpc::internal::reconstruction::is_construction_info_v<std::vector<double>>);
    ASSERT_EQ(true, mpirpc::internal::reconstruction::is_construction_info_v<CI1>);
    ASSERT_EQ(true, mpirpc::internal::reconstruction::is_construction_info_v<CI2>);
}

TEST(reconstruct_types, is_aligned_type_holder)
{
    using CI1 = mpirpc::construction_info<std::tuple<int>,std::tuple<int>,std::tuple<int>,std::tuple<mpirpc::constructor_storage_duration_tag_type>>;
    using CI2 = mpirpc::construction_info<std::tuple<int,std::vector<double>>,std::tuple<int,std::vector<double>>,std::tuple<int,const std::vector<double>&>,std::tuple<mpirpc::constructor_storage_duration_tag_type,mpirpc::constructor_storage_duration_tag_type>>;
    ASSERT_EQ(false, mpirpc::internal::reconstruction::is_aligned_type_holder_v<void>);
    ASSERT_EQ(false, mpirpc::internal::reconstruction::is_aligned_type_holder_v<double>);
    ASSERT_EQ(false, mpirpc::internal::reconstruction::is_aligned_type_holder_v<std::vector<double>>);
    using ATH1 = typename CI1::template aligned_type_holder<std::integral_constant<std::size_t, alignof(int)>>;
    ASSERT_EQ(true, mpirpc::internal::reconstruction::is_aligned_type_holder_v<ATH1>);
    using ATH2 = typename CI2::template aligned_type_holder<std::tuple<std::integral_constant<std::size_t, 32>,std::integral_constant<std::size_t,alignof(int)>,std::integral_constant<std::size_t,alignof(std::vector<double>)>>>;
    ASSERT_EQ(true, mpirpc::internal::reconstruction::is_aligned_type_holder_v<ATH2>);
}

class C
{
public:
    C(int a, double b) : m_a(a), m_b(b) {};
    int a() const { return m_a; }
    double b() const { return m_b; }
private:
    int m_a;
    double m_b;
};

class D {};

TEST(construction_alignment,scalar)
{
    ASSERT_TRUE(
        (std::is_same<
            std::tuple<std::integral_constant<std::size_t,alignof(double)>,std::integral_constant<std::size_t,alignof(double)>>,
            mpirpc::internal::reconstruction::construction_alignments_type<double,std::tuple<double>,std::tuple<>>
        >::value)
    );
    ASSERT_TRUE(
        (std::is_same<
            std::tuple<std::integral_constant<std::size_t,alignof(double)>,std::integral_constant<std::size_t,alignof(double)>>,
            mpirpc::internal::reconstruction::construction_alignments_type<double,std::tuple<double>,std::integral_constant<std::size_t,alignof(double)>>
        >::value)
    );
    ASSERT_TRUE(
        (std::is_same<
            std::tuple<std::integral_constant<std::size_t,alignof(double)>,std::integral_constant<std::size_t,alignof(double)>>,
            mpirpc::internal::reconstruction::construction_alignments_type<double,std::tuple<double>,std::tuple<std::integral_constant<std::size_t,alignof(double)>>>
        >::value)
    );
    ASSERT_TRUE(
        (std::is_same<
            std::tuple<std::integral_constant<std::size_t,alignof(double)>,std::integral_constant<std::size_t,alignof(double)>>,
            mpirpc::internal::reconstruction::construction_alignments_type<double,std::tuple<double>,std::tuple<std::integral_constant<std::size_t,alignof(double)>,std::integral_constant<std::size_t,alignof(double)>>>
        >::value)
    );
}

TEST(construction_alignment,class)
{
    ASSERT_TRUE(
        (std::is_same<
            std::tuple<std::integral_constant<std::size_t,alignof(C)>,std::integral_constant<std::size_t,alignof(int)>,std::integral_constant<std::size_t,alignof(double)>>,
            mpirpc::internal::reconstruction::construction_alignments_type<C,std::tuple<int,double>,std::tuple<>>
        >::value)
    );
    ASSERT_TRUE(
        (std::is_same<
            std::tuple<std::integral_constant<std::size_t,alignof(C)>,std::integral_constant<std::size_t,alignof(int)>,std::integral_constant<std::size_t,alignof(double)>>,
            mpirpc::internal::reconstruction::construction_alignments_type<C,std::tuple<int,double>,std::integral_constant<std::size_t,alignof(C)>>
        >::value)
    );
    ASSERT_TRUE(
        (std::is_same<
            std::tuple<std::integral_constant<std::size_t,alignof(C)>,std::integral_constant<std::size_t,alignof(int)>,std::integral_constant<std::size_t,alignof(double)>>,
            mpirpc::internal::reconstruction::construction_alignments_type<C,std::tuple<int,double>,std::tuple<std::integral_constant<std::size_t,alignof(C)>>>
        >::value)
    );
    ASSERT_TRUE(
        (std::is_same<
            std::tuple<std::integral_constant<std::size_t,alignof(C)>,std::integral_constant<std::size_t,alignof(int)>,std::integral_constant<std::size_t,alignof(double)>>,
            mpirpc::internal::reconstruction::construction_alignments_type<C,std::tuple<int,double>,std::tuple<std::integral_constant<std::size_t,alignof(C)>,std::integral_constant<std::size_t,alignof(int)>,std::integral_constant<std::size_t,alignof(double)>>>
        >::value)
    );
}

TEST(construction_alignment,noargs)
{
    ASSERT_TRUE(
        (std::is_same<
            std::tuple<std::integral_constant<std::size_t,alignof(D)>>,
            mpirpc::internal::reconstruction::construction_alignments_type<D,std::tuple<>,std::tuple<>>
        >::value)
    );
    ASSERT_TRUE(
        (std::is_same<
            std::tuple<std::integral_constant<std::size_t,alignof(D)>>,
            mpirpc::internal::reconstruction::construction_alignments_type<D,std::tuple<>,std::integral_constant<std::size_t,alignof(D)>>
        >::value)
    );
    ASSERT_TRUE(
        (std::is_same<
            std::tuple<std::integral_constant<std::size_t,alignof(D)>>,
            mpirpc::internal::reconstruction::construction_alignments_type<D,std::tuple<>,std::tuple<std::integral_constant<std::size_t,alignof(D)>>>
        >::value)
    );
    ASSERT_TRUE(
        (std::is_same<
            std::tuple<std::integral_constant<std::size_t,alignof(C)>,std::integral_constant<std::size_t,alignof(int)>,std::integral_constant<std::size_t,alignof(double)>>,
            mpirpc::internal::reconstruction::construction_alignments_type<C,std::tuple<int,double>,std::tuple<std::integral_constant<std::size_t,alignof(C)>,std::integral_constant<std::size_t,alignof(int)>,std::integral_constant<std::size_t,alignof(double)>>>
        >::value)
    );
}

class B
{
public:
    B(int b) : m_b{b} { std::cout << "constructed B" << std::endl; }
    int m_b;
    ~B() { std::cout << "Deleted B " << this << std::endl; }
};

class A
{
    template<typename T, typename Buffer, typename Alignment, typename>
    friend struct mpirpc::marshaller;
public:
    A(const double v1, int v2 ,const float& v3, bool& v4, long v5, const long long& v6, const B& b)
        : m_v1{v1}, m_v2{v2}, m_v3{v3}, m_v4{v4}, m_v5{v5}, m_v6{v6}, m_b{b}
    { std::cout << "constructed A " << b.m_b << std::endl; }
    A() = delete;
    A(const A&) = delete;
    A(A&&) = delete;
    ~A() { std::cout << "Deleted A" << std::endl; }
    double add () const
    {
        return m_v1 + m_v2 + m_v3 + m_v5 + m_v6 + m_b.m_b;
    }
private:
    const double m_v1;
    int m_v2;
    const float m_v3;
    bool& m_v4;
    long m_v5;
    const long long m_v6;
    B m_b;
};

namespace mpirpc
{

template<typename Buffer, typename Alignment, typename Options>
struct unmarshaller<B,Buffer,Alignment,Options>
{
    using type = mpirpc::construction_info<B,
        std::tuple<int>,
        std::tuple<int>,
        std::tuple<mpirpc::constructor_storage_duration_tag_type>
    >;
    template<typename Allocator>
    static type unmarshal(Allocator&& a, Buffer& b)
    {
        return type{mpirpc::get<int>(b,a)};
    }
};

template<typename Buffer, typename Alignment, typename Options>
struct marshaller<B,Buffer,Alignment,Options>
{
    static void marshal(Buffer& buf, const B& b)
    {
        buf.template push<int>(b.m_b);
    }
};

/*
 * class mpirpc::construction_info_to_aligned_type_holder<
 *      mpirpc::construction_info<A,
 *          std::tuple<const double, int, const float&, bool&, long int, const long long int, const B&>,
 *          std::tuple<const double, int, const float&, bool&, long int, const long long int, mpirpc::construction_info<B,
 *              std::tuple<int>,
 *              std::tuple<int>,
 *              std::tuple<std::integral_constant<bool, false> > >
 *          >,
 *          std::tuple<std::integral_constant<bool, false>, std::integral_constant<bool, false>, std::integral_constant<bool, false>, std::integral_constant<bool, true>, std::integral_constant<bool, false>, std::integral_constant<bool, false>, std::integral_constant<bool, false> > >,
 *          std::integral_constant<long unsigned int, 8ul> >
 *
 */

template<typename Buffer, typename Alignment>
struct marshaller<A,Buffer,Alignment>
{
    static void marshal(Buffer& b, const A& a)
    {
        b.template push<double>(a.m_v1);
        b.template push<int>(a.m_v2);
        b.template push<float>(a.m_v3);
        b.template push<bool>(a.m_v4);
        b.template push<long>(a.m_v5);
        b.template push<long long>(a.m_v6);
        b.template push<B>(a.m_b);
    }
};

template<typename Buffer, typename Alignment, typename Options>
struct unmarshaller<A,Buffer,Alignment,Options>
{
    using type = mpirpc::construction_info<A,
        std::tuple<const double, int, const float&, bool&, long, const long long, const B&>,
        std::tuple<const double, int, const float&, bool&, long, const long long, mpirpc::unmarshaller_type<B,mpirpc::parameter_buffer<>,std::allocator<char>>>,
        std::tuple<
            mpirpc::constructor_storage_duration_tag_type,
            mpirpc::constructor_storage_duration_tag_type,
            mpirpc::constructor_storage_duration_tag_type,
            mpirpc::function_storage_duration_tag_type,
            mpirpc::constructor_storage_duration_tag_type,
            mpirpc::constructor_storage_duration_tag_type,
            mpirpc::constructor_storage_duration_tag_type
        >
    >;
    template<typename Allocator>
    static type unmarshal(Allocator&& a, Buffer& b)
    {
        type ret{
            mpirpc::get<double>(b,a),
            mpirpc::get<int>(b,a),
            mpirpc::get<float>(b,a),
            mpirpc::get<bool>(b,a),
            mpirpc::get<long>(b,a),
            mpirpc::get<long long>(b,a),
            mpirpc::get<B>(b,a)
        };
        std::cout << std::get<0>(ret.args()) << std::endl;
        return ret;
        //class construction_info<T,std::tuple<ConstructorArgumentTypes...>,std::tuple<ArgumentTypes...>,std::tuple<StoredArguments...>>
    }
};

}

TEST(Test,parameter_setup)
{
    //mpirpc::unmarshaller_type<std::vector<double>,mpirpc::parameter_buffer<>,std::allocator<char>> a;
    mpirpc::parameter_buffer<> pb;
    pb.template push<int,std::integral_constant<std::size_t,32>>(2);
    B bval(5);
    bool boolval = true;
    A aval(3.5, 4, 12.9f, boolval, 1004L, 21414LL, bval);
    B bval2(9);
    pb.template push<A,std::integral_constant<std::size_t,32>>(aval);
    pb.template push<B>(bval2);
    pb.seek(0);
    using ArgsTuple = std::tuple<int,A&,B>;
    using AlignmentsTuple = std::tuple<
                                std::integral_constant<std::size_t,32>,
                                std::integral_constant<std::size_t,32>,
                                std::integral_constant<std::size_t,alignof(B)>
                            >;
#if 1
    mpirpc::internal::reconstruction::parameter_container<mpirpc::parameter_buffer<std::allocator<char>>,
                            ArgsTuple,
                            std::tuple<int, A&, B>,
                            AlignmentsTuple
                            > ps(pb,std::allocator<char>());
#endif
    std::cout << "------------------------" << std::endl;
    std::cout << ps.get<0>() << std::endl;
    std::cout << ps.get<1>().add() << " " << ((uintptr_t) &ps.get<1>()) % 32 << std::endl;
    std::cout << ps.get<2>().m_b << std::endl;
    ASSERT_EQ(2,ps.get<0>());
    ASSERT_EQ(aval.add(),ps.get<1>().add());
    ASSERT_EQ(9,ps.get<2>().m_b);
    std::cout << "------------------------" << std::endl;
}

std::tuple<double[5]> make_test_tuple()
{
    return {};
}

int main(int argc, char **argv) {
    std::tuple<double[5]> test2();
    const std::tuple<double[5]>& test = make_test_tuple();
    //std::cout << std::boolalpha;
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
