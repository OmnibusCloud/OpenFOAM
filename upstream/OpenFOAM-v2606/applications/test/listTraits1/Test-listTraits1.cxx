/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
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

Description
    Basic tests for List traits

\*---------------------------------------------------------------------------*/

#include "IOstreams.H"
#include "pTraits.H"
#include "contiguous.H"
#include "DynamicList.H"
#include "FixedList.H"
#include "IndirectList.H"
#include "error.H"
#include "face.H"
#include "triFace.H"

using namespace Foam;

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// Main program:

template<class T>
void printListTraits()
{
    Info<< error::demangle<T>() << " =>";
    Info<< " range:" << Foam::is_range<T>::value;
    Info<< " indirect:" << is_indirectlist_v<T>;
    Info<< " ulist:" << is_ulist_v<T>;
    Info<< " list:" << is_list_v<T>;
    Info<< " fixedlist:" << is_fixedlist_v<T>;
    Info<< " dynlist:" << is_dynamiclist_v<T>;
    Info<< endl;
}

template<class T>
void printListTraits(T&&)
{
    printListTraits<T>();
}

int main()
{
    printListTraits<word>();
    printListTraits<labelRange>();
    printListTraits<triFace>();
    printListTraits<face>();
    printListTraits<IndirectList<face>>();
    printListTraits<DynamicList<word>>();

    // With argument based dispatch:
    printListTraits(face());

    Info<< "End\n" << endl;

    return 0;
}


// ************************************************************************* //
