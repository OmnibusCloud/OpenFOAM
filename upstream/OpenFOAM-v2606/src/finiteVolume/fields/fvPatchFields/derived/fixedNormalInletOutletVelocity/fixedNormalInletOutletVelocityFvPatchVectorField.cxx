/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2014-2016 OpenFOAM Foundation
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

#include "fixedNormalInletOutletVelocityFvPatchVectorField.H"
#include "addToRunTimeSelectionTable.H"
#include "volFields.H"
#include "surfaceFields.H"


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::fixedNormalInletOutletVelocityFvPatchVectorField::
fixedNormalInletOutletVelocityFvPatchVectorField
(
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF
)
:
    parent_bctype(p, iF),
    phiName_("phi"),
    fixTangentialInflow_(true),
    normalVelocity_
    (
        fvPatchVectorField::New("fixedValue", p, iF)
    )
{
    refValue() = Zero;
    refGrad() = Zero;
    valueFraction() = Zero;
}


Foam::fixedNormalInletOutletVelocityFvPatchVectorField::
fixedNormalInletOutletVelocityFvPatchVectorField
(
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF,
    const dictionary& dict
)
:
    parent_bctype(p, iF),
    phiName_(dict.getOrDefault<word>("phi", "phi")),
    fixTangentialInflow_(dict.lookup("fixTangentialInflow")),
    normalVelocity_
    (
        fvPatchVectorField::New(p, iF, dict.subDict("normalVelocity"))
    )
{
    fvPatchFieldBase::readDict(dict);
    this->readValueEntry(dict, IOobjectOption::MUST_READ);
    refValue() = normalVelocity();
    refGrad() = Zero;
    valueFraction() = Zero;
}


Foam::fixedNormalInletOutletVelocityFvPatchVectorField::
fixedNormalInletOutletVelocityFvPatchVectorField
(
    const this_bctype& ptf,
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF,
    const fvPatchFieldMapper& mapper
)
:
    parent_bctype(ptf, p, iF, mapper),
    phiName_(ptf.phiName_),
    fixTangentialInflow_(ptf.fixTangentialInflow_),
    normalVelocity_
    (
        fvPatchVectorField::New(ptf.normalVelocity(), p, iF, mapper)
    )
{}


Foam::fixedNormalInletOutletVelocityFvPatchVectorField::
fixedNormalInletOutletVelocityFvPatchVectorField
(
    const this_bctype& pivpvf,
    const DimensionedField<vector, volMesh>& iF
)
:
    parent_bctype(pivpvf, iF),
    phiName_(pivpvf.phiName_),
    fixTangentialInflow_(pivpvf.fixTangentialInflow_),
    normalVelocity_(pivpvf.normalVelocity().clone(iF))
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::fixedNormalInletOutletVelocityFvPatchVectorField::autoMap
(
    const fvPatchFieldMapper& m
)
{
    this->parent_bctype::autoMap(m);
    normalVelocity_->autoMap(m);
}


void Foam::fixedNormalInletOutletVelocityFvPatchVectorField::rmap
(
    const fvPatchVectorField& ptf,
    const labelList& addr
)
{
    this->parent_bctype::rmap(ptf, addr);

    const auto& fniovptf = refCast<const this_bctype>(ptf);

    normalVelocity_->rmap(fniovptf.normalVelocity(), addr);
}


void Foam::fixedNormalInletOutletVelocityFvPatchVectorField::updateCoeffs()
{
    if (updated())
    {
        return;
    }

    normalVelocity_->evaluate();
    refValue() = normalVelocity();

    valueFraction() = sqr(patch().nf());

    if (fixTangentialInflow_)
    {
        const auto& phip =
            patch().lookupPatchField<surfaceScalarField>(phiName_);

        valueFraction() += neg(phip)*(I - valueFraction());
    }

    this->parent_bctype::updateCoeffs();
    this->parent_bctype::evaluate();
}


void Foam::fixedNormalInletOutletVelocityFvPatchVectorField::write
(
    Ostream& os
)
const
{
    fvPatchField<vector>::write(os);
    os.writeEntryIfDifferent<word>("phi", "phi", phiName_);
    os.writeEntry("fixTangentialInflow", fixTangentialInflow_);

    os.beginBlock("normalVelocity");
    normalVelocity_->write(os);
    os.endBlock();

    fvPatchField<vector>::writeValueEntry(os);
}


// * * * * * * * * * * * * * * * Member Operators  * * * * * * * * * * * * * //

void Foam::fixedNormalInletOutletVelocityFvPatchVectorField::operator=
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
        fixedNormalInletOutletVelocityFvPatchVectorField
    );
}

// ************************************************************************* //
