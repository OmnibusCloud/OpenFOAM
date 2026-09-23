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

#include "diagTensor.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

template<>
const char* const
Foam::DiagTensor<Foam::scalar>::vsType::typeName = "diagTensor";


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#undef  defineTraits
#define defineTraits(Type, Prefix)                                            \
                                                                              \
template<>                                                                    \
const char* const                                                             \
Foam::DiagTensor<Type>::vsType::componentNames[] =                            \
{                                                                             \
    "xx", "yy", "zz"                                                          \
};                                                                            \
                                                                              \
template<>                                                                    \
const Foam::DiagTensor<Type>                                                  \
Foam::DiagTensor<Type>::vsType::vsType::zero                                  \
(                                                                             \
    DiagTensor<Type>::uniform(0)                                              \
);                                                                            \
                                                                              \
template<>                                                                    \
const Foam::DiagTensor<Type>                                                  \
Foam::DiagTensor<Type>::vsType::one                                           \
(                                                                             \
    DiagTensor<Type>::uniform(1)                                              \
);                                                                            \
                                                                              \
template<>                                                                    \
const Foam::DiagTensor<Type>                                                  \
Foam::DiagTensor<Type>::vsType::max                                           \
(                                                                             \
    DiagTensor<Type>::uniform(Prefix##VGREAT)                                 \
);                                                                            \
                                                                              \
template<>                                                                    \
const Foam::DiagTensor<Type>                                                  \
Foam::DiagTensor<Type>::vsType::min                                           \
(                                                                             \
    DiagTensor<Type>::uniform(-Prefix##VGREAT)                                \
);                                                                            \
                                                                              \
template<>                                                                    \
const Foam::DiagTensor<Type>                                                  \
Foam::DiagTensor<Type>::vsType::rootMax                                       \
(                                                                             \
    DiagTensor<Type>::uniform(Prefix##ROOTVGREAT)                             \
);                                                                            \
                                                                              \
template<>                                                                    \
const Foam::DiagTensor<Type>                                                  \
Foam::DiagTensor<Type>::vsType::rootMin                                       \
(                                                                             \
    DiagTensor<Type>::uniform(-Prefix##ROOTVGREAT)                            \
);


// defineTraits(float, floatScalar);
// defineTraits(double, doubleScalar);
defineTraits(Foam::scalar, );

#undef defineTraits


// ************************************************************************* //
