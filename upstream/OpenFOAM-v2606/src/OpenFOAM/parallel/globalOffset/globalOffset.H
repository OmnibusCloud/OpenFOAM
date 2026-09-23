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

Class
    Foam::GlobalOffset

Description
    A Foam::OffsetRange (start, size, total) tuple with some additional
    parallel functionality. A common use case for OffsetRange would be
    the definition of non-overlapping global addressing.

    OffsetRange is used for the addressing information,
    and GlobalOffset for creating and managing that addressing.

Note
    In contrast to Foam::globalIndex, both GlobalOffset and OffsetRange
    only have their own local addressing information (and total size)
    without additional information about other ranks.

SourceFiles
    globalOffset.txx
    globalOffsetI.H

\*---------------------------------------------------------------------------*/

#ifndef Foam_globalOffset_H
#define Foam_globalOffset_H

#include "OffsetRange.H"
#include "OffsetRangeIO.H"
#include "UPstream.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// Forward Declarations
template<class IntType> class GlobalOffset;

// Common Types
//- A GlobalOffset with label for the addressing
typedef GlobalOffset<label> globalOffset;

/*---------------------------------------------------------------------------*\
                        Class GlobalOffset Declaration
\*---------------------------------------------------------------------------*/

template<class IntType>
class GlobalOffset
:
    public OffsetRange<IntType>
{
    // Private Member Functions

        //- Return a list of globally-consistent start/size/total ranges
        //- based on the given local input sizes.
        template<class IntegralType>
        static OffsetRange<IntegralType> calculate_impl
        (
            IntegralType localSize,
            int communicator,
            bool parallel
        );

        //- Return a list of globally-consistent start/size/total ranges
        //- based on the given local input sizes.
        template<class IntegralType>
        static List<OffsetRange<IntegralType>> calculateList_impl
        (
            const UList<IntegralType>& localSizes,
            int communicator,  //!< communicator
            bool parallel      //!< use parallel comms
        );

public:

    // Generated Methods: copy/move construct, copy/move assignment

    // Constructors

        //- Default construct as (0,0,0)
        inline constexpr GlobalOffset() noexcept;

        //- Construct with specified length, starting at zero (0,len,len).
        //- No communication.
        inline constexpr GlobalOffset(IntType len) noexcept;

        //- Construct from local size, with parallel coordination of
        //- the offsets and total size
        GlobalOffset
        (
            IntType len,
            int communicator,                   //!< communicator
            bool parallel = UPstream::parRun()  //!< use parallel comms
        );

        //- Copy construct from OffsetRange.
        //  Should be left as implicit to allow assignment from
        //  the results of the calculate() method
        inline GlobalOffset(const OffsetRange<IntType>& range) noexcept;

        //- Read construct from start/size/total tuple
        explicit GlobalOffset(Istream& is);


    // Member Functions

    // Access

        //- Same as start() - method name as per globalIndex
        IntType localStart() const noexcept { return this->start(); }

        //- Same as size() - method name as per globalIndex
        IntType localSize() const noexcept { return this->size(); }

        //- Same as total() - method name as per globalIndex
        IntType totalSize() const noexcept { return this->total(); }


    // Edit

        //- Reset from local size with zero offset.
        inline void reset(IntType len) noexcept;

        //- Reset from local size, with parallel coordination of
        //- the offsets and total size
        inline void reset
        (
            IntType len,
            int communicator,                   //!< communicator
            bool parallel = UPstream::parRun()  //!< use parallel comms
        );


    // Parallel Functions

        //- Based on the local input size,
        //- determine the globally-consistent start offset and total size
        void reduce(int communicator = UPstream::worldComm);

        //- Calculate a globally-consistent set of start/size/total
        //- based on the given local input size.
        static OffsetRange<IntType> calculate
        (
            IntType localSize,
            int communicator = UPstream::worldComm,  //!< communicator
            bool parallel = UPstream::parRun()       //!< use parallel comms
        )
        {
            return calculate_impl<IntType>(localSize, communicator, parallel);
        }

        //- Calculate a globally-consistent set of start/size/total
        //- based on the given local input size.
        template
        <
            class Int,
            class = std::enable_if_t<std::is_integral_v<Int>>
        >
        static OffsetRange<Int> calculate
        (
            IntType localSize,
            int communicator = UPstream::worldComm,  //!< communicator
            bool parallel = UPstream::parRun()       //!< use parallel comms
        )
        {
            return calculate_impl<Int>(localSize, communicator, parallel);
        }

        //- Return a list of globally-consistent start/size/total ranges
        //- based on the given local input sizes.
        template
        <
            class Int,
            class = std::enable_if_t<std::is_integral_v<Int>>
        >
        static List<OffsetRange<Int>> calculate
        (
            const UList<Int>& localSizes,
            int communicator = UPstream::worldComm,  //!< communicator
            bool parallel = UPstream::parRun()       //!< use parallel comms
        )
        {
            return calculateList_impl<Int>(localSizes, communicator, parallel);
        }


    // Some functionality as per globalIndex

    // Local queries and renumbering

        //- Test if index is within the range. Same as contains()
        bool isLocal(IntType i) const noexcept
        {
            return this->contains(i);
        }

        //- From local to global index
        template<class IntType2>
        IntType2 toGlobal(IntType2 i) const noexcept
        {
            return (this->start() + i);
        }

        //- Convert a list from local to global index
        template<class IntType2>
        List<IntType2> toGlobal(const UList<IntType2>& labels) const;

        //- Convert a list (inplace) from local to global index
        template<class IntType2>
        void inplaceToGlobal(UList<IntType2>& labels) const;

        //- From global to local
        //  FatalError for an out-of-range value
        template<class IntType2>
        IntType2 toLocal(IntType2 i) const;
};

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#include "globalOffsetI.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#ifdef NoRepository
    #include "globalOffset.txx"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
