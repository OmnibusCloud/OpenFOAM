/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2015-2017 OpenFOAM Foundation
    Copyright (C) 2018-2020 OpenCFD Ltd.
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

#include "prghTotalPressureFvPatchScalarField.H"
#include "addToRunTimeSelectionTable.H"
#include "fvPatchFieldMapper.H"
#include "volFields.H"
#include "surfaceFields.H"
#include "gravityMeshObject.H"

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::prghTotalPressureFvPatchScalarField::prghTotalPressureFvPatchScalarField
(
    const fvPatch& p,
    const DimensionedField<scalar, volMesh>& iF
)
:
    parent_bctype(p, iF),
    UName_("U"),
    phiName_("phi"),
    rhoName_("rho"),
    p0_(p.size(), Zero)
{}


Foam::prghTotalPressureFvPatchScalarField::prghTotalPressureFvPatchScalarField
(
    const fvPatch& p,
    const DimensionedField<scalar, volMesh>& iF,
    const dictionary& dict
)
:
    parent_bctype(p, iF, dict, IOobjectOption::NO_READ),
    UName_(dict.getOrDefault<word>("U", "U")),
    phiName_(dict.getOrDefault<word>("phi", "phi")),
    rhoName_(dict.getOrDefault<word>("rho", "rho")),
    p0_("p0", dict, p.size())
{
    if (!this->readValueEntry(dict))
    {
        fvPatchField<scalar>::operator=(p0_);
    }
}


Foam::prghTotalPressureFvPatchScalarField::prghTotalPressureFvPatchScalarField
(
    const this_bctype& ptf,
    const fvPatch& p,
    const DimensionedField<scalar, volMesh>& iF,
    const fvPatchFieldMapper& mapper
)
:
    parent_bctype(ptf, p, iF, mapper),
    UName_(ptf.UName_),
    phiName_(ptf.phiName_),
    rhoName_(ptf.rhoName_),
    p0_(ptf.p0_, mapper)
{}


Foam::prghTotalPressureFvPatchScalarField::prghTotalPressureFvPatchScalarField
(
    const this_bctype& ptf,
    const DimensionedField<scalar, volMesh>& iF
)
:
    parent_bctype(ptf, iF),
    UName_(ptf.UName_),
    phiName_(ptf.phiName_),
    rhoName_(ptf.rhoName_),
    p0_(ptf.p0_)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::prghTotalPressureFvPatchScalarField::autoMap
(
    const fvPatchFieldMapper& m
)
{
    this->parent_bctype::autoMap(m);
    p0_.autoMap(m);
}


void Foam::prghTotalPressureFvPatchScalarField::rmap
(
    const fvPatchScalarField& ptf,
    const labelList& addr
)
{
    this->parent_bctype::rmap(ptf, addr);

    const auto& tiptf = refCast<const this_bctype>(ptf);

    p0_.rmap(tiptf.p0_, addr);
}


void Foam::prghTotalPressureFvPatchScalarField::updateCoeffs()
{
    if (updated())
    {
        return;
    }

    const scalarField& rhop =
        patch().lookupPatchField<volScalarField>(rhoName_);

    const scalarField& phip =
        patch().lookupPatchField<surfaceScalarField>(phiName_);

    const vectorField& Up =
        patch().lookupPatchField<volVectorField>(UName_);

    const uniformDimensionedVectorField& g =
        meshObjects::gravity::New(db().time());

    const uniformDimensionedScalarField& hRef =
        db().lookupObject<uniformDimensionedScalarField>("hRef");

    dimensionedScalar ghRef
    (
        mag(g.value()) > SMALL
      ? g & (cmptMag(g.value())/mag(g.value()))*hRef
      : dimensionedScalar("ghRef", g.dimensions()*dimLength, 0)
    );

    operator==
    (
        p0_
      - 0.5*rhop*(neg(phip))*magSqr(Up)
      - rhop*((g.value() & patch().Cf()) - ghRef.value())
    );

    this->parent_bctype::updateCoeffs();
}


void Foam::prghTotalPressureFvPatchScalarField::write(Ostream& os) const
{
    fvPatchField<scalar>::write(os);
    os.writeEntryIfDifferent<word>("U", "U", UName_);
    os.writeEntryIfDifferent<word>("phi", "phi", phiName_);
    os.writeEntryIfDifferent<word>("rho", "rho", rhoName_);
    p0_.writeEntry("p0", os);
    fvPatchField<scalar>::writeValueEntry(os);
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
    makePatchTypeField
    (
        fvPatchScalarField,
        prghTotalPressureFvPatchScalarField
    );
}

// ************************************************************************* //
