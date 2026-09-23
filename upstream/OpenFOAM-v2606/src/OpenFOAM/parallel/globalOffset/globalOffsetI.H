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

\*---------------------------------------------------------------------------*/

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

template<class IntType>
inline constexpr Foam::GlobalOffset<IntType>::GlobalOffset() noexcept
:
    OffsetRange<IntType>()
{}


template<class IntType>
inline constexpr Foam::GlobalOffset<IntType>::GlobalOffset(IntType len) noexcept
:
    OffsetRange<IntType>(len)
{}


template<class IntType>
inline Foam::GlobalOffset<IntType>::GlobalOffset
(
    IntType len,
    int communicator,
    bool parallel
)
:
    OffsetRange<IntType>(len)
{
    if (parallel)
    {
        this->reduce(communicator);
    }
}


template<class IntType>
inline Foam::GlobalOffset<IntType>::GlobalOffset
(
    const OffsetRange<IntType>& range
) noexcept
:
    OffsetRange<IntType>(range)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class IntType>
inline void Foam::GlobalOffset<IntType>::reset(IntType len) noexcept
{
    OffsetRange<IntType>::reset(len);
}


template<class IntType>
inline void Foam::GlobalOffset<IntType>::reset
(
    IntType len,
    int communicator,
    bool parallel
)
{
    OffsetRange<IntType>::reset(len);

    if (parallel)
    {
        this->reduce(communicator);
    }
}


// ************************************************************************* //
