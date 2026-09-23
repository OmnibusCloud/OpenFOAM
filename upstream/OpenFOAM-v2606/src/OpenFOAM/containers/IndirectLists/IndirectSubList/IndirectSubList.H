/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2020 OpenCFD Ltd.
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
    Foam::IndirectSubList

Description
    Indirect access to a sub-section of a list.

    In many cases, using a Foam::SubList provides a simpler and more
    efficient means of accessing a sub-list.
    With an IndirectSubList, however, can recover the original underlying
    list at any time.

SeeAlso
    Foam::SubList

\*---------------------------------------------------------------------------*/

#ifndef Foam_IndirectSubList_H
#define Foam_IndirectSubList_H

#include "IndirectListBase.H"
#include "labelRange.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                       Class IndirectSubList Declaration
\*---------------------------------------------------------------------------*/

template<class T>
class IndirectSubList
:
    private IndirectListAddressing<labelRange>,
    public  IndirectListBase<T, labelRange>
{
    typedef IndirectListAddressing<labelRange> storage_type;
    typedef IndirectListBase<T, labelRange> parent_type;

public:

    // Constructors

        //- Construct from UList, the entire size
        explicit IndirectSubList(const UList<T>& values)
        :
            storage_type(0, values.size()),
            parent_type(values, storage_type::addressing())
        {}

        //- Construct from values list and range
        IndirectSubList(const UList<T>& values, const labelRange& addr)
        :
            storage_type(addr),
            parent_type(values, storage_type::addressing())
        {}


    // Safety

        //- The values list (reference) cannot be an rvalue!
        template<class... Args>
        IndirectSubList(UList<T>&&, Args&&...) = delete;


    // Member Functions

        //- The list addressing
        using IndirectListAddressing::addressing;


    // Member Operators

        //- Use standard assignment operations
        using parent_type::operator=;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
