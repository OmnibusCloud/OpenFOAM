/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2016 OpenFOAM Foundation
    Copyright (C) 2017-2020 OpenCFD Ltd.
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

#include "noSlipFvPatchVectorField.H"
#include "addToRunTimeSelectionTable.H"

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::noSlipFvPatchVectorField::noSlipFvPatchVectorField
(
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF
)
:
    // Field is zero
    parent_bctype(p, iF, Zero)
{}


Foam::noSlipFvPatchVectorField::noSlipFvPatchVectorField
(
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF,
    const dictionary& dict
)
:
    // Field is zero
    parent_bctype(p, iF, Zero)
{
    fvPatchFieldBase::readDict(dict);
}


Foam::noSlipFvPatchVectorField::noSlipFvPatchVectorField
(
    const this_bctype& pfld,
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF,
    const fvPatchFieldMapper&
)
:
    // Field is zero. No mapping
    parent_bctype(pfld, p, iF, Zero)
{}


Foam::noSlipFvPatchVectorField::noSlipFvPatchVectorField
(
    const this_bctype& pfld,
    const DimensionedField<vector, volMesh>& iF
)
:
    // Field is zero
    parent_bctype(pfld, pfld.patch(), iF, Zero)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::noSlipFvPatchVectorField::write(Ostream& os) const
{
    fvPatchField<vector>::write(os);
    // Without writeValueEntry() since the value == zero
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
    makePatchTypeField
    (
        fvPatchVectorField,
        noSlipFvPatchVectorField
    );
}

// ************************************************************************* //
