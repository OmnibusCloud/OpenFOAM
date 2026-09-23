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

#include "globalOffset.H"

// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

// Backends: template parameter has already been tested as integral
// by the caller

template<class IntType>
template<class IntegralType>
Foam::OffsetRange<IntegralType>
Foam::GlobalOffset<IntType>::calculate_impl
(
    IntegralType localSize,
    int communicator,
    bool parallel
)
{
    OffsetRange<IntegralType> result(localSize);

    if (parallel)
    {
        // Single-item reduction
        Foam::reduceOffset(result, communicator);
    }

    return result;
}


template<class IntType>
template<class IntegralType>
Foam::List<Foam::OffsetRange<IntegralType>>
Foam::GlobalOffset<IntType>::calculateList_impl
(
    const UList<IntegralType>& localSizes,
    int communicator,
    bool parallel
)
{
    const label len = localSizes.size();
    List<OffsetRange<IntegralType>> ranges(len);

    for (label i = 0; i < len; ++i)
    {
        ranges[i] = localSizes[i];
    }

    if (parallel)
    {
        Foam::reduceOffsets(communicator, ranges);
    }

    return ranges;
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

template<class IntType>
Foam::GlobalOffset<IntType>::GlobalOffset(Istream& is)
:
    OffsetRange<IntType>()
{
    is >> *this;
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class IntType>
void Foam::GlobalOffset<IntType>::reduce(int communicator)
{
    // Single-item reduction
    Foam::reduceOffset(*this, communicator);
}


template<class IntType>
template<class IntType2>
void Foam::GlobalOffset<IntType>::inplaceToGlobal
(
    UList<IntType2>& labels
) const
{
    if (const auto beg = this->start(); beg)
    {
        for (auto& val : labels)
        {
            val += beg;
        }
    }
}


template<class IntType>
template<class IntType2>
Foam::List<IntType2> Foam::GlobalOffset<IntType>::toGlobal
(
    const UList<IntType2>& labels
) const
{
    List<IntType2> result(labels);
    inplaceToGlobal(result);
    return result;
}


template<class IntType>
template<class IntType2>
IntType2 Foam::GlobalOffset<IntType>::toLocal(IntType2 i) const
{
    // (error_checking)
    {
        if (!this->contains(i))
        {
            FatalErrorInFunction
                << "Global id:" << i << " not contained in the interval ["
                << this->begin_value() << "," << this->end_value() << "]\n"
                << abort(FatalError);
        }
    }

    return (i - this->start());
}


// ************************************************************************* //
