/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
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

#include "labelSymmTensor.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

template<>
const char* const
Foam::SymmTensor<Foam::label>::vsType::typeName = "labelSymmTensor";


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#undef  defineTraits
#define defineTraits(Type, Prefix)                                            \
                                                                              \
template<>                                                                    \
const char* const                                                             \
Foam::SymmTensor<Type>::vsType::componentNames[] =                            \
{                                                                             \
    "xx", "xy", "xz",                                                         \
          "yy", "yz",                                                         \
                "zz"                                                          \
};                                                                            \
                                                                              \
template<>                                                                    \
const Foam::SymmTensor<Type>                                                  \
Foam::SymmTensor<Type>::vsType::vsType::zero                                  \
(                                                                             \
    SymmTensor<Type>::uniform(0)                                              \
);                                                                            \
                                                                              \
template<>                                                                    \
const Foam::SymmTensor<Type>                                                  \
Foam::SymmTensor<Type>::vsType::one                                           \
(                                                                             \
    SymmTensor<Type>::uniform(1)                                              \
);                                                                            \
                                                                              \
template<>                                                                    \
const Foam::SymmTensor<Type>                                                  \
Foam::SymmTensor<Type>::vsType::max                                           \
(                                                                             \
    SymmTensor<Type>::uniform(Prefix##Max)                                    \
);                                                                            \
                                                                              \
template<>                                                                    \
const Foam::SymmTensor<Type>                                                  \
Foam::SymmTensor<Type>::vsType::min                                           \
(                                                                             \
    SymmTensor<Type>::uniform(-Prefix##Max)                                   \
);                                                                            \
                                                                              \
template<>                                                                    \
const Foam::SymmTensor<Type>                                                  \
Foam::SymmTensor<Type>::vsType::rootMax                                       \
(                                                                             \
    SymmTensor<Type>::uniform(::sqrt(double(Prefix##Max)))                    \
);                                                                            \
                                                                              \
template<>                                                                    \
const Foam::SymmTensor<Type>                                                  \
Foam::SymmTensor<Type>::vsType::rootMin                                       \
(                                                                             \
    SymmTensor<Type>::uniform(-::sqrt(double(Prefix##Max)))                   \
);                                                                            \
                                                                              \
template<>                                                                    \
const Foam::SymmTensor<Type> Foam::SymmTensor<Type>::I                        \
(                                                                             \
    1, 0, 0,                                                                  \
       1, 0,                                                                  \
          1                                                                   \
);


defineTraits(Foam::label, label);

#undef defineTraits


// ************************************************************************* //
