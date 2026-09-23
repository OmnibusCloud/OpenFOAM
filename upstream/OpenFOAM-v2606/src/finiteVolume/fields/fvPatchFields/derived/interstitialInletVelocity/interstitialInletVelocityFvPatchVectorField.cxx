/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2013-2016 OpenFOAM Foundation
    Copyright (C) 2020 OpenCFD Ltd.
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

#include "interstitialInletVelocityFvPatchVectorField.H"
#include "volFields.H"
#include "addToRunTimeSelectionTable.H"
#include "fvPatchFieldMapper.H"
#include "surfaceFields.H"

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::interstitialInletVelocityFvPatchVectorField::
interstitialInletVelocityFvPatchVectorField
(
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF
)
:
    parent_bctype(p, iF),
    inletVelocity_(p.size(), Zero),
    alphaName_("alpha")
{}


Foam::interstitialInletVelocityFvPatchVectorField::
interstitialInletVelocityFvPatchVectorField
(
    const this_bctype& ptf,
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF,
    const fvPatchFieldMapper& mapper
)
:
    parent_bctype(ptf, p, iF, mapper),
    inletVelocity_(ptf.inletVelocity_, mapper),
    alphaName_(ptf.alphaName_)
{}


Foam::interstitialInletVelocityFvPatchVectorField::
interstitialInletVelocityFvPatchVectorField
(
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF,
    const dictionary& dict
)
:
    parent_bctype(p, iF, dict),
    inletVelocity_("inletVelocity", dict, p.size()),
    alphaName_(dict.getOrDefault<word>("alpha", "alpha"))
{}


Foam::interstitialInletVelocityFvPatchVectorField::
interstitialInletVelocityFvPatchVectorField
(
    const this_bctype& ptf,
    const DimensionedField<vector, volMesh>& iF
)
:
    parent_bctype(ptf, iF),
    inletVelocity_(ptf.inletVelocity_),
    alphaName_(ptf.alphaName_)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::interstitialInletVelocityFvPatchVectorField::autoMap
(
    const fvPatchFieldMapper& m
)
{
    this->parent_bctype::autoMap(m);
    inletVelocity_.autoMap(m);
}


void Foam::interstitialInletVelocityFvPatchVectorField::rmap
(
    const fvPatchVectorField& ptf,
    const labelList& addr
)
{
    this->parent_bctype::rmap(ptf, addr);

    const auto& tiptf = refCast<const this_bctype>(ptf);

    inletVelocity_.rmap(tiptf.inletVelocity_, addr);
}


void Foam::interstitialInletVelocityFvPatchVectorField::updateCoeffs()
{
    if (updated())
    {
        return;
    }

    const auto& alphap = patch().lookupPatchField<volScalarField>(alphaName_);

    operator==(inletVelocity_/alphap);
    this->parent_bctype::updateCoeffs();
}


void Foam::interstitialInletVelocityFvPatchVectorField::write(Ostream& os) const
{
    fvPatchField<vector>::write(os);
    os.writeEntryIfDifferent<word>("alpha", "alpha", alphaName_);
    inletVelocity_.writeEntry("inletVelocity", os);
    fvPatchField<vector>::writeValueEntry(os);
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
   makePatchTypeField
   (
       fvPatchVectorField,
       interstitialInletVelocityFvPatchVectorField
   );
}


// ************************************************************************* //
