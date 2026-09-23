/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2017 OpenFOAM Foundation
    Copyright (C) 2020-2025 OpenCFD Ltd.
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

#include "tensor2D.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

template<>
const char* const
Foam::Tensor2D<Foam::scalar>::vsType::typeName = "tensor2D";


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#undef  defineTraits
#define defineTraits(Type, Prefix)                                            \
                                                                              \
template<>                                                                    \
const char* const                                                             \
Foam::Tensor2D<Type>::vsType::componentNames[] =                              \
{                                                                             \
    "xx", "xy",                                                               \
    "yx", "yy"                                                                \
};                                                                            \
                                                                              \
template<>                                                                    \
const Foam::Tensor2D<Type>                                                    \
Foam::Tensor2D<Type>::vsType::vsType::zero                                    \
(                                                                             \
    Tensor2D<Type>::uniform(0)                                                \
);                                                                            \
                                                                              \
template<>                                                                    \
const Foam::Tensor2D<Type>                                                    \
Foam::Tensor2D<Type>::vsType::one                                             \
(                                                                             \
    Tensor2D<Type>::uniform(1)                                                \
);                                                                            \
                                                                              \
template<>                                                                    \
const Foam::Tensor2D<Type>                                                    \
Foam::Tensor2D<Type>::vsType::max                                             \
(                                                                             \
    Tensor2D<Type>::uniform(Prefix##VGREAT)                                   \
);                                                                            \
                                                                              \
template<>                                                                    \
const Foam::Tensor2D<Type>                                                    \
Foam::Tensor2D<Type>::vsType::min                                             \
(                                                                             \
    Tensor2D<Type>::uniform(-Prefix##VGREAT)                                  \
);                                                                            \
                                                                              \
template<>                                                                    \
const Foam::Tensor2D<Type>                                                    \
Foam::Tensor2D<Type>::vsType::rootMax                                         \
(                                                                             \
    Tensor2D<Type>::uniform(Prefix##ROOTVGREAT)                               \
);                                                                            \
                                                                              \
template<>                                                                    \
const Foam::Tensor2D<Type>                                                    \
Foam::Tensor2D<Type>::vsType::rootMin                                         \
(                                                                             \
    Tensor2D<Type>::uniform(-Prefix##ROOTVGREAT)                              \
);                                                                            \
                                                                              \
template<>                                                                    \
const Foam::Tensor2D<Type> Foam::Tensor2D<Type>::I                            \
(                                                                             \
    1, 0,                                                                     \
    0, 1                                                                      \
);


// defineTraits(float, floatScalar);
// defineTraits(double, doubleScalar);

defineTraits(Foam::scalar, );

#undef defineTraits


// ************************************************************************* //
