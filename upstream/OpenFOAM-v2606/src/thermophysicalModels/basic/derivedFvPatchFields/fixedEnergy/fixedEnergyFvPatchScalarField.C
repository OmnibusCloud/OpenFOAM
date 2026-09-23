/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2012 OpenFOAM Foundation
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

#include "fvPatchFieldMapper.H"
#include "volFields.H"
#include "basicThermo.H"
#include "addToRunTimeSelectionTable.H"
#include "fixedEnergyFvPatchScalarField.H"

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::fixedEnergyFvPatchScalarField::
fixedEnergyFvPatchScalarField
(
    const fvPatch& p,
    const DimensionedField<scalar, volMesh>& iF
)
:
    parent_bctype(p, iF)
{}


Foam::fixedEnergyFvPatchScalarField::
fixedEnergyFvPatchScalarField
(
    const this_bctype& ptf,
    const fvPatch& p,
    const DimensionedField<scalar, volMesh>& iF,
    const fvPatchFieldMapper& mapper
)
:
    parent_bctype(ptf, p, iF, mapper)
{}


Foam::fixedEnergyFvPatchScalarField::
fixedEnergyFvPatchScalarField
(
    const fvPatch& p,
    const DimensionedField<scalar, volMesh>& iF,
    const dictionary& dict
)
:
    parent_bctype(p, iF, dict)
{}


Foam::fixedEnergyFvPatchScalarField::
fixedEnergyFvPatchScalarField
(
    const this_bctype& ptf,
    const DimensionedField<scalar, volMesh>& iF
)
:
    parent_bctype(ptf, iF)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::fixedEnergyFvPatchScalarField::updateCoeffs()
{
    if (updated())
    {
        return;
    }

    const basicThermo& thermo = basicThermo::lookupThermo(*this);
    const label patchi = patch().index();

    const scalarField& pw = thermo.p().boundaryField()[patchi];
    auto& Tw =
        const_cast<fvPatchScalarField&>(thermo.T().boundaryField()[patchi]);

    Tw.evaluate();
    operator==(thermo.he(pw, Tw, patchi));

    this->parent_bctype::updateCoeffs();
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
    makePatchTypeField
    (
        fvPatchScalarField,
        fixedEnergyFvPatchScalarField
    );
}

// ************************************************************************* //
