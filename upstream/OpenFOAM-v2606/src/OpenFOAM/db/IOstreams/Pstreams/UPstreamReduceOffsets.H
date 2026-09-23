/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2025 Mark Olesen
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

Note
    Creating globally consistent offsets and total uses the following
    process:

    - apply MPI_Exscan(MPI_SUM) using the rank-local size to produce
      the offsets. Although MPI_Exscan() does not specify what value
      rank0 will have, our own mpiExscan_sum() does and assigns 0
      for rank0.
    - determine the total size as the offset + local size.
      This value is only correct on the (nProcs-1) rank.
    - broad cast the total size from the (nProcs-1) rank to all ranks.

    The reduceOffsets() version uses exactly the same process, except
    with a work array to bundle/unbundle values and use the same two
    MPI calls.

\*---------------------------------------------------------------------------*/

#ifndef Foam_UPstreamReduceOffsets_H
#define Foam_UPstreamReduceOffsets_H

#include "OffsetRange.H"
#include "UPstream.H"
#include <array>

// * * * * * * * * * * * * * * * * * Details * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace PstreamDetail
{

// Implementation for Foam::reduceOffset
//
// Reduction of OffsetRange for a globally consistent offset/total
// based on the local size
template<class IntType>
void reduce_offsetRange
(
    Foam::OffsetRange<IntType>& range,
    const int communicator  // The parallel communicator
)
{
    if (UPstream::is_parallel(communicator))
    {
        // Exscan (sum) yields the offsets, assigns 0 for rank=0
        IntType work = range.size();
        UPstream::mpiExscan_sum(&work, 1, communicator);

        // For the truly paranoid:
        // if (UPstream::master(communicator)) work = 0;

        range.start() = work;   // Copy out offset from work
        work += range.size();   // Update work as total == (start + size)

        // The rank=(nProcs-1) knows the total - broadcast to others
        const auto root = (UPstream::nProcs(communicator)-1);
        UPstream::broadcast(&work, 1, communicator, root);

        range.total() = work;
    }
}


// Implementation for Foam::reduceOffsets
//
// Equivalent to Foam::reduceOffset()
// but bundles values and performs operations on multiple values,
// which avoids calling MPI repeatedly
template
<
    class IntType,       // Must match OffsetRange::value_type
    std::size_t... Is,
    class... OffsetRanges
>
void reduce_offsetRanges
(
    const int communicator,       // The parallel communicator
    std::index_sequence<Is...>,   // Indices into items
    OffsetRanges&... items
)
{
    if (UPstream::is_parallel(communicator))
    {
        // Like Foam::reduceOffset()
        // but handling multiple items at once for lower communication

        // Pack all sizes into the work buffer
        std::array<IntType, sizeof...(items)> work{ (items.size())... };

        // Exscan (sum) yields the offsets, assigns 0 for rank=0
        UPstream::mpiExscan_sum(work.data(), work.size(), communicator);

        // For the truly paranoid:
        // if (UPstream::master(communicator)) work.fill(0);

        // The work buffer now contains the offsets, copy back to starts
        ((items.start() = work[Is]), ...);

        // The rank=(nProcs-1) knows the total - broadcast to others
        const auto root = (UPstream::nProcs(communicator)-1);
        if (root == UPstream::myProcNo(communicator))
        {
            // Update work buffer as total == (start + size)
            ((work[Is] += items.size()), ...);
        }
        UPstream::broadcast(work.data(), work.size(), communicator, root);

        // The work buffer now contains the totals, copy back to total
        ((items.total() = work[Is]), ...);
    }
}

} // End namespace PstreamDetail
} // End namespace Foam


// * * * * * * * * * * * * * * * Global Reduction  * * * * * * * * * * * * * //

namespace Foam
{

//- Parallel reduction of OffsetRange (eg, GlobalOffset) on its size value to
//- yield the globally-consistent offset and the total size across all ranks.
//
//  \note Only the OffsetRange::size() member is used for this reduction.
//  It is assumed that the combined results will always start with an offset
//  of zero on the first rank, and that ranges represent contiguous addressing
//  across all ranks.
template<class IntType>
void reduceOffset
(
    Foam::OffsetRange<IntType>& range,
    //! The parallel communicator
    const int communicator = UPstream::worldComm
)
{
    // Single-item reduction
    Foam::PstreamDetail::reduce_offsetRange<IntType>
    (
        range,
        communicator
    );
}


//- Parallel reduction of multiple OffsetRange (eg, GlobalOffset) items.
//  To avoid calling MPI multiple times, it packs/unpacks values into
//  a local work array (compile-time).
//
//  \note Only the OffsetRange::size() member is used for this reduction.
//  It is assumed that the combined results will always start with an offset
//  of zero on the first rank, and that ranges represent contiguous addressing
//  across all ranks.
//
//  \note OffsetRange is already restricted to integral types,
//  but this wrapper also requires that the parameters have the same
//  representation (ie, cannot mix int32/int64, signed/unsigned etc)
template
<
    class OffsetRangeT,
    class... Rest,
    class = std::enable_if_t
    <
    (
        std::is_integral_v<typename OffsetRangeT::value_type>
     && std::is_base_of_v
        <Foam::OffsetRange<typename OffsetRangeT::value_type>, OffsetRangeT>
     && (std::is_base_of_v
        <Foam::OffsetRange<typename OffsetRangeT::value_type>, Rest> && ...)
    )>
>
void reduceOffsets
(
    //! The parallel communicator
    const int communicator,
    //! The first offset-range in the reduction
    OffsetRangeT& first,
    //! Any additional ranges
    Rest&... rest
)
{
    using IntType = typename OffsetRangeT::value_type;

    if constexpr (sizeof...(rest) == 0)
    {
        // Single-item reduction
        Foam::PstreamDetail::reduce_offsetRange<IntType>
        (
            first,
            communicator
        );
    }
    else
    {
        // Reduce multiple items, use pack/unpack
        Foam::PstreamDetail::reduce_offsetRanges<IntType>
        (
            communicator,
            std::make_index_sequence<1 + sizeof...(rest)>{},
            first,
            rest...
        );
    }
}


//- Parallel reduction of multiple OffsetRange (eg, GlobalOffset) items.
//  To avoid calling MPI multiple times, it packs/unpacks values into
//  a local work array.
//
//  \note Only the OffsetRange::size() member is used for this reduction.
//  It is assumed that the combined results will always start with an offset
//  of zero on the first rank, and that ranges represent contiguous addressing
//  across all ranks.
template
<
    class OffsetRangeT,
    class = std::enable_if_t
    <
    (
        std::is_integral_v<typename OffsetRangeT::value_type>
     && std::is_base_of_v
        <Foam::OffsetRange<typename OffsetRangeT::value_type>, OffsetRangeT>
    )>
>
void reduceOffsets
(
    //! The parallel communicator
    const int communicator,
    //! The offset-ranges to reduce. List length must be parallel-consistent.
    Foam::UList<OffsetRangeT>& ranges
)
{
    const auto len = ranges.size();

    if (ranges.empty() || !UPstream::is_parallel(communicator))
    {
        // nothing to do
    }
    else if (len == 1)
    {
        // Single-item reduction
        Foam::reduceOffset(ranges[0], communicator);
    }
    else
    {
        using IntType = typename OffsetRangeT::value_type;

        // Like Foam::reduceOffset()
        // but handling multiple items at once for lower communication
        List<IntType> work(len);

        // Pack all sizes into the work buffer
        for (label i = 0; i < len; ++i)
        {
            work[i] = ranges[i].size();
        }

        // Exscan (sum) yields the offsets, assigns 0 for rank=0
        UPstream::mpiExscan_sum(work.data(), work.size(), communicator);

        // For the truly paranoid:
        // if (UPstream::master(communicator)) work = 0;

        // The work buffer now contains the offsets, copy back to starts
        // and update work buffer as total == (start + size)
        for (label i = 0; i < len; ++i)
        {
            ranges[i].start() = work[i];
            work[i] += ranges[i].size();
        }

        // The rank=(nProcs-1) knows the total - broadcast to others
        const auto root = (UPstream::nProcs(communicator)-1);
        UPstream::broadcast(work.data(), work.size(), communicator, root);

        // The work buffer now contains the totals, copy back to total
        for (label i = 0; i < len; ++i)
        {
            ranges[i].total() = work[i];
        }
    }
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
