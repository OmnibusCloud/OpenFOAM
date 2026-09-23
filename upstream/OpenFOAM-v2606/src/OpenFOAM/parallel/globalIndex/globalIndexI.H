/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
    Copyright (C) 2018-2025 OpenCFD Ltd.
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

\*---------------------------------------------------------------------------*/

#include "ListOps.H"

// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

inline Foam::labelRange
Foam::globalIndex::range_impl(label proci) const noexcept
{
    if
    (
        const auto len = (offsets_.size() - 1);
        (len > 0 && proci >= 0 && proci < len)
    )
    {
        const auto* off = offsets_.cdata();
        return { off[proci], (off[proci+1] - off[proci]) };
    }
    else
    {
        return {};
    }
}


inline Foam::OffsetRange<Foam::label>
Foam::globalIndex::slice_impl(label proci) const noexcept
{
    if
    (
        const auto len = (offsets_.size() - 1);
        (len > 0 && proci >= 0 && proci < len)
    )
    {
        const auto* off = offsets_.cdata();
        return { off[proci], (off[proci+1] - off[proci]), off[len] };
    }
    else
    {
        return {};
    }
}


// * * * * * * * * * * * * * Static Member Functions * * * * * * * * * * * * //

inline void Foam::globalIndex::checkOverflowAndExit
(
    const label idx,
    const label prevOffset,
    const label count
)
{
    if (prevOffset >= 0 && count >= 0)
    {
        if (label next = (prevOffset + count); next < prevOffset)
        {
            reportOverflowAndExit(idx, prevOffset, count);
        }
    }
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

inline Foam::globalIndex::globalIndex
(
    const labelUList& listOffsets
)
{
    if (listOffsets.size() > 1)  // Enforce sizing sanity
    {
        offsets_ = listOffsets;
    }
}


inline Foam::globalIndex::globalIndex
(
    labelList&& listOffsets
)
:
    offsets_(std::move(listOffsets))
{
    if (offsets_.size() == 1)  // Enforce sizing sanity
    {
        offsets_.clear();
    }
}


inline Foam::globalIndex::globalIndex
(
    globalIndex::gatherNone,
    const labelUList& localSizes
)
{
    reset(localSizes);
}


inline Foam::globalIndex::globalIndex
(
    label localSize,
    const label comm,
    const bool parallel
)
{
    reset(localSize, comm, parallel);
}


inline Foam::globalIndex::globalIndex
(
    globalIndex::gatherNone,
    label localSize,
    int communicator  /* unused: (not applicable) */
)
:
    offsets_(2)
{
    offsets_[0] = 0;
    offsets_[1] = localSize;
}


inline Foam::globalIndex::globalIndex
(
    globalIndex::gatherOnly,
    label localSize,
    const label comm
)
{
    // one-sided
    reset(UPstream::listGatherValues(localSize, comm));
}


inline Foam::globalIndex::globalIndex
(
    globalIndex::gatherNonLocal,
    label localSize,
    const label comm
)
{
    // one-sided: non-master sizes only
    if (UPstream::master(comm)) localSize = 0;
    reset(UPstream::listGatherValues(localSize, comm));
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

inline bool Foam::globalIndex::empty() const noexcept
{
    const label len = (offsets_.size() - 1);
    return (len < 1) || (*(offsets_.cdata() + len) == 0);
}


inline bool Foam::globalIndex::single() const noexcept
{
    return (offsets_.size() == 2);
}


inline Foam::label Foam::globalIndex::length() const noexcept
{
    const label len = (offsets_.size() - 1);
    return (len < 1) ? 0 : len;
}


inline Foam::label Foam::globalIndex::span() const noexcept
{
    return (end_value() - begin_value());
}


inline Foam::label Foam::globalIndex::begin_value() const noexcept
{
    return (offsets_.empty() ? 0 : *(offsets_.cdata()));
}


inline Foam::label Foam::globalIndex::end_value() const noexcept
{
    const label len = (offsets_.size() - 1);
    return (len < 1) ? 0 : *(offsets_.cdata() + len);
}


inline bool Foam::globalIndex::contains(const label i) const noexcept
{
    return (begin_value() <= i) && (i < end_value());
}


inline Foam::label Foam::globalIndex::totalSize() const noexcept
{
    return end_value();
}


inline Foam::labelList Foam::globalIndex::sizes() const
{
    return localSizes();
}


inline Foam::label Foam::globalIndex::nProcs() const noexcept
{
    return length();
}


inline Foam::labelRange Foam::globalIndex::allProcs() const noexcept
{
    // Proc 0 -> nProcs
    const label len = (offsets_.size() - 1);
    return (len < 1) ? labelRange() : labelRange(0, len);
}


inline Foam::labelRange Foam::globalIndex::subProcs() const noexcept
{
    // Proc 1 -> nProcs
    const label len = (offsets_.size() - 2);
    return (len < 1) ? labelRange() : labelRange(1, len);
}


inline const Foam::labelList& Foam::globalIndex::offsets() const noexcept
{
    return offsets_;
}


inline Foam::labelList& Foam::globalIndex::offsets() noexcept
{
    return offsets_;
}


inline void Foam::globalIndex::clear()
{
    offsets_.clear();
}


inline const Foam::labelUList Foam::globalIndex::localStarts() const
{
    if (const label len = (offsets_.size() - 1); len > 0)
    {
        return labelList::subList(offsets_, len);
    }
    else
    {
        return labelUList::null();
    }
}


inline Foam::label Foam::globalIndex::localStart(const label proci) const
{
    return offsets_[proci];
}


inline Foam::label Foam::globalIndex::localStart() const
{
    return localStart(UPstream::myProcNo(UPstream::worldComm));
}


inline Foam::label Foam::globalIndex::localEnd(const label proci) const
{
    return offsets_[proci+1];
}


inline Foam::label Foam::globalIndex::localEnd() const
{
    return localEnd(UPstream::myProcNo(UPstream::worldComm));
}


inline Foam::label Foam::globalIndex::localSize(const label proci) const
{
    return offsets_[proci+1] - offsets_[proci];
}


inline Foam::label Foam::globalIndex::localSize() const
{
    return localSize(UPstream::myProcNo(UPstream::worldComm));
}


inline Foam::label Foam::globalIndex::maxSize() const
{
    // Use out-of-range proci to avoid excluding any processor
    return maxNonLocalSize(-1);
}


inline Foam::label Foam::globalIndex::maxNonLocalSize() const
{
    return maxNonLocalSize(UPstream::myProcNo(UPstream::worldComm));
}


inline Foam::labelRange Foam::globalIndex::range(label proci) const noexcept
{
    return range_impl(proci);
}


inline Foam::labelRange Foam::globalIndex::range() const
{
    return range_impl(UPstream::myProcNo(UPstream::worldComm));
}


inline Foam::OffsetRange<Foam::label>
Foam::globalIndex::slice(label proci) const noexcept
{
    return slice_impl(proci);
}


inline Foam::OffsetRange<Foam::label>
Foam::globalIndex::slice() const
{
    return slice_impl(UPstream::myProcNo(UPstream::worldComm));
}


inline bool Foam::globalIndex::isLocal(const label proci, const label i) const
{
    // range contains()
    return (offsets_[proci] <= i) && (i < offsets_[proci+1]);
}


inline bool Foam::globalIndex::isLocal(const label i) const
{
    return isLocal(UPstream::myProcNo(UPstream::worldComm), i);
}


inline Foam::label Foam::globalIndex::toGlobal
(
    const label proci,
    const label i
) const
{
    return i + offsets_[proci];
}


inline Foam::label Foam::globalIndex::toGlobal(const label i) const
{
    return toGlobal(UPstream::myProcNo(UPstream::worldComm), i);
}


inline Foam::labelList Foam::globalIndex::toGlobal
(
    const label proci,
    const labelUList& labels
) const
{
    // Or using std::transform

    //std::transform(labels.begin(), labels.end(), result.begin(),
    //    [=](label id) { return id += off });

    labelList result(labels);
    inplaceToGlobal(proci, result);

    return result;
}


inline Foam::labelList Foam::globalIndex::toGlobal
(
    const labelUList& labels
) const
{
    return toGlobal(UPstream::myProcNo(UPstream::worldComm), labels);
}


inline void Foam::globalIndex::inplaceToGlobal
(
    const label proci,
    labelUList& labels
) const
{
    if (const auto beg = offsets_[proci]; beg)
    {
        for (auto& val : labels)
        {
            val += beg;
        }
    }
}


inline void Foam::globalIndex::inplaceToGlobal(labelUList& labels) const
{
    inplaceToGlobal(UPstream::myProcNo(UPstream::worldComm), labels);
}


inline Foam::label
Foam::globalIndex::toLocal(const label proci, const label i) const
{
    // range !contains()
    if (i < offsets_[proci] || i >= offsets_[proci+1])
    {
        FatalErrorInFunction
            << "Global id:" << i << " does not belong on processor "
            << proci << nl
            << " Offsets:" << offsets_
            << abort(FatalError);
    }
    return (i - offsets_[proci]);
}


inline Foam::label Foam::globalIndex::toLocal(const label i) const
{
    return toLocal(UPstream::myProcNo(UPstream::worldComm), i);
}


inline Foam::label Foam::globalIndex::findProc
(
    const label proci,
    const label i
) const
{
    // Simple checks first
    if
    (
        (proci < 0)                     // Invalid proc
     || (proci+1 > offsets_.size())     // Invalid proc
     || (i < offsets_.front())          // Below the start
     || (i >= offsets_.back())          // Beyond the end
    )
    {
        return -1;
    }

    // Assume that in many cases we have many queries for the local proci,
    // so query that first but also make it the split point for
    // restricting the binary searches

    if (isLocal(proci, i))
    {
        return proci;
    }

    if (i < offsets_[proci])
    {
        // Can restrict search to procs below proci
        const labelList::subList slice(offsets_, proci);
        return findLower(slice, i+1);
    }

    // Search starts at proci+1 (and above)
    return findLower(offsets_, i+1, proci+1);
}


inline Foam::label Foam::globalIndex::findProcAbove
(
    const label proci,
    const label i
) const
{
    // Simple checks first
    if
    (
        (proci < 0)                     // Invalid proci
     || (proci+1 >= offsets_.size())    // Nothing above last proc
     || (i < offsets_[proci+1])         // Exclude proc-local and below
     || (i >= offsets_.back())          // Beyond the end
    )
    {
        return -1;
    }

    // Search starts at proci+1 (and above)
    return findLower(offsets_, i+1, (proci+1));
}


inline Foam::label Foam::globalIndex::findProcBelow
(
    const label proci,
    const label i
) const
{
    if
    (
        (proci <= 0)                    // Nothing below first proc
     || (proci >= offsets_.size())      // Invalid proci
     || (i >= offsets_[proci])          // Exclude proc-local and above
     || (i < offsets_[0])               // Beyond the begin
    )
    {
        return -1;
    }

    // Restrict search to procs below proci
    const labelList::subList slice(offsets_, proci);
    return findLower(slice, i+1);
}


// Note: could split this into failsafe/non-failsafe versions
inline Foam::label Foam::globalIndex::whichProcID
(
    const label proci,
    const label i
) const
{
    label foundProc = findProc(proci, i);

    if (foundProc < 0)
    {
        FatalErrorInFunction
            << "Global id:" << i << " does not belong on any processor." << nl
            << "Offsets:" << offsets_
            << abort(FatalError);
    }

    return foundProc;
}


inline Foam::label Foam::globalIndex::whichProcID
(
    const label i
) const
{
    return whichProcID(UPstream::myProcNo(UPstream::worldComm), i);
}


inline void Foam::globalIndex::reset
(
    globalIndex::gatherNone,
    label localSize,
    int communicator  /* unused: (not applicable) */
)
{
    offsets_.resize_nocopy(2);
    offsets_[0] = 0;
    offsets_[1] = localSize;
}


inline void Foam::globalIndex::reset
(
    globalIndex::gatherOnly,
    label localSize,
    const label comm
)
{
    // one-sided
    reset(UPstream::listGatherValues(localSize, comm));
}


inline void Foam::globalIndex::reset
(
    globalIndex::gatherNonLocal,
    label localSize,
    const label comm
)
{
    // one-sided: non-master sizes only
    if (UPstream::master(comm)) localSize = 0;
    reset(UPstream::listGatherValues(localSize, comm));
}


inline void Foam::globalIndex::reset(const globalIndex& rhs)
{
    if (this == &rhs)
    {
        return;  // Self-assignment is a no-op
    }
    this->offsets_ = rhs.offsets_;
}


// * * * * * * * * * * * * * * * * Iterators * * * * * * * * * * * * * * * * //

inline Foam::globalIndex::const_iterator::
const_iterator
(
    const globalIndex* globalIdx,
    const label i
) noexcept
:
    parent_(globalIdx),
    index_(i)
{}


inline Foam::label Foam::globalIndex::const_iterator::
index() const noexcept
{
    return index_;
}


inline Foam::label Foam::globalIndex::const_iterator::
start() const
{
    return (*parent_).localStart(index_);
}


inline Foam::label Foam::globalIndex::const_iterator::
size() const
{
    return (*parent_).localSize(index_);
}


inline Foam::labelRange Foam::globalIndex::const_iterator::
range() const
{
    return (*parent_).range(index_);
}


inline Foam::labelRange Foam::globalIndex::const_iterator::
operator*() const
{
    return this->range();
}


inline Foam::globalIndex::const_iterator&
Foam::globalIndex::const_iterator::
operator++()
{
    ++index_;
    return *this;
}


inline Foam::globalIndex::const_iterator
Foam::globalIndex::const_iterator::
operator++(int)
{
    const_iterator old(*this);
    ++index_;
    return old;
}


inline Foam::globalIndex::const_iterator&
Foam::globalIndex::const_iterator::
operator--()
{
    --index_;
    return *this;
}


inline Foam::globalIndex::const_iterator
Foam::globalIndex::const_iterator::
operator--(int)
{
    const_iterator old(*this);
    --index_;
    return old;
}


inline bool
Foam::globalIndex::const_iterator::
operator==
(
    const const_iterator& iter
) const noexcept
{
    return (index_ == iter.index_);
}


inline bool
Foam::globalIndex::const_iterator::
operator!=
(
    const const_iterator& iter
) const noexcept
{
    return (index_ != iter.index_);
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

inline Foam::globalIndex::const_iterator
Foam::globalIndex::cbegin() const noexcept
{
    return const_iterator(this);
}


inline const Foam::globalIndex::const_iterator
Foam::globalIndex::cend() const noexcept
{
    return const_iterator(this, this->length());
}


inline Foam::globalIndex::const_iterator
Foam::globalIndex::begin() const noexcept
{
    return const_iterator(this);
}


inline const Foam::globalIndex::const_iterator
Foam::globalIndex::end() const noexcept
{
    return const_iterator(this, this->length());
}


inline Foam::globalIndex::const_iterator
Foam::globalIndex::cbegin(const label i) const noexcept
{
    const label len = this->length();
    return const_iterator(this, (i < 0 ? 0 : len < i ? len : i));
}


inline Foam::globalIndex::const_iterator
Foam::globalIndex::begin(const label i) const noexcept
{
    return this->cbegin(i);
}


// ************************************************************************* //
