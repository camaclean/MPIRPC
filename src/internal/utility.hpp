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

#ifndef MPIRPC__INTERNAL__UTILITY_HPP
#define MPIRPC__INTERNAL__UTILITY_HPP

#include "detail/utility.hpp"

#include <utility>

namespace mpirpc
{

namespace internal
{

/**
 * \internal
 * swallow can be used for ordered execution on template parameter packs
 */
using swallow = int[];

/**
 * \internal
 * Passer can be used along with uniform initialization to unpack parameter packs
 * and execute the parameters in the order in which they appear. This is necessary
 * for correctness when side effects are important.
 */
struct passer { passer(...) {} };

/**
 * \internal
 * See std::experimental::apply
 *
 * TEST: utility_test.cpp: Utility.apply
 */
template <typename F, typename Tuple>
decltype(auto) apply(F&& f, Tuple&& t);

/**
 * \internal
 * Like std::experimental::apply, but with a member function
 *
 * TEST: utility_test.cpp: Utility.apply_mem_fn
 */
template <typename F, class Class, typename Tuple>
decltype(auto) apply(F&& f, Class *c, Tuple&& t);

/**
 * \internal
 * Extracts an element at position Pos from Pack in template\<typename Int, Int... Pack\>
 * This struct inherits from std::integral_constant. See std::integral_constant for members.
 *
 * TEST: utility_test.cpp: Utility.integer_sequence_element_v
 */
template<std::size_t Pos, typename Int, Int... Is>
struct extract_integer_pack_element;

/**
 * \internal
 * Convenience variable template for extract_integer_pack_element::value.
 *
 * TEST: utility_test.cpp: Utility.integer_sequence_element_v
 */
template<std::size_t Pos, typename Int, Int... Is>
constexpr Int extract_integer_pack_element_v = extract_integer_pack_element<Pos,Int,Is...>::value;

template<std::size_t Pos, typename IntegerSequence>
struct integer_sequence_element;

/**
 * \internal
 * Extracts an element at position Pos from a std::integer_sequence\<Int,Int...\>
 * This struct inherits from std::integral_constant. See std::integral_constant for members.
 *
 * TEST: utility_test.cpp: Utility.integer_sequence_element_v
 */
template<std::size_t Pos, typename Int, Int... Is>
struct integer_sequence_element<Pos,std::integer_sequence<Int,Is...>> : extract_integer_pack_element<Pos,Int,Is...> {};

/**
 * \internal
 * Convenience template alias for integer_sequence_element. Gets a std::integral_constant\<Int,I\>
 */
template<std::size_t Pos, typename IntegerSequence>
using integer_sequence_element_type = typename integer_sequence_element<Pos,IntegerSequence>::type;

/**
 * \internal
 * Convenience variable template for integer_sequence_element::value.
 *
 * TEST: utility_test.cpp: Utility.integer_sequence_element_v
 */
template<std::size_t Pos, typename IntegerSequence>
constexpr decltype(integer_sequence_element<Pos,IntegerSequence>::value) integer_sequence_element_v = integer_sequence_element<Pos,IntegerSequence>::value;

/**
 * \internal
 * Like std::get(), but for a std::integer_sequence
 *
 * TEST: utility_test.cpp: Utility.integer_sequence_get
 */
template<std::size_t Pos, typename Int, Int...Is>
constexpr Int get(std::integer_sequence<Int,Is...>)
{
    return extract_integer_pack_element_v<Pos,Int,Is...>;
}

/**
 * \internal
 * Concatenate two std::integer_sequences of the same type
 *
 * TEST: utility_test.cpp: Utility.integer_sequence_cat
 */
template<typename Is1, typename... Is>
struct integer_sequence_cat;


/**
 * \internal
 * Convenience template alias for integer_sequence_cat::type
 *
 * TEST: utility_test.cpp: Utility.integer_sequence_cat
 */
template <typename Is1, typename... Is>
using integer_sequence_cat_type = typename integer_sequence_cat<Is1,Is...>::type;


/**
 * \internal
 * Prepend a value to a std::integer_sequence
 *
 * TEST: utility_test.cpp: Utility.integer_sequence_prepend
 */
template<typename Is, typename Is::value_type I>
struct integer_sequence_prepend;

/**
 * \internal
 * Convenience template alias for integer_sequence_prepend::type
 *
 * TEST: utility_test.cpp: Utility.integer_sequence_prepend
 */
template<typename Is, typename Is::value_type I>
using integer_sequence_prepend_type = typename integer_sequence_prepend<Is,I>::type;

/**
 * \internal
 * Append a value to a std::integer_sequence
 *
 * TEST: utility_test.cpp: Utility.integer_sequence_append
 */
template<typename Is, typename Is::value_type I>
struct integer_sequence_append;

/**
 * \internal
 * Convenience template alias for integer_sequence_append::type
 *
 * TEST: utility_test.cpp: Utility.integer_sequence_append
 */
template<typename Is, typename Is::value_type I>
using integer_sequence_append_type = typename integer_sequence_append<Is,I>::type;

/**
 * \internal
 * Conditionally prepend an integer to a std::integer_sequence
 *
 * TEST: utility_test.cpp: Utility.conditional_integer_sequence_prepend
 */
template<bool Condition, typename Is, typename Is::value_type I>
struct conditional_integer_sequence_prepend;


/**
 * \internal
 * Convenience template alias for conditional_index_sequence_prepend::type
 *
 * TEST: utility_test.cpp: Utility.conditional_integer_sequence_prepend
 */
template<bool Condition, typename Is, typename Is::value_type I>
using conditional_integer_sequence_prepend_type = typename conditional_integer_sequence_prepend<Condition,Is,I>::type;

/**
 * \internal
 * Conditionally append an integer to a std::integer_sequence
 *
 * TEST: utility_test.cpp: Utility.conditional_integer_sequence_append
 */
template<bool Condition, typename Is, typename Is::value_type I>
struct conditional_integer_sequence_append;

/**
 * \internal
 * Convenience template alias for conditional_index_sequence_append::type
 *
 * TEST: utility_test.cpp: Utility.conditional_integer_sequence_append
 */
template<bool Condition, typename Is, typename Is::value_type I>
using conditional_integer_sequence_append_type = typename conditional_integer_sequence_append<Condition,Is,I>::type;

/**
 * \internal
 * Concatenate tuple types
 *
 * TEST: utility_test.cpp: Utility.tuple_type_cat
 */
template<typename T1, typename... Tuples>
struct tuple_type_cat;

/**
 * \internal
 * Convenience template alias for tuple_type_cat::type
 *
 * TEST: utility_test.cpp: Utility.tuple_type_cat
 */
template<typename T1, typename... Tuples>
using tuple_type_cat_type = typename tuple_type_cat<T1,Tuples...>::type;

/**
 * \internal
 * Prepend a type to a tuple type
 *
 * TEST: utility_test.cpp: Utility.tuple_type_prepend
 */
template<typename T, typename Tuple>
struct tuple_type_prepend;

/**
 * \internal
 * Convenience template alias for tuple_type_prepend::type
 *
 * TEST: utility_test.cpp: Utility.tuple_type_prepend
 */
template<typename T, typename Tuple>
using tuple_type_prepend_type = typename tuple_type_prepend<T,Tuple>::type;

/**
 * \internal
 * Append a type to a tuple type
 *
 * TEST: utility_test.cpp: Utility.tuple_type_append
 */
template<typename Tuple, typename T>
struct tuple_type_append;

/**
 * \internal
 * Convenience template alias for tuple_type_append::type
 *
 * TEST: utility_test.cpp: Utility.tuple_type_append
 */
template<typename Tuple, typename T>
using tuple_type_append_type = typename tuple_type_append<Tuple,T>::type;

/**
 * \internal
 * Conditionally prepend a type to a tuple type
 *
 * TEST: utility_test.cpp: Utility.conditional_tuple_type_prepend
 */
template<bool Condition, typename T, typename Tuple>
struct conditional_tuple_type_prepend;

/**
 * \internal
 * Convenience template alias for conditional_tuple_type_prepend::type
 *
 * TEST: utility_test.cpp: Utility.conditional_tuple_type_prepend
 */
template<bool Condition, typename T, typename Tuple>
using conditional_tuple_type_prepend_type = typename conditional_tuple_type_prepend<Condition,T,Tuple>::type;

/**
 * \internal
 * Conditionally append a type to a tuple type
 *
 * TEST: utility_test.cpp: Utility.conditional_tuple_type_append
 */
template<bool Condition, typename Tuple, typename T>
struct conditional_tuple_type_append;

/**
 * \internal
 * Convenience template alias for conditional_tuple_type_append::type
 *
 * TEST: utility_test.cpp: Utility.conditional_tuple_type_append
 */
template<bool Condition, typename Tuple, typename T>
using conditional_tuple_type_append_type = typename conditional_tuple_type_append<Condition,Tuple,T>::type;

/**
 * \internal
 * Given a tuple and a tuple of std::integral_constant\<bool,value\>, make a tuple
 * tuple of types from the TypesTuple where the corresponding value in ConditionsTuple
 * is std::true_type
 *
 * TEST: utility_test.cpp: Utility.filter_tuple_types
 */
template<typename ConditionsTuple, typename TypesTuple>
struct filter_tuple_types;

/**
 * \internal
 * Convenience template alias for filter_tuple_types::type
 *
 * TEST: utility_test.cpp: Utility.filter_tuple_types
 */
template<typename Conditions, typename Types>
using filter_tuple = typename filter_tuple_types<Conditions,Types>::type;

/**
 * \internal
 * Given a parameter pack of bools, count how many are true
 *
 * TEST: utility_test.cpp: Utility.count_trues
 */
template<bool... Bs>
struct count_trues;

/**
 * \internal
 * Convenience variable template for count_trues::value
 *
 * TEST: utility_test.cpp: Utility.count_trues
 */
template<bool... Bs>
constexpr std::size_t count_trues_v = count_trues<Bs...>::value;

/**
 * Given a tuple of std::integral_constant\<bool,value\>, count how many are
 * std::true_type
 *
 * TEST: utility_test.cpp: Utility.count_true_types
 */
template<typename... Bs>
struct count_true_types;

/**
 * \internal
 * Convenience variable template for count_true_types::value
 *
 * TEST: utility_test.cpp: Utility.count_true_types
 */
template<typename... Bs>
constexpr std::size_t count_true_types_v = count_true_types<Bs...>::value;

/**
 * \internal
 * Indicates an invalid index when mapping orignal indexes to filtered tuple indexes
 */
struct invalid_index_type {};

/**
 * \internal
 * A constexpr instance of invalid_index_type
 */
constexpr invalid_index_type invalid_index;

/**
 * \internal
 * For use with filter_tuple. Creates a tuple with each position either mapping to
 * the index in the filtered tuple (std::integral_constant\<std::size_t,index\>
 * or mpirpc::internal::invalid_index_type.
 *
 * For example:
 *      false   false   true    true    false   true
 *      invalid invalid 0       1       invalid 3
 *
 * TEST: utility_test.cpp: Utility.filtered_indexes
 */
template<bool... Included>
struct filtered_indexes;

/**
 * \internal
 * Like filtered_indexes, but takes a std::integer_sequence\<bool,Is...\>
 * or std::tuple\<std::integral_constant\<bool,value\>...\> as a parameter.
 */
template<typename T>
struct filtered_sequence_indexes;

/**
 * \internal
 * Convenience template alias for filtered_indexes::type
 *
 * TEST: utility_test.cpp: Utility.filtered_indexes
 */
template<bool... Included>
using filtered_indexes_type = typename filtered_indexes<Included...>::type;

/**
 * \internal
 * Convenience template alias for filtered_sequence_indexes
 */
template<typename T>
using filtered_sequence_indexes_type = typename filtered_sequence_indexes<T>::type;

/**
 * \internal
 * Map an unfiltered index to a filtered index. This is useful when processing elements
 * of an unfilted tuple with an index sequence and access to the index of a related
 * filtered tuple is needed. Maps to a std::integral_constant\<std::size_t,index>
 * or mpirpc::internal::invalid_index_type type.
 */
template<std::size_t Index, bool... Included>
using filtered_index_type = std::tuple_element_t<Index,filtered_indexes_type<Included...>>;

/**
 * \internal
 * This convenience template variable can be used when it is known that the unfiltered
 * index maps to a valid filtered index.
 */
template<std::size_t Index, bool... Included>
constexpr std::size_t filtered_index_v = filtered_index_type<Index,Included...>::value;

/**
 * \internal
 * Like filtered_indexes, but taking a tuple of std::integral_constant\<bool,value\>
 */
template<typename Included>
struct filtered_true_type_indexes;

/**
 * \internal
 * Like filtered_indexes_type, but taking a tuple of std::integral_constant\<bool,value\>
 */
template<typename Included>
using filtered_true_type_indexes_type = typename filtered_true_type_indexes<Included>::type;

/**
 * \internal
 * Like filtered_index_type, but taking a tuple of std::integral_constant\<bool,value\>
 */
template<std::size_t Index, typename Included>
using filtered_true_type_index_type = std::tuple_element_t<Index,filtered_true_type_indexes_type<Included>>;

/**
 * \internal
 * Like filtered_index_v, but taking a tuple of std::integral_constant\<bool,value\>
 */
template<std::size_t Index, typename Included>
constexpr std::size_t filtered_true_type_index_v = filtered_true_type_index_type<Index,Included>::value;

/**
 * \internal
 * Like filtered_indexes, but taking a tuple of std::integer_sequence\<bool,values...\>
 */
template<typename Included>
struct filtered_bool_sequence_indexes;

/**
 * \internal
 * Like filtered_indexes_type, but taking a tuple of std::integer_sequence\<bool,values...\>
 */
template<typename Included>
using filtered_bool_sequence_indexes_type = typename filtered_bool_sequence_indexes<Included>::type;

/**
 * \internal
 * Like filtered_index_type, but taking a tuple of std::integer_sequence\<bool,values...\>
 */
template<std::size_t Index, typename Included>
using filtered_bool_sequence_index_type = std::tuple_element_t<Index,filtered_bool_sequence_indexes_type<Included>>;

/**
 * \internal
 * Like filtered_index_v, but taking a tuple of std::integer_sequence\<bool,values...\>
 */
template<std::size_t Index, typename Included>
constexpr std::size_t filtered_bool_sequence_index_v = filtered_bool_sequence_index_type<Index,Included>::value;

/**
 * \internal
 * Maps a filtered index to an unfiltered index. This is useful when processing elements
 * of a filtered tuple with an index sequence and access to the index of a related
 * unfiltered tuple is needed. Maps to a tuple of std::integral_constant\<std::size_t,index\>.
 *
 * TEST: utility_test.cpp: Utility.unfiltered_indexes
 */
template<bool... Included>
struct unfiltered_indexes;

/**
 * \internal
 * Convenience template alias for unfiltered::type
 *
 * TEST: utility_test.cpp: Utility.filtered_indexes
 */
template<bool... Included>
using unfiltered_indexes_type = typename unfiltered_indexes<Included...>::type;

/**
 * \internal
 * Map a filtered index to an unfiltered index.
 */
template<std::size_t Index, bool... Included>
using unfiltered_index_type = std::tuple_element_t<Index,unfiltered_indexes_type<Included...>>;

/**
 * \internal
 * Map a filtered index to an unfiltered index value.
 */
template<std::size_t Index, bool... Included>
constexpr std::size_t unfiltered_index_v = unfiltered_index_type<Index,Included...>::value;

/**
 * \internal
 * Like unfiltered_indexes, but taking a tuple of std::integral_constant\<bool,value\>
 */
template<typename Included>
struct unfiltered_true_type_indexes;

/**
 * \internal
 * Like unfiltered_indexes_type, but taking a tuple of std::integral_constant\<bool,value\>
 */
template<typename Included>
using unfiltered_true_type_indexes_type = typename unfiltered_true_type_indexes<Included>::type;

/**
 * \internal
 * Like unfiltered_index_type, but taking a tuple of std::integral_constant\<bool,value\>
 */
template<std::size_t Index, typename Included>
using unfiltered_true_type_index_type = std::tuple_element_t<Index,unfiltered_true_type_indexes_type<Included>>;

/**
 * \internal
 * Like unfiltered_index_v, but taking a tuple of std::integral_constant\<bool,value\>
 */
template<std::size_t Index, typename Included>
constexpr std::size_t unfiltered_true_type_index_v = unfiltered_true_type_index_type<Index,Included>::value;

/**
 * \internal
 * Like unfiltered_indexes, but taking a std::integer_sequence\<bool,values...\>
 */
template<typename Included>
struct unfiltered_bool_sequence_indexes;

/**
 * \internal
 * Like unfiltered_indexes_type, but taking a std::integer_sequence\<bool,values...\>
 */
template<typename Included>
using unfiltered_bool_sequence_indexes_type = typename unfiltered_bool_sequence_indexes<Included>::type;

/**
 * \internal
 * Like unfiltered_index_type, but taking a std::integer_sequence\<bool,values...\>
 */
template<std::size_t Index, typename Included>
using unfiltered_bool_sequence_index_type = std::tuple_element_t<Index,unfiltered_bool_sequence_indexes_type<Included>>;

/**
 * \internal
 * Like unfiltered_index_v, but taking a std::integer_sequence\<bool,values...\>
 */
template<std::size_t Index, typename Included>
constexpr std::size_t unfiltered_bool_sequence_index_v = unfiltered_bool_sequence_index_type<Index,Included>::value;

/*************************************************************************************/
/*************************************************************************************/
/*                                  Implementation                                   */
/*************************************************************************************/
/*************************************************************************************/

/*************************************************************************************/
/*                              mpirpc::internal::apply                              */
/*************************************************************************************/

template <typename F, typename Tuple>
decltype(auto) apply(F&& f, Tuple&& t)
{
    using Indices = std::make_index_sequence<std::tuple_size<std::decay_t<Tuple>>::value>;
    return detail::apply_impl(std::forward<F>(f), std::forward<Tuple>(t), Indices{});
}

template <typename F, class Class, typename Tuple>
decltype(auto) apply(F&& f, Class *c, Tuple&& t)
{
    using Indices = std::make_index_sequence<std::tuple_size<std::decay_t<Tuple>>::value>;
    return detail::apply_impl(std::forward<F>(f), c, std::forward<Tuple>(t), Indices{});
}

/*************************************************************************************/
/*                  mpirpc::internal::extract_integer_pack_element                   */
/*************************************************************************************/

template<std::size_t Pos, typename Int, Int I, Int... Is>
struct extract_integer_pack_element<Pos,Int,I,Is...> : extract_integer_pack_element<Pos-1,Int,Is...> {};

template<typename Int, Int I, Int... Is>
struct extract_integer_pack_element<0,Int,I,Is...> : std::integral_constant<Int,I> {};

/*************************************************************************************/
/*                      mpirpc::internal::integer_sequence_cat                       */
/*************************************************************************************/

template<typename Int, Int... I1s, Int... I2s, typename... Is>
struct integer_sequence_cat<std::integer_sequence<Int, I1s...>, std::integer_sequence<Int, I2s...>,Is...>
    : integer_sequence_cat<std::integer_sequence<Int, I1s..., I2s...>,Is...>
{};

template<typename Int, Int... I1s, Int... I2s>
struct integer_sequence_cat<std::integer_sequence<Int, I1s...>, std::integer_sequence<Int, I2s...>>
{
    using type = std::integer_sequence<Int,I1s...,I2s...>;
};

/*************************************************************************************/
/*                    mpirpc::internal::integer_sequence_prepend                     */
/*************************************************************************************/

template<typename Int, Int I, Int... Is>
struct integer_sequence_prepend<std::integer_sequence<Int,Is...>,I>
{
    using type = std::integer_sequence<Int,I,Is...>;
};

/*************************************************************************************/
/*                     mpirpc::internal::integer_sequence_append                     */
/*************************************************************************************/

template<typename Int, Int I, Int... Is>
struct integer_sequence_append<std::integer_sequence<Int,Is...>,I>
{
    using type = std::integer_sequence<Int,Is...,I>;
};

/*************************************************************************************/
/*              mpirpc::internal::conditional_integer_sequence_prepend               */
/*************************************************************************************/

template<bool Condition, typename Is, typename Is::value_type I>
struct conditional_integer_sequence_prepend
{
    using type = std::conditional_t<Condition,integer_sequence_prepend_type<Is,I>,Is>;
};

/*************************************************************************************/
/*               mpirpc::internal::conditional_integer_sequence_append               */
/*************************************************************************************/

template<bool Condition, typename Is, typename Is::value_type I>
struct conditional_integer_sequence_append
{
    using type = std::conditional_t<Condition,integer_sequence_append_type<Is,I>,Is>;
};

/*************************************************************************************/
/*                         mpirpc::internal::tuple_type_cat                          */
/*************************************************************************************/

template<typename... T1s, typename... T2s, typename... Tuples>
struct tuple_type_cat<std::tuple<T1s...>,std::tuple<T2s...>,Tuples...>
    : tuple_type_cat<std::tuple<T1s...,T2s...>,Tuples...>
{};

template<typename...T1s, typename...T2s>
struct tuple_type_cat<std::tuple<T1s...>,std::tuple<T2s...>>
{
    using type = std::tuple<T1s...,T2s...>;
};

/*************************************************************************************/
/*                       mpirpc::internal::tuple_type_prepend                        */
/*************************************************************************************/

template<typename T, typename... Ts>
struct tuple_type_prepend<T,std::tuple<Ts...>>
{
    using type = std::tuple<T,Ts...>;
};

/*************************************************************************************/
/*                       mpirpc::internal::tuple_type_append                         */
/*************************************************************************************/

template<typename... Ts, typename T>
struct tuple_type_append<std::tuple<Ts...>,T>
{
    using type = std::tuple<Ts...,T>;
};

/*************************************************************************************/
/*                 mpirpc::internal::conditional_tuple_type_prepend                  */
/*************************************************************************************/

template<bool Condition, typename T, typename Tuple>
struct conditional_tuple_type_prepend
{
    using type = std::conditional_t<Condition,tuple_type_prepend_type<T,Tuple>,Tuple>;
};

/*************************************************************************************/
/*                 mpirpc::internal::conditional_tuple_type_append                   */
/*************************************************************************************/

template<bool Condition, typename Tuple, typename T>
struct conditional_tuple_type_append
{
    using type = std::conditional_t<Condition,tuple_type_append_type<Tuple,T>,Tuple>;
};

/*************************************************************************************/
/*                      mpirpc::internal::filter_tuple_types                         */
/*************************************************************************************/

template<typename T, typename... Ts, bool C, bool... Cs>
struct filter_tuple_types<std::tuple<std::integral_constant<bool,C>,std::integral_constant<bool,Cs>...>,std::tuple<T,Ts...>>
    : conditional_tuple_type_prepend<C,T,typename filter_tuple_types<std::integer_sequence<bool,Cs...>,std::tuple<Ts...>>::type>
{};

template<bool C, bool... Cs, typename T, typename... Ts>
struct filter_tuple_types<std::integer_sequence<bool,C,Cs...>, std::tuple<T,Ts...>>
    : conditional_tuple_type_prepend<C,T,typename filter_tuple_types<std::integer_sequence<bool,Cs...>,std::tuple<Ts...>>::type>
{};

template<>
struct filter_tuple_types<std::tuple<>,std::tuple<>>
{
    using type = std::tuple<>;
};

template<>
struct filter_tuple_types<std::integer_sequence<bool>,std::tuple<>>
{
    using type = std::tuple<>;
};

/*************************************************************************************/
/*                          mpirpc::internal::count_trues                            */
/*************************************************************************************/

template<bool B,bool... Bs>
struct count_trues<B,Bs...> : std::integral_constant<std::size_t,std::size_t(B)+count_trues<Bs...>::value> {};

template<bool B>
struct count_trues<B> : std::integral_constant<std::size_t,std::size_t(B)> {};

template<>
struct count_trues<> : std::integral_constant<std::size_t,0> {};

/*************************************************************************************/
/*                        mpirpc::internal::count_true_types                         */
/*************************************************************************************/

template<bool... Bs>
struct count_true_types<std::tuple<std::integral_constant<bool,Bs>...>> : count_trues<Bs...>::type {};

/*************************************************************************************/
/*                        mpirpc::internal::filtered_indexes                         */
/*************************************************************************************/

template<bool... Included>
struct filtered_indexes : detail::filtered_indexes_helper<count_trues_v<Included...>,Included...> {};

/*************************************************************************************/
/*                   mpirpc::internal::filtered_sequence_indexes                     */
/*************************************************************************************/

template<bool... Included>
struct filtered_sequence_indexes<std::integer_sequence<bool,Included...>> : detail::filtered_indexes_helper<count_trues_v<Included...>,Included...> {};

template<bool... Included>
struct filtered_sequence_indexes<std::tuple<std::integral_constant<bool,Included>...>> : detail::filtered_indexes_helper<count_trues_v<Included...>,Included...> {};

/*************************************************************************************/
/*                       mpirpc::internal::unfiltered_indexes                        */
/*************************************************************************************/

template<bool... Included>
struct unfiltered_indexes : detail::unfiltered_indexes_helper<sizeof...(Included),Included...> {};

/*************************************************************************************/
/*                   mpirpc::internal::filtered_true_type_indexes                    */
/*************************************************************************************/

template<bool... Included>
struct filtered_true_type_indexes<std::tuple<std::integral_constant<bool,Included>...>> : filtered_indexes<Included...> {};

/*************************************************************************************/
/*                   mpirpc::internal::filtered_true_type_indexes                    */
/*************************************************************************************/

template<bool... Included>
struct filtered_bool_sequence_indexes<std::integer_sequence<bool,Included...>> : filtered_indexes<Included...> {};

/*************************************************************************************/
/*                  mpirpc::internal::unfiltered_true_type_indexes                   */
/*************************************************************************************/

template<bool... Included>
struct unfiltered_true_type_indexes<std::tuple<std::integral_constant<bool,Included>...>> : unfiltered_indexes<Included...> {};

/*************************************************************************************/
/*                  mpirpc::internal::unfiltered_true_type_indexes                   */
/*************************************************************************************/

template<bool... Included>
struct unfiltered_bool_sequence_indexes<std::integer_sequence<bool,Included...>> : unfiltered_indexes<Included...> {};


} //internal

} //mpirpc

#endif /* MPIRPC__INTERNAL__UTILITY_HPP */

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle;
