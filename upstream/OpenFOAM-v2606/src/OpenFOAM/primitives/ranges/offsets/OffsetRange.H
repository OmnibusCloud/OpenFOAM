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

Class
    Foam::OffsetRange

Description
    A tuple of integers comprising start, size, total.

    This frequently corresponds what is termed a "hyperslab", a "slice"
    or a "selection" for MPI addressing. For this type of use, the
    OffsetRange would represent the local rank addressing. Apart from
    the total size, it would not have information about any other ranks
    (like Foam::globalIndex does).

Note
    Since this range is low-level, the IO operators are defined
    in the separate OffsetRangeIO.H header.

SeeAlso
    Foam::globalIndex
    Foam::GlobalOffset

SourceFiles
    OffsetRangeI.H
    OffsetRangeIO.H

\*---------------------------------------------------------------------------*/

#ifndef Foam_OffsetRange_H
#define Foam_OffsetRange_H

#include "IndexIterator.H"
#include "IntRange.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// Forward Declarations
template<class T> class OffsetRange;
class Istream;
class Ostream;

//- Read from Istream as bracketed (start size total) tuple
template<class T> Istream& operator>>(Istream&, OffsetRange<T>&);

//- Write to Ostream as bracketed (start size total) tuple
template<class T> Ostream& operator<<(Ostream&, const OffsetRange<T>&);

// Common Types
// FUTURE?: offsetRange, labelOffsetRange, ...?

/*---------------------------------------------------------------------------*\
                         Class OffsetRange Declaration
\*---------------------------------------------------------------------------*/

template<class IntType>
class OffsetRange
{
    static_assert(std::is_integral_v<IntType>, "Integral required");

    // Private Data

        //- The start (offset)
        IntType start_;

        //- The size
        IntType size_;

        //- The total size
        IntType total_;


    // Private Member Functions

        //- The value "at" the local index \p idx, with bounds checking.
        //  If index is out-of-bounds, returns the end_value()
        inline IntType at_value(IntType idx) const noexcept;

public:

    // STL type definitions

        //- Type of values contained
        typedef IntType value_type;

        //- The type that can represent the size
        typedef IntType size_type;

        //- Input iterator with const access
        using const_iterator = IndexIterator<IntType>;

        //- Reverse input iterator with const access
        using const_reverse_iterator = ReverseIndexIterator<IntType>;


    // Generated Methods

        //- Copy construct
        OffsetRange(const OffsetRange&) noexcept = default;

        //- Move construct
        OffsetRange(OffsetRange&&) noexcept = default;

        //- Copy assignment
        OffsetRange& operator=(const OffsetRange&) noexcept = default;

        //- Move assignment
        OffsetRange& operator=(OffsetRange&&) noexcept = default;


    // Constructors

        //- Default construct as (0,0,0)
        inline constexpr OffsetRange() noexcept;

        //- Construct with start=0 and specified size, assigns total=size
        inline constexpr OffsetRange(IntType len) noexcept;

        // No construct from (start, size).
        // Unclear if the total should be [size] or [start+size]

        //- Construct from all components, no checks
        inline constexpr OffsetRange
        (
            IntType beg,
            IntType len,
            IntType tot
        ) noexcept;


    // Member Functions

    // Access

        //- True if zero-sized
        constexpr bool empty() const noexcept { return !size_; }

        //- The size
        constexpr IntType size() const noexcept { return size_; }

        //- Non-const access to the size
        constexpr IntType& size() noexcept { return size_; }

        //- The lower value of the range
        constexpr IntType start() const noexcept { return start_; }

        //- Non-const access to start of the range
        constexpr IntType& start() noexcept { return start_; }

        //- The total size
        constexpr IntType total() const noexcept { return total_; }

        //- Non-const access to the total size
        constexpr IntType& total() noexcept { return total_; }

        //- The (start,size) range
        IntRange<IntType> range() const noexcept { return { start_, size_ }; }


    // Edit

        //- Reset to zero
        inline void clear() noexcept;

        //- Reset to the specified size, with start=0 and total=size
        //  Can also use operator= assignment
        inline void reset(IntType len) noexcept;

        // No reset from (start, size).
        // Unclear if the total should be [size] or [start+size]

        //- Reset all components
        inline void reset(IntType beg, IntType len, IntType tot) noexcept;


    // Comparison

        //- Test equality of start/size/total.
        template<class T2>
        inline constexpr bool equals(const OffsetRange<T2>&) const noexcept;

        //- Compare start/size/total in that order.
        //  \return -1,0,+1
        template<class T2>
        inline constexpr int compare(const OffsetRange<T2>&) const noexcept;

        //- Test for equality of begin/size/total values
        template<class T2>
        constexpr bool operator==(const OffsetRange<T2>& other) const noexcept
        {
            return this->equals(other);
        }

        //- Comparison function for sorting, compares the start.
        //  If the start values are equal, also compares the size, etc
        template<class T2>
        constexpr bool operator<(const OffsetRange<T2>& other) const noexcept
        {
            return (this->compare(other) < 0);
        }

        // Derived comparisons
        template<class T2>
        constexpr bool operator!=(const OffsetRange<T2>& other) const noexcept
        {
            return !(*this == other);
        }

        template<class T2>
        constexpr bool operator<=(const OffsetRange<T2>& other) const noexcept
        {
            return !(other < *this);
        }

        template<class T2>
        constexpr bool operator>(const OffsetRange<T2>& other) const noexcept
        {
            return (other < *this);
        }

        template<class T2>
        constexpr bool operator>=(const OffsetRange<T2>& other) const noexcept
        {
            return !(*this < other);
        }


    // Operators

        //- Assign to the specified size, with start=0 and total=size
        void operator=(IntType len) noexcept
        {
            reset(len);
        }

        //- Assign from an OffsetRange with the same or lower representation
        template
        <
            class IntT2,
            class = std::enable_if_t
            <std::is_integral_v<IntT2> && (sizeof(IntT2) <= sizeof(IntType))>
        >
        void operator=(const OffsetRange<IntT2>& rhs) noexcept
        {
            start_ = rhs.start(); size_ = rhs.size(); total_ = rhs.total();
        }


        //- Return const_iterator to a position within the range,
        //- with bounds checking.
        //  \return iterator at the requested position, or end() for
        //      out-of-bounds
        const_iterator at(IntType i) const { return at_value(i); }

        //- Offset dereference, without bounds checking
        inline constexpr IntType operator[](IntType i) const noexcept;

        //- True if the value is between the start and size range.
        //  Behaviour identical to contains() - usable as a predicate
        constexpr bool operator()(IntType i) const noexcept
        {
            return contains(i);
        }


    // Search

        //- True if the value is between the start and size range range.
        inline constexpr bool contains(IntType value) const noexcept;


    // Iterator range values

        //- The value at the beginning of the start/size range - same as start()
        inline constexpr IntType begin_value() const noexcept;

        //- The value 1 beyond the end of the start/size range.
        inline constexpr IntType end_value() const noexcept;

        //- The max value of the start/size range.
        inline constexpr IntType rbegin_value() const noexcept;

        //- The value 1 before the begin of start/size range
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
};

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#include "OffsetRangeI.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
