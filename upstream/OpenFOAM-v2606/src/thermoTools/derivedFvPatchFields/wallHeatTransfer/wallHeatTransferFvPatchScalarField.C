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

#include "wallHeatTransferFvPatchScalarField.H"
#include "addToRunTimeSelectionTable.H"
#include "fvPatchFieldMapper.H"
#include "turbulentFluidThermoModel.H"

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::wallHeatTransferFvPatchScalarField::wallHeatTransferFvPatchScalarField
(
    const fvPatch& p,
    const DimensionedField<scalar, volMesh>& iF
)
:
    parent_bctype(p, iF),
    Tinf_(p.size(), Zero),
    alphaWall_(p.size(), Zero)
{
    refValue() = Zero;
    refGrad() = Zero;
    valueFraction() = 0.0;
}


Foam::wallHeatTransferFvPatchScalarField::wallHeatTransferFvPatchScalarField
(
    const this_bctype& ptf,
    const fvPatch& p,
    const DimensionedField<scalar, volMesh>& iF,
    const fvPatchFieldMapper& mapper
)
:
    parent_bctype(ptf, p, iF, mapper),
    Tinf_(ptf.Tinf_, mapper),
    alphaWall_(ptf.alphaWall_, mapper)
{}


Foam::wallHeatTransferFvPatchScalarField::wallHeatTransferFvPatchScalarField
(
    const fvPatch& p,
    const DimensionedField<scalar, volMesh>& iF,
    const dictionary& dict
)
:
    parent_bctype(p, iF),
    Tinf_("Tinf", dict, p.size()),
    alphaWall_("alphaWall", dict, p.size())
{
    refValue() = Tinf_;
    refGrad() = Zero;
    valueFraction() = 0.0;

    if (!this->readValueEntry(dict))
    {
        evaluate();
    }
}


Foam::wallHeatTransferFvPatchScalarField::wallHeatTransferFvPatchScalarField
(
    const this_bctype& tppsf,
    const DimensionedField<scalar, volMesh>& iF
)
:
    parent_bctype(tppsf, iF),
    Tinf_(tppsf.Tinf_),
    alphaWall_(tppsf.alphaWall_)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::wallHeatTransferFvPatchScalarField::autoMap
(
    const fvPatchFieldMapper& m
)
{
    scalarField::autoMap(m);
    Tinf_.autoMap(m);
    alphaWall_.autoMap(m);
}


void Foam::wallHeatTransferFvPatchScalarField::rmap
(
    const fvPatchScalarField& ptf,
    const labelList& addr
)
{
    this->parent_bctype::rmap(ptf, addr);

    const auto& tiptf = refCast<const this_bctype>(ptf);

    Tinf_.rmap(tiptf.Tinf_, addr);
    alphaWall_.rmap(tiptf.alphaWall_, addr);
}


void Foam::wallHeatTransferFvPatchScalarField::updateCoeffs()
{
    if (updated())
    {
        return;
    }

    const compressible::turbulenceModel& turbModel =
        db().lookupObject<compressible::turbulenceModel>
        (
            IOobject::groupName
            (
                turbulenceModel::propertiesName,
                internalField().group()
            )
        );

    const label patchi = patch().index();

    valueFraction() =
        1.0/
        (
            1.0
          + turbModel.kappaEff(patchi)*patch().deltaCoeffs()/alphaWall_
        );

    this->parent_bctype::updateCoeffs();
}


void Foam::wallHeatTransferFvPatchScalarField::write(Ostream& os) const
{
    fvPatchField<scalar>::write(os);
    Tinf_.writeEntry("Tinf", os);
    alphaWall_.writeEntry("alphaWall", os);
    fvPatchField<scalar>::writeValueEntry(os);
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
    makePatchTypeField
    (
        fvPatchScalarField,
        wallHeatTransferFvPatchScalarField
    );
}

// ************************************************************************* //
