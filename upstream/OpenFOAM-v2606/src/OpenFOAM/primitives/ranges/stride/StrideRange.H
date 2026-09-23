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

Class
    Foam::StrideRange

Description
    A tuple of integrals comprising start, size, stride.
    \em Caution: not properly tested for use with negative strides!

SourceFiles
    StrideRangeI.H
    StrideRangeIO.H

\*---------------------------------------------------------------------------*/

#ifndef Foam_StrideRange_H
#define Foam_StrideRange_H

#include "StrideIterator.H"
#include "IntRange.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// Forward Declarations
template<class T> class StrideRange;
class Istream;
class Ostream;

//- Read StrideRange from Istream as (start size stride) tuple, no checks
template<class T> Istream& operator>>(Istream&, StrideRange<T>&);

//- Write StrideRange to Ostream as (start size stride) tuple
template<class T> Ostream& operator<<(Ostream&, const StrideRange<T>&);

/*---------------------------------------------------------------------------*\
                         Class StrideRange Declaration
\*---------------------------------------------------------------------------*/

template<class IntType>
class StrideRange
{
    static_assert(std::is_integral_v<IntType>, "Integral required");

    // Private Data

        //- The start of the interval
        IntType start_;

        //- The length (number of items) for the interval
        IntType size_;

        //- The stride of the interval
        IntType stride_;


    // Private Member Functions

        //- The value "at" the local index \p idx, with bounds checking.
        //  If index is out-of-bounds, returns the end_value()
        inline IntType at_value(IntType idx) const noexcept;

        //- Compare start/size/stride in that order.
        //  \return -1,0,+1
        template<class IntT2>
        inline constexpr int
        compare(const StrideRange<IntT2>& other) const noexcept;


public:

    // STL type definitions

        //- Type of values the range contains
        typedef IntType value_type;

        //- The type that can represent the size of the range
        typedef IntType size_type;

        //- Input iterator with const access
        using const_iterator = StrideIterator<IntType>;

        //- Reverse input iterator with const access
        using const_reverse_iterator = RevStrideIterator<IntType>;


    // Generated Methods: copy/move construct, copy/move assignment

    // Constructors

        //- Default construct an empty stride range (0,0,0)
        inline constexpr StrideRange() noexcept;

        // //- Construct a range starting at zero with the specified length
        // //- and stride (0,len,stride)
        // inline constexpr StrideRange(IntType len, stride) noexcept;

        //- Construct start/size/stride, no checks
        inline constexpr StrideRange
        (
            IntType beg,
            IntType len,
            IntType stride
        ) noexcept;


    // Member Functions

    // Access

        //- True if range is empty (zero-sized)
        constexpr bool empty() const noexcept { return !size_; }

        //- The size of the range (number of items)
        constexpr IntType size() const noexcept { return size_; }

        //- Non-const access to size of the range (number of items)
        constexpr IntType& size() noexcept { return size_; }

        //- The (inclusive) lower value of the range
        constexpr IntType start() const noexcept { return start_; }

        //- Non-const access to start of the range
        constexpr IntType& start() noexcept { return start_; }

        //- The stride of the range
        constexpr IntType stride() const noexcept { return stride_; }

        //- Non-const access to the stride
        constexpr IntType& stride() noexcept { return stride_; }

        //- The (inclusive) lower value of the range,
        //- same as start(), begin_value()
        constexpr IntType min() const noexcept { return start_; }

        //- The (inclusive) upper value of the range,
        //- same as rbegin_value(). Ill-defined for an empty range
        constexpr IntType max() const noexcept
        {
            return (size_ > 0) ? rbegin_value() : start_;
        }


    // Member Operators

        //- Offset dereference, without bounds checking
        //- Return element in the range, without bounds checking
        inline constexpr IntType operator[](IntType i) const noexcept;


    // Comparison

        //- Test for equality of start/size/stride values
        template<class IntT2>
        constexpr bool
        operator==(const StrideRange<IntT2>& other) const noexcept;

        //- Compare lexicographically: compares start, size, stride
        template<class IntT2>
        constexpr bool
        operator<(const StrideRange<IntT2>& other) const noexcept
        {
            return (this->compare(other) < 0);
        }

        // Derived comparisons
        template<class IntT2>
        constexpr bool
        operator!=(const StrideRange<IntT2>& other) const noexcept
        {
            return !(*this == other);
        }

        template<class IntT2>
        constexpr bool
        operator<=(const StrideRange<IntT2>& other) const noexcept
        {
            return !(other < *this);
        }

        template<class IntT2>
        constexpr bool
        operator>(const StrideRange<IntT2>& other) const noexcept
        {
            return (other < *this);
        }

        template<class IntT2>
        constexpr bool
        operator>=(const StrideRange<IntT2>& other) const noexcept
        {
            return !(*this < other);
        }


    // Iterator range values

        //- The value at the beginning of the range - same as start()
        inline constexpr IntType begin_value() const noexcept;

        //- The value that is one stride beyond the end of the range
        inline constexpr IntType end_value() const noexcept;

        //- The max value of the end of start/size range
        inline constexpr IntType rbegin_value() const noexcept;

        //- The value that is one stride before the begin of start/size range
        inline constexpr IntType rend_value() const noexcept;


    // Iteration / Generation

        //- Return a forward values generator
        StrideIndexer<IntType> generator() const noexcept
        {
            return { start_, stride_ };
        }

        //- Return const_iterator to a position within the range,
        //- with bounds checking.
        //  \return iterator at the requested position, or end() for
        //      out-of-bounds
        const_iterator at(IntType i) const
        {
            return { at_value(i), stride_ };
        }

        //- A const_iterator set to the beginning of the range
        const_iterator begin() const noexcept
        {
            return { begin_value(), stride_ };
        }

        //- A const_iterator set to the beginning of the range
        const_iterator cbegin() const noexcept
        {
            return { begin_value(), stride_ };
        }

        //- A const_iterator set to 1 stride beyond the end of the range.
        const_iterator cend() const noexcept
        {
            return { end_value(), stride_ };
        }

        //- A const_iterator set to 1 stride beyond the end of the range.
        const_iterator end() const noexcept
        {
            return { end_value(), stride_ };
        }

        //- A const_reverse_iterator set to 1 stride before the end of range
        const_reverse_iterator rbegin() const noexcept
        {
            return { rbegin_value(), stride_ };
        }

        //- A const_reverse_iterator set to 1 stride before the end of range
        const_reverse_iterator crbegin() const noexcept
        {
            return { rbegin_value(), stride_ };
        }

        //- A const_reverse_iterator set to 1 stride before the begin of range
        const_reverse_iterator rend() const noexcept
        {
            return { rend_value(), stride_ };
        }

        //- A const_reverse_iterator set to 1 stride before the begin of range
        const_reverse_iterator crend() const noexcept
        {
            return { rend_value(), stride_ };
        }
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#include "StrideRangeI.H"

#endif

// ************************************************************************* //
