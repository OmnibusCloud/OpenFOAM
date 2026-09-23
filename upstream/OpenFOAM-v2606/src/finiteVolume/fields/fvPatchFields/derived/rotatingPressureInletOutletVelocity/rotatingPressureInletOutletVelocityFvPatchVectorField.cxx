/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
    Copyright (C) 2017-2021 OpenCFD Ltd.
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

#include "rotatingPressureInletOutletVelocityFvPatchVectorField.H"
#include "addToRunTimeSelectionTable.H"
#include "volFields.H"
#include "surfaceFields.H"

// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

void Foam::rotatingPressureInletOutletVelocityFvPatchVectorField::
calcTangentialVelocity()
{
    const scalar t = this->db().time().timeOutputValue();
    vector om = omega_->value(t);

    vector axisHat = om/mag(om);
    const vectorField tangentialVelocity
    (
        (-om) ^ (patch().Cf() - axisHat*(axisHat & patch().Cf()))
    );

    const vectorField n(patch().nf());
    refValue() = tangentialVelocity - n*(n & tangentialVelocity);
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::rotatingPressureInletOutletVelocityFvPatchVectorField::
rotatingPressureInletOutletVelocityFvPatchVectorField
(
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF
)
:
    parent_bctype(p, iF),
    omega_()
{}


Foam::rotatingPressureInletOutletVelocityFvPatchVectorField::
rotatingPressureInletOutletVelocityFvPatchVectorField
(
    const this_bctype& ptf,
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF,
    const fvPatchFieldMapper& mapper
)
:
    parent_bctype(ptf, p, iF, mapper),
    omega_(ptf.omega_.clone())
{
    calcTangentialVelocity();
}


Foam::rotatingPressureInletOutletVelocityFvPatchVectorField::
rotatingPressureInletOutletVelocityFvPatchVectorField
(
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF,
    const dictionary& dict
)
:
    parent_bctype(p, iF, dict),
    omega_(Function1<vector>::New("omega", dict, &db()))
{
    calcTangentialVelocity();
}


Foam::rotatingPressureInletOutletVelocityFvPatchVectorField::
rotatingPressureInletOutletVelocityFvPatchVectorField
(
    const this_bctype& rppvf,
    const DimensionedField<vector, volMesh>& iF
)
:
    parent_bctype(rppvf, iF),
    omega_(rppvf.omega_.clone())
{
    calcTangentialVelocity();
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::rotatingPressureInletOutletVelocityFvPatchVectorField::write
(
    Ostream& os
) const
{
    fvPatchField<vector>::write(os);
    os.writeEntry("phi", phiName());
    omega_->writeData(os);
    fvPatchField<vector>::writeValueEntry(os);
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
    makePatchTypeField
    (
        fvPatchVectorField,
        rotatingPressureInletOutletVelocityFvPatchVectorField
    );
}

// ************************************************************************* //
