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

Class
    Foam::IntRange

Description
    An interval of (signed) integers defined by a start and a size.

Note
    Since this range is low-level, the IO operators are defined
    in the separate IntRangeIO.H header.

SourceFiles
    IntRangeI.H
    IntRangeIO.H

\*---------------------------------------------------------------------------*/

#ifndef Foam_IntRange_H
#define Foam_IntRange_H

#include "IndexIterator.H"
#include "labelFwd.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// Forward Declarations
template<class T> class IntRange;
class Istream;
class Ostream;

//- Read from Istream as bracketed (start size) tuple
template<class T> Istream& operator>>(Istream&, IntRange<T>&);

//- Write to Ostream as bracketed (start size) tuple
template<class T> Ostream& operator<<(Ostream&, const IntRange<T>&);

/*---------------------------------------------------------------------------*\
                          Class IntRange Declaration
\*---------------------------------------------------------------------------*/

template<class IntType>
class IntRange
{
    static_assert(std::is_integral_v<IntType>, "Integral required");

    // Private Data

        //- The start of the interval
        IntType start_;

        //- The length of the interval
        IntType size_;


    // Private Member Functions

        //- The value "at" the local index \p idx, with bounds checking.
        //  If index is out-of-bounds, returns the end_value()
        inline IntType at_value(IntType idx) const noexcept;

public:

    // STL type definitions

        //- Type of values the range contains
        typedef IntType value_type;

        //- The type that can represent the size of the range
        typedef IntType size_type;

        //- Input iterator with const access
        using const_iterator = IndexIterator<IntType>;

        //- Reverse input iterator with const access
        using const_reverse_iterator = ReverseIndexIterator<IntType>;


    // Public Classes

        //- Unary predicate for greater than 0 (int values)
        struct gt0
        {
            template<class Int>
            constexpr bool operator()(Int val) const noexcept
            {
                return (val > 0);
            }
        };

        //- Unary predicate for less than 0 (int values)
        struct lt0
        {
            template<class Int>
            constexpr bool operator()(Int val) const noexcept
            {
                return (val < 0);
            }
        };

        //- Unary predicate for greater-equal 0 (int values)
        struct ge0
        {
            template<class Int>
            constexpr bool operator()(Int val) const noexcept
            {
                return (val >= 0);
            }
        };

        //- Unary predicate for less-equal 0 (int values)
        struct le0
        {
            template<class Int>
            constexpr bool operator()(Int val) const noexcept
            {
                return (val <= 0);
            }
        };


    // Generated Methods: copy/move construct, copy/move assignment


    // Constructors

        //- Default construct an empty range (0,0)
        inline constexpr IntRange() noexcept;

        //- Construct a range with specified length, starting at zero (0,len)
        inline explicit constexpr IntRange(IntType len) noexcept;

        //- Construct a range from start/length, no checks
        inline constexpr IntRange(IntType beg, IntType len) noexcept;


    // Member Functions

    // Access

        //- True if range is empty (zero-sized)
        constexpr bool empty() const noexcept { return !size_; }

        //- True if range has size greater than zero
        constexpr bool good() const noexcept { return (size_ > 0); }

        //- The size of the range
        constexpr IntType size() const noexcept { return size_; }

        //- Non-const access to size of the range
        constexpr IntType& size() noexcept { return size_; }

        //- The (inclusive) lower value of the range
        constexpr IntType start() const noexcept { return start_; }

        //- Non-const access to start of the range
        constexpr IntType& start() noexcept { return start_; }

        //- The (inclusive) lower value of the range,
        //- same as start(), begin_value()
        constexpr IntType min() const noexcept { return start_; }

        //- The (inclusive) upper value of the range,
        //- same as rbegin_value(). Ill-defined for an empty range
        constexpr IntType max() const noexcept { return (start_ + (size_-1)); }


    // Edit

        //- Reset to (0,0)
        inline void clear() noexcept;

        //- Reset to (0,0)
        inline void reset() noexcept;

        //- Reset start and length, no checks
        inline void reset(IntType beg, IntType len) noexcept;

        //- Set the start position, no checks
        inline void setStart(IntType i) noexcept;

        //- Change the size, no checks. Identical to resize()
        inline void setSize(IntType n) noexcept;

        //- Change the size, no checks. Identical to setSize()
        inline void resize(IntType n) noexcept;

        //- Enforce non-negative size
        inline void clampSize() noexcept;


    // Search

        //- True if the (global) value is within the range
        inline constexpr bool contains(IntType value) const noexcept;


    // Member Operators

        //- Return const_iterator to a position within the range,
        //- with bounds checking.
        //  \return iterator at the requested position, or end() for
        //      out-of-bounds
        const_iterator at(IntType i) const { return at_value(i); }

        //- Offset dereference, without bounds checking
        inline constexpr IntType operator[](IntType i) const noexcept;

        //- True if the global value is located within the range.
        //  Behaviour identical to contains() - usable as a predicate
        constexpr bool operator()(IntType i) const noexcept
        {
            return contains(i);
        }

        //- Increase the size by 1.
        inline IntType operator++() noexcept;
        inline IntType operator++(int) noexcept;

        //- Increase the size by n.
        inline IntType operator+=(IntType n) noexcept;

        //- Decrease the size by 1, but never below 0.
        inline IntType operator--() noexcept;
        inline IntType operator--(int) noexcept;

        //- Decrease the size by n, but never below 0.
        inline IntType operator-=(IntType n) noexcept;

        //- True if range has size greater than zero. Same as good()
        explicit operator bool() const noexcept { return (size_ > 0); }


    // Comparison

        //- Test for equality of start/size values
        template<class IntT2>
        constexpr bool operator==(const IntRange<IntT2>& other) const noexcept
        {
            return (start() == other.start() && size() == other.size());
        }

        //- Compare lexicographically: compares start first, then size
        template<class IntT2>
        constexpr bool operator<(const IntRange<IntT2>& other) const noexcept
        {
            return
            (
                (start() < other.start())
             || (start() == other.start() && size() < other.size())
            );
        }

        // Derived comparisons
        template<class IntT2>
        constexpr bool operator!=(const IntRange<IntT2>& other) const noexcept
        {
            return !(*this == other);
        }

        template<class IntT2>
        constexpr bool operator<=(const IntRange<IntT2>& other) const noexcept
        {
            return !(other < *this);
        }

        template<class IntT2>
        constexpr bool operator>(const IntRange<IntT2>& other) const noexcept
        {
            return (other < *this);
        }

        template<class IntT2>
        constexpr bool operator>=(const IntRange<IntT2>& other) const noexcept
        {
            return !(*this < other);
        }


    // Iterator ranges

        //- The value at the beginning of the range - same as min(), start()
        inline constexpr IntType begin_value() const noexcept;

        //- The value 1 beyond the end of the range.
        inline constexpr IntType end_value() const noexcept;

        //- The max value of the range.
        inline constexpr IntType rbegin_value() const noexcept;

        //- The value 1 before the begin of range
        inline constexpr IntType rend_value() const noexcept;


    // Bidirectional input iterators (const)

        //- A const_iterator set to the beginning of the range
        const_iterator begin() const noexcept { return begin_value(); }

        //- A const_iterator set to the beginning of the range
        const_iterator cbegin() const noexcept { return begin_value(); }

        //- A const_iterator set to 1 beyond the end of the range.
        const_iterator cend() const noexcept { return end_value(); }

        //- A const_iterator set to 1 beyond the end of the range.
        const_iterator end() const noexcept { return end_value(); }


    // Bidirectional reverse input iterators (const)

        //- A const_reverse_iterator set to 1 before the end of range
        const_reverse_iterator
        rbegin() const noexcept { return rbegin_value(); }

        //- A const_reverse_iterator set to 1 before the end of range
        const_reverse_iterator
        crbegin() const noexcept { return rbegin_value(); }

        //- A const_reverse_iterator set to 1 before the begin of range
        const_reverse_iterator rend() const noexcept { return rend_value(); }

        //- A const_reverse_iterator set to 1 before the begin of range
        const_reverse_iterator crend() const noexcept { return rend_value(); }


    // Housekeeping

        //- True if the (global) value is within the range
        bool found(IntType val) const noexcept { return contains(val); }
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#include "IntRangeI.H"

#endif

// ************************************************************************* //
