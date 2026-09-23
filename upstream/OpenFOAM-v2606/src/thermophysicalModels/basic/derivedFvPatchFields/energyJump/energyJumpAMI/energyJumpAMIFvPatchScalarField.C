/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2012-2017 OpenFOAM Foundation
    Copyright (C) 2022 OpenCFD Ltd.
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

#include "addToRunTimeSelectionTable.H"
#include "energyJumpAMIFvPatchScalarField.H"
#include "fixedJumpAMIFvPatchFields.H"
#include "basicThermo.H"

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::energyJumpAMIFvPatchScalarField::energyJumpAMIFvPatchScalarField
(
    const fvPatch& p,
    const DimensionedField<scalar, volMesh>& iF
)
:
    parent_bctype(p, iF)
{}


Foam::energyJumpAMIFvPatchScalarField::energyJumpAMIFvPatchScalarField
(
    const this_bctype& ptf,
    const fvPatch& p,
    const DimensionedField<scalar, volMesh>& iF,
    const fvPatchFieldMapper& mapper
)
:
    parent_bctype(ptf, p, iF, mapper)
{}


Foam::energyJumpAMIFvPatchScalarField::energyJumpAMIFvPatchScalarField
(
    const fvPatch& p,
    const DimensionedField<scalar, volMesh>& iF,
    const dictionary& dict
)
:
    parent_bctype(p, iF)
{
    if (!this->readValueEntry(dict))
    {
        evaluate(Pstream::commsTypes::buffered);
    }
}


Foam::energyJumpAMIFvPatchScalarField::energyJumpAMIFvPatchScalarField
(
    const this_bctype& ptf,
    const DimensionedField<scalar, volMesh>& iF
)
:
    parent_bctype(ptf, iF)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::energyJumpAMIFvPatchScalarField::updateCoeffs()
{
    if (this->updated())
    {
        return;
    }

    if (this->cyclicAMIPatch().owner())
    {
        const basicThermo& thermo = basicThermo::lookupThermo(*this);
        label patchID = patch().index();

        const scalarField& pp = thermo.p().boundaryField()[patchID];
        const auto& TbPatch =
            refCast<const fixedJumpAMIFvPatchScalarField>
            (
                thermo.T().boundaryField()[patchID]
            );

        auto& Tbp = const_cast<fixedJumpAMIFvPatchScalarField&>(TbPatch);

        // force update of jump
        Tbp.evaluate(Pstream::commsTypes::buffered);

        const labelUList& faceCells = this->patch().faceCells();

        jump_ =
            thermo.he(pp, Tbp+Tbp.jump(), faceCells)
          - thermo.he(pp, Tbp, faceCells);
    }

    this->parent_bctype::updateCoeffs();
}


void Foam::energyJumpAMIFvPatchScalarField::write(Ostream& os) const
{
    this->parent_bctype::write(os);
    fvPatchField<scalar>::writeValueEntry(os);
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
   makePatchTypeField
   (
       fvPatchScalarField,
       energyJumpAMIFvPatchScalarField
   );
}

// ************************************************************************* //
