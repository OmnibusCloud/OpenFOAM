/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
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

#include "atmBoundaryLayerInletOmegaFvPatchScalarField.H"
#include "addToRunTimeSelectionTable.H"
#include "fvPatchFieldMapper.H"
#include "volFields.H"
#include "surfaceFields.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

atmBoundaryLayerInletOmegaFvPatchScalarField::
atmBoundaryLayerInletOmegaFvPatchScalarField
(
    const fvPatch& p,
    const DimensionedField<scalar, volMesh>& iF
)
:
    parent_bctype(p, iF),
    atmBoundaryLayer(iF.time(), p.patch())
{}


atmBoundaryLayerInletOmegaFvPatchScalarField::
atmBoundaryLayerInletOmegaFvPatchScalarField
(
    const fvPatch& p,
    const DimensionedField<scalar, volMesh>& iF,
    const dictionary& dict
)
:
    parent_bctype(p, iF),
    atmBoundaryLayer(iF.time(), p.patch(), dict)
{
    phiName_ = dict.getOrDefault<word>("phi", "phi");

    refValue() = omega(patch().Cf());
    refGrad() = 0;
    valueFraction() = 1;

    if (!initABL_)
    {
        this->readValueEntry(dict, IOobjectOption::MUST_READ);
    }
    else
    {
        scalarField::operator=(refValue());
        initABL_ = false;
    }
}


atmBoundaryLayerInletOmegaFvPatchScalarField::
atmBoundaryLayerInletOmegaFvPatchScalarField
(
    const this_bctype& psf,
    const fvPatch& p,
    const DimensionedField<scalar, volMesh>& iF,
    const fvPatchFieldMapper& mapper
)
:
    parent_bctype(psf, p, iF, mapper),
    atmBoundaryLayer(psf, p, mapper)
{}


atmBoundaryLayerInletOmegaFvPatchScalarField::
atmBoundaryLayerInletOmegaFvPatchScalarField
(
    const this_bctype& psf,
    const DimensionedField<scalar, volMesh>& iF
)
:
    parent_bctype(psf, iF),
    atmBoundaryLayer(psf)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void atmBoundaryLayerInletOmegaFvPatchScalarField::updateCoeffs()
{
    if (updated())
    {
        return;
    }

    refValue() = omega(patch().Cf());

    this->parent_bctype::updateCoeffs();
}


void atmBoundaryLayerInletOmegaFvPatchScalarField::autoMap
(
    const fvPatchFieldMapper& m
)
{
    this->parent_bctype::autoMap(m);
    atmBoundaryLayer::autoMap(m);
}


void atmBoundaryLayerInletOmegaFvPatchScalarField::rmap
(
    const fvPatchScalarField& psf,
    const labelList& addr
)
{
    this->parent_bctype::rmap(psf, addr);

    const auto& blpsf = refCast<const this_bctype>(psf);

    atmBoundaryLayer::rmap(blpsf, addr);
}


void atmBoundaryLayerInletOmegaFvPatchScalarField::write(Ostream& os) const
{
    fvPatchField<scalar>::write(os);
    os.writeEntryIfDifferent<word>("phi", "phi", phiName_);
    atmBoundaryLayer::write(os);
    fvPatchField<scalar>::writeValueEntry(os);
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

makePatchTypeField
(
    fvPatchScalarField,
    atmBoundaryLayerInletOmegaFvPatchScalarField
);

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// ************************************************************************* //
