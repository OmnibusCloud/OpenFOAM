/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
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

#include "pressureInletOutletVelocityFvPatchVectorField.H"
#include "addToRunTimeSelectionTable.H"
#include "volFields.H"
#include "surfaceFields.H"


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::pressureInletOutletVelocityFvPatchVectorField::
pressureInletOutletVelocityFvPatchVectorField
(
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF
)
:
    parent_bctype(p, iF),
    phiName_("phi")
{
    refValue() = Zero;
    refGrad() = Zero;
    valueFraction() = Zero;
}


Foam::pressureInletOutletVelocityFvPatchVectorField::
pressureInletOutletVelocityFvPatchVectorField
(
    const this_bctype& ptf,
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF,
    const fvPatchFieldMapper& mapper
)
:
    parent_bctype(ptf, p, iF, mapper),
    phiName_(ptf.phiName_)
{
    if (ptf.tangentialVelocity_.size())
    {
        tangentialVelocity_ = mapper(ptf.tangentialVelocity_);
    }
}


Foam::pressureInletOutletVelocityFvPatchVectorField::
pressureInletOutletVelocityFvPatchVectorField
(
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF,
    const dictionary& dict
)
:
    parent_bctype(p, iF),
    phiName_(dict.getOrDefault<word>("phi", "phi"))
{
    fvPatchFieldBase::readDict(dict);
    this->readValueEntry(dict, IOobjectOption::MUST_READ);

    if (dict.found("tangentialVelocity"))
    {
        setTangentialVelocity
        (
            vectorField("tangentialVelocity", dict, p.size())
        );
    }
    else
    {
        refValue() = Zero;
    }

    refGrad() = Zero;
    valueFraction() = Zero;
}


Foam::pressureInletOutletVelocityFvPatchVectorField::
pressureInletOutletVelocityFvPatchVectorField
(
    const this_bctype& pivpvf,
    const DimensionedField<vector, volMesh>& iF
)
:
    parent_bctype(pivpvf, iF),
    phiName_(pivpvf.phiName_),
    tangentialVelocity_(pivpvf.tangentialVelocity_)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::pressureInletOutletVelocityFvPatchVectorField::
setTangentialVelocity(const vectorField& tangentialVelocity)
{
    tangentialVelocity_ = tangentialVelocity;
    const vectorField n(patch().nf());
    refValue() = tangentialVelocity_ - n*(n & tangentialVelocity_);
}


void Foam::pressureInletOutletVelocityFvPatchVectorField::autoMap
(
    const fvPatchFieldMapper& m
)
{
    this->parent_bctype::autoMap(m);
    if (tangentialVelocity_.size())
    {
        tangentialVelocity_.autoMap(m);
    }
}


void Foam::pressureInletOutletVelocityFvPatchVectorField::rmap
(
    const fvPatchVectorField& ptf,
    const labelList& addr
)
{
    this->parent_bctype::rmap(ptf, addr);

    const auto& tiptf = refCast<const this_bctype>(ptf);

    if (tangentialVelocity_.size())
    {
        tangentialVelocity_.rmap(tiptf.tangentialVelocity_, addr);
    }
}


void Foam::pressureInletOutletVelocityFvPatchVectorField::updateCoeffs()
{
    if (updated())
    {
        return;
    }

    const fvsPatchField<scalar>& phip =
        patch().lookupPatchField<surfaceScalarField>(phiName_);

    valueFraction() = neg(phip)*(I - sqr(patch().nf()));

    this->parent_bctype::updateCoeffs();
    this->parent_bctype::evaluate();
}


void Foam::pressureInletOutletVelocityFvPatchVectorField::write
(
    Ostream& os
)
const
{
    fvPatchField<vector>::write(os);
    os.writeEntryIfDifferent<word>("phi", "phi", phiName_);
    if (tangentialVelocity_.size())
    {
        tangentialVelocity_.writeEntry("tangentialVelocity", os);
    }
    fvPatchField<vector>::writeValueEntry(os);
}


// * * * * * * * * * * * * * * * Member Operators  * * * * * * * * * * * * * //

void Foam::pressureInletOutletVelocityFvPatchVectorField::operator=
(
    const fvPatchField<vector>& pvf
)
{
    tmp<vectorField> normalValue = transform(valueFraction(), refValue());
    tmp<vectorField> transformGradValue = transform(I - valueFraction(), pvf);
    fvPatchField<vector>::operator=(normalValue + transformGradValue);
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
    makePatchTypeField
    (
        fvPatchVectorField,
        pressureInletOutletVelocityFvPatchVectorField
    );
}

// ************************************************************************* //
