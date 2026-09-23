/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2020-2025 OpenCFD Ltd.
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
    Foam::IndexIterator

Description
    A random-access, integer-like, input iterator for integral values.

Note
    No \c operator+(value_type, iterator) is defined
    since this provokes misleading resolution errors

Class
    Foam::ReverseIndexIterator

Description
    A random-access, integer-like, input iterator for integral values
    that behaves like a reverse iterator.

Note
    No \c operator+(value_type, iterator) is defined
    since this provokes misleading resolution errors

\*---------------------------------------------------------------------------*/

#ifndef Foam_IndexIterator_H
#define Foam_IndexIterator_H

#include <iterator>
#include <type_traits>

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                        Class IndexIterator Declaration
\*---------------------------------------------------------------------------*/

template<class IntType>
class IndexIterator
{
    static_assert(std::is_integral_v<IntType>, "Integral required");

    // Private Data

        //- The current value
        IntType value_;

public:

    // STL definitions (as per std::iterator)
    typedef std::random_access_iterator_tag iterator_category;
    typedef IntType value_type;
    typedef IntType difference_type;
    typedef const IntType* pointer;
    typedef IntType reference;


    // Constructors

        //- Default construct as zero
        constexpr IndexIterator() noexcept : value_(0) {}

        //- Construct with specified value
        constexpr IndexIterator(value_type val) noexcept : value_(val) {}


    // Member Operators

        //- Return the value
        constexpr value_type operator*() const noexcept { return value_; }

        //- Offset dereference operator
        constexpr value_type operator[](value_type n) const noexcept
        {
            return (value_ + n);
        }

        //- Prefix increment
        IndexIterator& operator++() noexcept
        {
            ++value_;
            return *this;
        }

        //- Postfix increment
        IndexIterator operator++(int) noexcept
        {
            auto old(*this);
            ++value_;
            return old;
        }

        //- Prefix decrement
        IndexIterator& operator--() noexcept
        {
            --value_;
            return *this;
        }

        //- Postfix decrement
        IndexIterator operator--(int) noexcept
        {
            auto old(*this);
            --value_;
            return old;
        }

        //- Arbitrary increment
        IndexIterator& operator+=(value_type n) noexcept
        {
            value_ += n;
            return *this;
        }

        //- Arbitrary decrement
        IndexIterator& operator-=(value_type n) noexcept
        {
            value_ -= n;
            return *this;
        }

        //- Return iterator with offset
        constexpr IndexIterator
        operator+(value_type n) const noexcept { return (value_ + n); }

        //- Return iterator with offset
        IndexIterator
        operator-(value_type n) const noexcept { return (value_ - n); }

        //- Difference operator
        constexpr value_type
        operator-(const IndexIterator& iter) const noexcept
        {
            return (value_ - iter.value_);
        }


    // Comparison

        //- Test for equality of values
        constexpr bool
        operator==(const IndexIterator& iter) const noexcept
        {
            return (value_ == iter.value_);
        }

        //- Compare less-than
        constexpr bool
        operator<(const IndexIterator& iter) const noexcept
        {
            return (value_ < iter.value_);
        }


    // Derived comparisons

        constexpr bool
        operator!=(const IndexIterator& iter) const noexcept
        {
            return !(*this == iter);
        }

        constexpr bool
        operator<=(const IndexIterator& iter) const noexcept
        {
            return !(iter < *this);
        }

        constexpr bool
        operator>(const IndexIterator& iter) const noexcept
        {
            return (iter < *this);
        }

        constexpr bool
        operator>=(const IndexIterator& iter) const noexcept
        {
            return !(*this < iter);
        }
};


/*---------------------------------------------------------------------------*\
                    Class ReverseIndexIterator Declaration
\*---------------------------------------------------------------------------*/

template<class IntType>
class ReverseIndexIterator
{
    //- The current value
    IntType value_;

public:

    // STL definitions (as per std::iterator)
    typedef std::random_access_iterator_tag iterator_category;
    typedef IntType value_type;
    typedef IntType difference_type;
    typedef const IntType* pointer;
    typedef IntType reference;


    // Constructors

        //- Default construct as zero
        constexpr ReverseIndexIterator() noexcept : value_(0) {}

        //- Construct with specified value
        constexpr ReverseIndexIterator(value_type n) noexcept : value_(n) {}


    // Member Operators

        //- Return the value
        constexpr value_type operator*() const noexcept { return value_; }

        //- Offset dereference operator
        constexpr value_type operator[](value_type n) const noexcept
        {
            return (value_ - n);
        }

        //- Prefix increment
        ReverseIndexIterator& operator++() noexcept
        {
            --value_;
            return *this;
        }

        //- Postfix increment
        ReverseIndexIterator operator++(int) noexcept
        {
            auto old(*this);
            --value_;
            return old;
        }

        //- Prefix decrement
        ReverseIndexIterator& operator--() noexcept
        {
            ++value_;
            return *this;
        }

        //- Postfix decrement
        ReverseIndexIterator operator--(int) noexcept
        {
            auto old(*this);
            ++value_;
            return old;
        }

        //- Arbitrary increment
        ReverseIndexIterator& operator+=(value_type n) noexcept
        {
            value_ -= n;
            return *this;
        }

        //- Arbitrary decrement
        ReverseIndexIterator& operator-=(value_type n) noexcept
        {
            value_ += n;
            return *this;
        }

        //- Return iterator with offset
        constexpr ReverseIndexIterator
        operator+(value_type n) const noexcept { return (value_ - n); }

        //- Return iterator with offset
        constexpr ReverseIndexIterator
        operator-(value_type n) const noexcept { return (value_ + n); }

        //- Difference operator
        constexpr value_type
        operator-(const ReverseIndexIterator& iter) const noexcept
        {
            return (iter.value_ - value_);
        }


    // Comparison

        //- Test for equality of values
        constexpr bool
        operator==(const ReverseIndexIterator& iter) const noexcept
        {
            return (value_ == iter.value_);
        }

        //- Compare less-than
        constexpr bool
        operator<(const ReverseIndexIterator& iter) const noexcept
        {
            return (value_ > iter.value_);
        }


    // Derived Comparisons

        constexpr bool
        operator!=(const ReverseIndexIterator& iter) const noexcept
        {
            return !(*this == iter);
        }

        constexpr bool
        operator<=(const ReverseIndexIterator& iter) const noexcept
        {
            return !(iter < *this);
        }

        constexpr bool
        operator>(const ReverseIndexIterator& iter) const noexcept
        {
            return (iter < *this);
        }

        constexpr bool
        operator>=(const ReverseIndexIterator& iter) const noexcept
        {
            return !(*this < iter);
        }
};

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
