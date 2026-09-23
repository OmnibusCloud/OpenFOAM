/*---------------------------------------------------------------------------* \
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

#include "SRFVelocityFvPatchVectorField.H"
#include "addToRunTimeSelectionTable.H"
#include "volFields.H"

#include "SRFModel.H"

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::SRFVelocityFvPatchVectorField::SRFVelocityFvPatchVectorField
(
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF
)
:
    parent_bctype(p, iF),
    relative_(false),
    inletValue_(p.size(), Zero)
{}


Foam::SRFVelocityFvPatchVectorField::SRFVelocityFvPatchVectorField
(
    const this_bctype& ptf,
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF,
    const fvPatchFieldMapper& mapper
)
:
    parent_bctype(ptf, p, iF, mapper),
    relative_(ptf.relative_),
    inletValue_(ptf.inletValue_, mapper)
{}


Foam::SRFVelocityFvPatchVectorField::SRFVelocityFvPatchVectorField
(
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF,
    const dictionary& dict
)
:
    parent_bctype(p, iF, dict),
    relative_(dict.get<Switch>("relative")),
    inletValue_("inletValue", dict, p.size())
{}


Foam::SRFVelocityFvPatchVectorField::SRFVelocityFvPatchVectorField
(
    const this_bctype& ptf,
    const DimensionedField<vector, volMesh>& iF
)
:
    parent_bctype(ptf, iF),
    relative_(ptf.relative_),
    inletValue_(ptf.inletValue_)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::SRFVelocityFvPatchVectorField::autoMap
(
    const fvPatchFieldMapper& m
)
{
    vectorField::autoMap(m);
    inletValue_.autoMap(m);
}


void Foam::SRFVelocityFvPatchVectorField::rmap
(
    const fvPatchVectorField& ptf,
    const labelList& addr
)
{
    this->parent_bctype::rmap(ptf, addr);

    const auto& tiptf = refCast<const this_bctype>(ptf);

    inletValue_.rmap(tiptf.inletValue_, addr);
}


void Foam::SRFVelocityFvPatchVectorField::updateCoeffs()
{
    if (updated())
    {
        return;
    }

    // If not relative to the SRF include the effect of the SRF
    if (!relative_)
    {
        // Get reference to the SRF model
        const SRF::SRFModel& srf =
            db().lookupObject<SRF::SRFModel>("SRFProperties");

        // Determine patch velocity due to SRF
        const vectorField SRFVelocity(srf.velocity(patch().Cf()));

        operator==(-SRFVelocity + inletValue_);
    }
    // If already relative to the SRF simply supply the inlet value as a fixed
    // value
    else
    {
        operator==(inletValue_);
    }

    this->parent_bctype::updateCoeffs();
}


void Foam::SRFVelocityFvPatchVectorField::write(Ostream& os) const
{
    fvPatchField<vector>::write(os);
    os.writeEntry("relative", relative_);
    inletValue_.writeEntry("inletValue", os);
    fvPatchField<vector>::writeValueEntry(os);
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
    makePatchTypeField
    (
        fvPatchVectorField,
        SRFVelocityFvPatchVectorField
    );
}

// ************************************************************************* //
