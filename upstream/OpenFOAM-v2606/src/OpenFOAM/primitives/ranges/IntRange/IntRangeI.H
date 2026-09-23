/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2020-2026 OpenCFD Ltd.
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
inline constexpr Foam::IntRange<IntType>::IntRange() noexcept
:
    start_(0),
    size_(0)
{}


template<class IntType>
inline constexpr Foam::IntRange<IntType>::IntRange(IntType len) noexcept
:
    start_(0),
    size_(len)
{}


template<class IntType>
inline constexpr Foam::IntRange<IntType>::IntRange
(
    IntType beg,
    IntType len
) noexcept
:
    start_(beg),
    size_(len)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class IntType>
inline IntType Foam::IntRange<IntType>::at_value(IntType i) const noexcept
{
    return (start_ + ((i < 0 || i > size_) ? size_ : i));
}


template<class IntType>
inline constexpr IntType Foam::IntRange<IntType>::begin_value() const noexcept
{
    return (start_);
}


template<class IntType>
inline constexpr IntType Foam::IntRange<IntType>::end_value() const noexcept
{
    return (start_ + size_);
}


template<class IntType>
inline constexpr IntType Foam::IntRange<IntType>::rbegin_value() const noexcept
{
    return (start_ + (size_ - 1));
}


template<class IntType>
inline constexpr IntType Foam::IntRange<IntType>::rend_value() const noexcept
{
    return (start_ - 1);
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class IntType>
inline void Foam::IntRange<IntType>::clear() noexcept
{
    start_ = 0;
    size_  = 0;
}


template<class IntType>
inline void Foam::IntRange<IntType>::reset() noexcept
{
    start_ = 0;
    size_  = 0;
}


template<class IntType>
inline void Foam::IntRange<IntType>::reset(IntType beg, IntType len) noexcept
{
    start_ = beg;
    size_  = len;
}


template<class IntType>
inline void Foam::IntRange<IntType>::setStart(IntType i) noexcept
{
    start_ = i;
}


template<class IntType>
inline void Foam::IntRange<IntType>::setSize(IntType n) noexcept
{
    size_ = n;
}


template<class IntType>
inline void Foam::IntRange<IntType>::resize(IntType n) noexcept
{
    size_ = n;
}


template<class IntType>
inline void Foam::IntRange<IntType>::clampSize() noexcept
{
    if (size_ < 0) size_ = 0;
}


template<class IntType>
inline constexpr bool Foam::IntRange<IntType>::contains(IntType value)
const noexcept
{
    // This check is non-overflowing...
    return (size_ && start_ <= value && (value - start_) < size_);
}


// * * * * * * * * * * * * * * * Member Operators  * * * * * * * * * * * * * //

template<class IntType>
inline constexpr IntType Foam::IntRange<IntType>::
operator[](IntType i) const noexcept
{
    return (start_ + i);
}


template<class IntType>
inline IntType Foam::IntRange<IntType>::
operator++() noexcept
{
    return ++size_;
}


template<class IntType>
inline IntType Foam::IntRange<IntType>::
operator++(int) noexcept
{
    return size_++;
}


template<class IntType>
inline IntType Foam::IntRange<IntType>::
operator--() noexcept
{
    if (size_ > 0) --size_;  // clamp: no negative sizes
    return size_;
}


template<class IntType>
inline IntType Foam::IntRange<IntType>::
operator--(int) noexcept
{
    auto old(size_);
    if (size_ > 0) --size_;  // clamp: no negative sizes
    return old;
}


template<class IntType>
inline IntType Foam::IntRange<IntType>::
operator+=(IntType n) noexcept
{
    size_ += n;
    return size_;
}


template<class IntType>
inline IntType Foam::IntRange<IntType>::
operator-=(IntType n) noexcept
{
    size_ -= n;
    if (size_ <= 0) size_ = 0;  // clamp: no negative sizes
    return size_;
}


// ************************************************************************* //
