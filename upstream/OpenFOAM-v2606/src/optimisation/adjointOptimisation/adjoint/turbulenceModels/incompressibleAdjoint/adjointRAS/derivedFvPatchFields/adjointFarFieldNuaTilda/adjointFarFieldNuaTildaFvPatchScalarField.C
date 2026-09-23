/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2007-2020 PCOpt/NTUA
    Copyright (C) 2013-2020 FOSS GP
    Copyright (C) 2019 OpenCFD Ltd.
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

#include "adjointFarFieldNuaTildaFvPatchScalarField.H"
#include "addToRunTimeSelectionTable.H"
#include "fvPatchFieldMapper.H"
#include "volFields.H"
#include "surfaceFields.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

adjointFarFieldNuaTildaFvPatchScalarField::
adjointFarFieldNuaTildaFvPatchScalarField
(
    const fvPatch& p,
    const DimensionedField<scalar, volMesh>& iF
)
:
    parent_bctype(p, iF),
    adjointScalarBoundaryCondition(p, iF, word::null)
{}


adjointFarFieldNuaTildaFvPatchScalarField::
adjointFarFieldNuaTildaFvPatchScalarField
(
    const this_bctype& ptf,
    const fvPatch& p,
    const DimensionedField<scalar, volMesh>& iF,
    const fvPatchFieldMapper& mapper
)
:
    parent_bctype(ptf, p, iF, mapper),
    adjointScalarBoundaryCondition(p, iF, ptf.adjointSolverName_)
{}


adjointFarFieldNuaTildaFvPatchScalarField::
adjointFarFieldNuaTildaFvPatchScalarField
(
    const fvPatch& p,
    const DimensionedField<scalar, volMesh>& iF,
    const dictionary& dict
)
:
    parent_bctype(p, iF),
    adjointScalarBoundaryCondition(p, iF, dict.get<word>("solverName"))
{
    this->readValueEntry(dict, IOobjectOption::MUST_READ);
}


adjointFarFieldNuaTildaFvPatchScalarField::
adjointFarFieldNuaTildaFvPatchScalarField
(
    const this_bctype& tppsf,
    const DimensionedField<scalar, volMesh>& iF
)
:
    parent_bctype(tppsf, iF),
    adjointScalarBoundaryCondition(tppsf)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void adjointFarFieldNuaTildaFvPatchScalarField::updateCoeffs()
{
    if (updated())
    {
        return;
    }
    vectorField nf(patch().nf());

    const fvPatchField<vector>& Ub = boundaryContrPtr_->Ub();
    tmp<scalarField> tnuEff(boundaryContrPtr_->TMVariable1Diffusion());
    const scalarField& nuEff = tnuEff();
    const fvsPatchField<scalar>& phip = boundaryContrPtr_->phib();

    // Patch-adjacent nuaTilda nuaTildaNei
    tmp<scalarField> tnuaTildaNei(patchInternalField());
    const scalarField& nuaTildaNei = tnuaTildaNei();

    // Patch deltas
    const scalarField& delta = patch().deltaCoeffs();

    operator==
    (
        pos(phip)
       *(
            (nuEff*delta*nuaTildaNei)
           /((Ub & nf) + nuEff*delta)
        )
    );

    this->parent_bctype::updateCoeffs();
}


tmp<Field<scalar>> adjointFarFieldNuaTildaFvPatchScalarField::
valueInternalCoeffs
(
    const tmp<scalarField>&
) const
{
    const fvsPatchField<scalar>& phip = boundaryContrPtr_->phib();

    // For fixedValue nuTilda patches
    return tmp<Field<scalar>>::New(neg(phip)*pTraits<scalar>::one);
}


tmp<Field<scalar>> adjointFarFieldNuaTildaFvPatchScalarField::
valueBoundaryCoeffs
(
    const tmp<scalarField>&
) const
{
    const fvsPatchField<scalar>& phip = boundaryContrPtr_->phib();

    // For zeroGradient nuTilda patches
    return tmp<Field<scalar>>::New(pos(phip)*(*this));
}


void adjointFarFieldNuaTildaFvPatchScalarField::write(Ostream& os) const
{
    fvPatchField<scalar>::write(os);
    os.writeEntry("solverName", adjointSolverName_);
    fvPatchField<scalar>::writeValueEntry(os);
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

makePatchTypeField
(
    fvPatchScalarField,
    adjointFarFieldNuaTildaFvPatchScalarField
);

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// ************************************************************************* //
