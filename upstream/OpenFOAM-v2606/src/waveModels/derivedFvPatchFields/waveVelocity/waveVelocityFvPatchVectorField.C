/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2015 IH-Cantabria
    Copyright (C) 2016-2020 OpenCFD Ltd.
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

#include "waveVelocityFvPatchVectorField.H"
#include "addToRunTimeSelectionTable.H"
#include "fvPatchFieldMapper.H"
#include "waveModel.H"

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::waveVelocityFvPatchVectorField::waveVelocityFvPatchVectorField
(
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF
)
:
    parent_bctype(p, iF),
    waveDictName_(waveModel::dictName)
{}


Foam::waveVelocityFvPatchVectorField::waveVelocityFvPatchVectorField
(
    const this_bctype& ptf,
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF,
    const fvPatchFieldMapper& mapper
)
:
    parent_bctype(ptf, p, iF, mapper),
    waveDictName_(ptf.waveDictName_)
{}


Foam::waveVelocityFvPatchVectorField::waveVelocityFvPatchVectorField
(
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF,
    const dictionary& dict
)
:
    parent_bctype(p, iF, dict),
    waveDictName_(dict.getOrDefault<word>("waveDict", waveModel::dictName))
{}


Foam::waveVelocityFvPatchVectorField::waveVelocityFvPatchVectorField
(
    const this_bctype& ptf,
    const DimensionedField<vector, volMesh>& iF
)
:
    parent_bctype(ptf, iF),
    waveDictName_(ptf.waveDictName_)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::waveVelocityFvPatchVectorField::updateCoeffs()
{
    if (updated())
    {
        return;
    }

    tmp<waveModel> tmodel
    (
        waveModel::lookupOrCreate
        (
            patch().patch(),
            internalField().mesh(),
            waveDictName_
        )
    );

    waveModel& model = tmodel.constCast();

    model.correct(db().time().value());

    operator==(model.U());

    this->parent_bctype::updateCoeffs();
}


void Foam::waveVelocityFvPatchVectorField::write(Ostream& os) const
{
    fvPatchField<vector>::write(os);

    os.writeEntry("waveDictName", waveDictName_);

    fvPatchField<vector>::writeValueEntry(os);
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
   makePatchTypeField
   (
       fvPatchVectorField,
       waveVelocityFvPatchVectorField
   );
}


// ************************************************************************* //
