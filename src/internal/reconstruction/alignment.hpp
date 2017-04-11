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

#ifndef MPIRPC__INTERNAL__RECONSTRUCTION__ALIGNMENT_HPP
#define MPIRPC__INTERNAL__RECONSTRUCTION__ALIGNMENT_HPP

#include "detail/alignment.hpp"

namespace mpirpc
{
    
namespace internal
{
    
namespace reconstruction
{

/**
 * \internal
 * Extract the alignment for the construction type. Use the default alignment
 * if alignment data not provided.
 */
template<typename T, typename ArgumentsTuple, typename Alignments>
using construction_type_alignment_type = typename detail::construction_alignments<T,ArgumentsTuple,Alignments>::type_alignment;

/**
 * \internal
 * Extract a tuple of the alignments for the constructor argument types. Use
 * default alignments if alignment data not provided.
 */
template<typename T, typename ArgumentsTuple, typename Alignments>
using construction_arg_alignments_type = typename detail::construction_alignments<T,ArgumentsTuple,Alignments>::arg_alignments;

/**
 * \internal
 * Prepare the full type + constructor type alignment data, using default
 * alignments for unspecified alignments.
 */
template<typename T, typename ArgumentsTuple, typename Alignments>
using construction_alignments_type = typename detail::construction_alignments<T,ArgumentsTuple,Alignments>::alignments;

}

}

}

#endif /* MPIRPC__INTERNAL__RECONSTRUCTION__ALIGNMENT_HPP */
