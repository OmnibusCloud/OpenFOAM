/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2025 OpenCFD Ltd.
    Copyright (C) 2026 Keysight Technologies
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

// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

template<class IntType>
inline IntType
Foam::StrideRange<IntType>::at_value(IntType i) const noexcept
{
    return (start_ + ((i < 0 || i > size_) ? size_ : i) * stride_);
}


template<class IntType>
template<class IntT2>
inline constexpr int Foam::StrideRange<IntType>::compare
(
    const StrideRange<IntT2>& other
) const noexcept
{
    if (this->start() < other.start()) return -1;
    if (other.start() < this->start()) return +1;

    if (this->size() < other.size()) return -1;
    if (other.size() < this->size()) return +1;

    if (this->stride() < other.stride()) return -1;
    if (other.stride() < this->stride()) return +1;

    return 0;
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

template<class IntType>
inline constexpr Foam::StrideRange<IntType>::StrideRange() noexcept
:
    start_(0),
    size_(0),
    stride_(0)
{}


template<class IntType>
inline constexpr Foam::StrideRange<IntType>::StrideRange
(
    IntType beg,
    IntType len,
    IntType stride
) noexcept
:
    start_(beg),
    size_(len),
    stride_(stride)
{}


// template<class IntType>
// inline constexpr Foam::StrideRange<IntType>::StrideRange
// (
//     IntType len,
//     IntType stride
// ) noexcept
// :
//     start_(0),
//     size_(len),
//     stride_(stride)
// {}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class IntType>
inline constexpr IntType
Foam::StrideRange<IntType>::begin_value() const noexcept
{
    return (start_);
}


template<class IntType>
inline constexpr IntType
Foam::StrideRange<IntType>::end_value() const noexcept
{
    return (start_ + size_ * stride_);
}


template<class IntType>
inline constexpr IntType
Foam::StrideRange<IntType>::rbegin_value() const noexcept
{
    return (start_ + (size_-1) * stride_);
}


template<class IntType>
inline constexpr IntType
Foam::StrideRange<IntType>::rend_value() const noexcept
{
    return (start_ - stride_);
}


// * * * * * * * * * * * * * * * Member Operators  * * * * * * * * * * * * * //

template<class IntType>
inline constexpr IntType
Foam::StrideRange<IntType>::operator[](IntType i) const noexcept
{
    return (start_ + i * stride_);
}


template<class IntType>
template<class IntT2>
inline constexpr bool Foam::StrideRange<IntType>::operator==
(
    const StrideRange<IntT2>& other
) const noexcept
{
    return
    (
        this->start() == other.start()
     && this->size() == other.size()
     && this->stride() == other.stride()
    );
}


// ************************************************************************* //
