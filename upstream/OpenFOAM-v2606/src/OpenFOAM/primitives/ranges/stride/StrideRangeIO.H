/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2025 OpenCFD Ltd.
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

Description
    Implementation of IO operators for Foam::StrideRange

\*---------------------------------------------------------------------------*/

#ifndef Foam_StrideRangeIO_H
#define Foam_StrideRangeIO_H

#include "StrideRange.H"
#include "token.H"
#include "Istream.H"
#include "Ostream.H"

// * * * * * * * * * * * * * * * IOstream Operators  * * * * * * * * * * * * //

template<class T>
Foam::Istream& Foam::operator>>
(
    Foam::Istream& is,
    Foam::StrideRange<T>& range
)
{
    is.readBegin("StrideRange");
    is >> range.start() >> range.size() >> range.stride();
    is.readEnd("StrideRange");

    is.check(FUNCTION_NAME);
    return is;
}


template<class T>
Foam::Ostream& Foam::operator<<
(
    Foam::Ostream& os,
    const Foam::StrideRange<T>& range
)
{
    os  << Foam::token::BEGIN_LIST
        << range.start() << Foam::token::SPACE
        << range.size() << Foam::token::SPACE
        << range.stride()
        << Foam::token::END_LIST;

    os.check(FUNCTION_NAME);
    return os;
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
