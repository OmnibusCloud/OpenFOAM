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
    Foam::StrideIndexer

Description
    Indexer or generator for strided integral values.
    The extended base functionality (eg, prev, next) allows its reuse for
    StrideIterator etc.

Class
    Foam::StrideIterator

Description
    A random-access, integer-like, input iterator for integral values with
    a stride.
    If the stride is 1, the Foam::IndexIterator would be a better choice.

Class
    Foam::RevStrideIterator

Description
    A random-access, integer-like, input iterator for integral values with
    a stride that behaves like a reverse iterator.

\*---------------------------------------------------------------------------*/

#ifndef Foam_StrideIterator_H
#define Foam_StrideIterator_H

#include <iterator>
#include <type_traits>

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                        Class StrideIndexer Declaration
\*---------------------------------------------------------------------------*/

template<class IntType>
class StrideIndexer
{
    static_assert(std::is_integral_v<IntType>, "Integral required");

    // Private Data

        //- The current value
        IntType value_;

        //- The indexing stride
        IntType stride_;

public:

    // STL definitions (as per std::iterator)
    typedef IntType value_type;
    typedef IntType difference_type;


    // Constructors

        //- Default construct as zero, stride=1
        constexpr StrideIndexer() noexcept
        :
            value_(0), stride_(1)
        {}

        //- Construct with value zero and specified stride.
        //- No checks (ie, possible to have a 0 stride!)
        constexpr StrideIndexer(value_type stride) noexcept
        :
            value_(0), stride_(stride)
        {}

        //- Construct with specified value and stride.
        //- No checks (ie, possible to have a 0 stride!)
        constexpr StrideIndexer(value_type start, value_type stride) noexcept
        :
            value_(start), stride_(stride)
        {}


    // Member Functions

        //- The current value
        constexpr value_type value() const noexcept { return value_; }

        //- The stride
        constexpr value_type stride() const noexcept { return stride_; }

        //- Return the value offset by \p n strides
        constexpr value_type value(value_type n) const noexcept
        {
            return value_ + (n * stride_);
        }

        //- Decrement the value by the stride
        void prev() noexcept { value_ -= stride_; }

        //- Increment the value by the stride
        void next() noexcept { value_ += stride_; }

        //- Decrease value by \p n strides
        void prev(value_type n) noexcept { value_ -= (n * stride_); }

        //- Increase value by \p n strides
        void next(value_type n) noexcept { value_ += (n * stride_); }


    // Member Operators

        //- Return the value
        constexpr value_type operator*() const noexcept { return value_; }

        //- Apply a postfix increment and return the current value.
        //  This operator definition is required for a generator -
        //  see std::generate()
        value_type operator()() noexcept
        {
            auto curr(value_);
            next();
            return curr;
        }
};


/*---------------------------------------------------------------------------*\
                       Class StrideIterator Declaration
\*---------------------------------------------------------------------------*/

template<class IntType>
class StrideIterator
:
    private StrideIndexer<IntType>
{
public:

    // STL definitions (as per std::iterator)
    typedef std::random_access_iterator_tag iterator_category;
    typedef IntType value_type;
    typedef IntType difference_type;
    typedef const IntType* pointer;
    typedef IntType reference;


    // Constructors

        //- Default construct as zero, stride 1
        constexpr StrideIterator() noexcept
        :
            StrideIndexer<IntType>()
        {}

        // Not yet useful:
        //
        // //- Construct with value zero and specified stride.
        // //- No checks (ie, possible to have a 0 stride!)
        // constexpr StrideIterator(value_type stride) noexcept
        // :
        //     StrideIndexer<IntType>(stride)
        // {}

        //- Construct with specified value and stride.
        //- No checks (ie, possible to have a 0 stride!)
        constexpr StrideIterator(value_type val, value_type stride) noexcept
        :
            StrideIndexer<IntType>(val, stride)
        {}


    // Member Operators

        //- Return the value
        constexpr value_type operator*() const noexcept
        {
            return this->value();
        }

        //- Offset dereference operator
        constexpr value_type operator[](value_type n) const noexcept
        {
            return this->value(n);
        }

        //- Prefix increment
        StrideIterator& operator++() noexcept
        {
            this->next();
            return *this;
        }

        //- Postfix increment
        StrideIterator operator++(int) noexcept
        {
            auto old(*this);
            this->next();
            return old;
        }

        //- Prefix decrement
        StrideIterator& operator--() noexcept
        {
            this->prev();
            return *this;
        }

        //- Postfix decrement
        StrideIterator operator--(int) noexcept
        {
            auto old(*this);
            this->prev();
            return old;
        }

        //- Arbitrary increment
        StrideIterator& operator+=(value_type n) noexcept
        {
            this->next(n);
            return *this;
        }

        //- Arbitrary decrement
        StrideIterator& operator-=(value_type n) noexcept
        {
            this->prev(n);
            return *this;
        }

        //- Return iterator with offset
        constexpr StrideIterator
        operator+(value_type n) const noexcept
        {
            return { this->value(n), this->stride() };
        }

        //- Return iterator with offset
        constexpr StrideIterator
        operator-(value_type n) const noexcept
        {
            return { this->value(-n), this->stride() };
        }

        //- Difference operator
        constexpr value_type
        operator-(const StrideIterator& iter) const noexcept
        {
            return
            (
                this->stride()
              ? ((this->value() - iter.value()) / this->stride())
              : 0
            );
        }


    // Comparison

        //- Test for equality of values (ignores stride)
        constexpr bool
        operator==(const StrideIterator& iter) const noexcept
        {
            return (this->value() == iter.value());
        }

        //- Compare less-than values (ignores stride)
        constexpr bool
        operator<(const StrideIterator& iter) const noexcept
        {
            return (this->value() < iter.value());
        }


    // Derived comparisons

        constexpr bool
        operator!=(const StrideIterator& iter) const noexcept
        {
            return !(*this == iter);
        }

        constexpr bool
        operator<=(const StrideIterator& iter) const noexcept
        {
            return !(iter < *this);
        }

        constexpr bool
        operator>(const StrideIterator& iter) const noexcept
        {
            return (iter < *this);
        }

        constexpr bool
        operator>=(const StrideIterator& iter) const noexcept
        {
            return !(*this < iter);
        }
};


/*---------------------------------------------------------------------------*\
                      Class RevStrideIterator Declaration
\*---------------------------------------------------------------------------*/

template<class IntType>
class RevStrideIterator
:
    private StrideIndexer<IntType>
{
public:

    // STL definitions (as per std::iterator)
    typedef std::random_access_iterator_tag iterator_category;
    typedef IntType value_type;
    typedef IntType difference_type;
    typedef const IntType* pointer;
    typedef IntType reference;


    // Constructors

        //- Default construct as zero, stride 1
        constexpr RevStrideIterator() noexcept
        :
            StrideIndexer<IntType>()
        {}

        //- Construct with specified value and stride.
        //- No checks (ie, possible to have a 0 stride!)
        constexpr RevStrideIterator(value_type val, value_type stride) noexcept
        :
            StrideIndexer<IntType>(val, stride)
        {}


    // Member Operators

        //- Return the value
        constexpr value_type operator*() const noexcept
        {
            return this->value();
        }

        //- Offset dereference operator
        constexpr value_type operator[](value_type n) const noexcept
        {
            return this->value(-n);
        }

        //- Prefix increment
        RevStrideIterator& operator++() noexcept
        {
            this->prev();
            return *this;
        }

        //- Postfix increment
        RevStrideIterator operator++(int) noexcept
        {
            auto old(*this);
            this->prev();
            return old;
        }

        //- Prefix decrement
        RevStrideIterator& operator--() noexcept
        {
            this->next();
            return *this;
        }

        //- Postfix decrement
        RevStrideIterator operator--(int) noexcept
        {
            auto old(*this);
            this->next();
            return old;
        }

        //- Arbitrary increment
        RevStrideIterator& operator+=(value_type n) noexcept
        {
            this->prev(n);
            return *this;
        }

        //- Arbitrary decrement
        RevStrideIterator& operator-=(value_type n) noexcept
        {
            this->next(n);
            return *this;
        }

        //- Return iterator with offset
        constexpr RevStrideIterator
        operator+(value_type n) const noexcept
        {
            return { this->value(-n), this->stride() };
        }

        //- Return iterator with offset
        constexpr RevStrideIterator
        operator-(value_type n) const noexcept
        {
            return { this->value(n), this->stride() };
        }

        //- Difference operator
        constexpr value_type
        operator-(const RevStrideIterator& iter) const noexcept
        {
            return
            (
                this->stride()
              ? ((iter.value() - this->value()) / this->stride())
              : 0
            );
        }


    // Comparison

        //- Test for equality of values (ignore stride)
        constexpr bool
        operator==(const RevStrideIterator& iter) const noexcept
        {
            return (this->value() == iter.value());
        }

        //- Compare less-than values (ignore stride)
        constexpr bool
        operator<(const RevStrideIterator& iter) const noexcept
        {
            return (iter.value() < this->value());
        }


    // Derived comparisons

        constexpr bool
        operator!=(const RevStrideIterator& iter) const noexcept
        {
            return !(*this == iter);
        }

        constexpr bool
        operator<=(const RevStrideIterator& iter) const noexcept
        {
            return !(iter < *this);
        }

        constexpr bool
        operator>(const RevStrideIterator& iter) const noexcept
        {
            return (iter < *this);
        }

        constexpr bool
        operator>=(const RevStrideIterator& iter) const noexcept
        {
            return !(*this < iter);
        }
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

#endif

// ************************************************************************* //
