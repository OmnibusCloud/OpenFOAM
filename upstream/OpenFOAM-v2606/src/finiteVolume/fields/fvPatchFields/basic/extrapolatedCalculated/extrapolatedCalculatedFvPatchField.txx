/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2016 OpenFOAM Foundation
    Copyright (C) 2023 OpenCFD Ltd.
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

#include "extrapolatedCalculatedFvPatchField.H"
#include "fvPatchFieldMapper.H"

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

template<class Type>
Foam::extrapolatedCalculatedFvPatchField<Type>::
extrapolatedCalculatedFvPatchField
(
    const fvPatch& p,
    const DimensionedField<Type, volMesh>& iF
)
:
    parent_bctype(p, iF)
{}


template<class Type>
Foam::extrapolatedCalculatedFvPatchField<Type>::
extrapolatedCalculatedFvPatchField
(
    const fvPatch& p,
    const DimensionedField<Type, volMesh>& iF,
    const dictionary& dict
)
:
    parent_bctype(p, iF, dict, IOobjectOption::NO_READ)
{
    fvPatchField<Type>::extrapolateInternal();  // Zero-gradient patch values
}


template<class Type>
Foam::extrapolatedCalculatedFvPatchField<Type>::
extrapolatedCalculatedFvPatchField
(
    const this_bctype& ptf,
    const fvPatch& p,
    const DimensionedField<Type, volMesh>& iF,
    const fvPatchFieldMapper& mapper
)
:
    parent_bctype(ptf, p, iF, mapper)
{}


template<class Type>
Foam::extrapolatedCalculatedFvPatchField<Type>::
extrapolatedCalculatedFvPatchField
(
    const this_bctype& ptf,
    const DimensionedField<Type, volMesh>& iF
)
:
    parent_bctype(ptf, iF)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class Type>
void Foam::extrapolatedCalculatedFvPatchField<Type>::evaluate
(
    const Pstream::commsTypes
)
{
    if (!this->updated())
    {
        this->updateCoeffs();
    }

    fvPatchField<Type>::extrapolateInternal();  // Zero-gradient patch values
    calculatedFvPatchField<Type>::evaluate();
}


// ************************************************************************* //
