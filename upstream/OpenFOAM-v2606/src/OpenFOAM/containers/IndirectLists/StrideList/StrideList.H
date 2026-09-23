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

Class
    Foam::StrideList

Description
    A List with indirect stride addressing.

\*---------------------------------------------------------------------------*/

#ifndef Foam_StrideList_H
#define Foam_StrideList_H

#include "IndirectListBase.H"
#include "StrideRange.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                         Class StrideList Declaration
\*---------------------------------------------------------------------------*/

template<class T>
class StrideList
:
    private IndirectListAddressing<StrideRange<label>>,
    public  IndirectListBase<T, StrideRange<label>>
{
    typedef IndirectListAddressing<StrideRange<label>> storage_type;
    typedef IndirectListBase<T, StrideRange<label>> parent_type;

public:

    // Constructors

        //- Copy construct from values list and striding
        StrideList
        (
            const UList<T>& values,
            const StrideRange<label>& addr
        ) noexcept
        :
            storage_type(addr),
            parent_type(values, storage_type::addressing())
        {}


    // Safety

        //- The values list (reference) cannot be an rvalue!
        template<class... Args>
        StrideList(UList<T>&&, Args&&...) = delete;


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
