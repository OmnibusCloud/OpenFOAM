/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2025-2026 OpenCFD Ltd.
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

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

template<class IntType>
inline constexpr Foam::OffsetRange<IntType>::OffsetRange() noexcept
:
    start_(0),
    size_(0),
    total_(0)
{}


template<class IntType>
inline constexpr Foam::OffsetRange<IntType>::OffsetRange(IntType len) noexcept
:
    start_(0),
    size_(len),
    total_(len)
{}


template<class IntType>
inline constexpr Foam::OffsetRange<IntType>::OffsetRange
(
    IntType beg,
    IntType len,
    IntType tot
) noexcept
:
    start_(beg),
    size_(len),
    total_(tot)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class IntType>
inline IntType Foam::OffsetRange<IntType>::at_value(IntType i) const noexcept
{
    return (start_ + ((i < 0 || i > size_) ? size_ : i));
}


template<class IntType>
inline constexpr IntType
Foam::OffsetRange<IntType>::begin_value() const noexcept
{
    return (start_);
}


template<class IntType>
inline constexpr IntType
Foam::OffsetRange<IntType>::end_value() const noexcept
{
    return (start_ + size_);
}


template<class IntType>
inline constexpr IntType
Foam::OffsetRange<IntType>::rbegin_value() const noexcept
{
    return (start_ + (size_ - 1));
}


template<class IntType>
inline constexpr IntType
Foam::OffsetRange<IntType>::rend_value() const noexcept
{
    return (start_ - 1);
}


template<class IntType>
inline void Foam::OffsetRange<IntType>::clear() noexcept
{
    start_ = 0;
    size_ = 0;
    total_ = 0;
}


template<class IntType>
inline void Foam::OffsetRange<IntType>::reset(IntType len) noexcept
{
    start_ = 0;
    size_ = len;
    total_ = len;
}


template<class IntType>
inline void Foam::OffsetRange<IntType>::reset
(
    IntType beg,
    IntType len,
    IntType tot
) noexcept
{
    start_ = beg;
    size_ = len;
    total_ = tot;
}


template<class IntType>
inline constexpr bool
Foam::OffsetRange<IntType>::contains(IntType value) const noexcept
{
    // This check is non-overflowing...
    return (size_ && start_ <= value && (value - start_) < size_);
}


template<class IntType>
template<class IntT2>
inline constexpr bool
Foam::OffsetRange<IntType>::equals(const OffsetRange<IntT2>& b) const noexcept
{
    const auto& a = *this;

    return
    (
        a.start() == b.start()
     && a.size() == b.size()
     && a.total() == b.total()
    );
}


template<class IntType>
template<class IntT2>
inline constexpr int
Foam::OffsetRange<IntType>::compare(const OffsetRange<IntT2>& b) const noexcept
{
    const auto& a = *this;

    if (a.start() < b.start()) return -1;
    if (b.start() < a.start()) return +1;

    if (a.size() < b.size()) return -1;
    if (b.size() < a.size()) return +1;

    if (a.total() < b.total()) return -1;
    if (b.total() < a.total()) return +1;

    return 0;
}


// * * * * * * * * * * * * * * * Member Operators  * * * * * * * * * * * * * //

template<class IntType>
inline constexpr IntType
Foam::OffsetRange<IntType>::operator[](IntType i) const noexcept
{
    return (start_ + i);
}


// ************************************************************************* //
