/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2016-2017 Wikki Ltd
    Copyright (C) 2020-2023 OpenCFD Ltd.
    Copyright (C) 2026 Keysight Technologies
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

#include "inletOutletFaPatchField.H"

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

template<class Type>
Foam::inletOutletFaPatchField<Type>::inletOutletFaPatchField
(
    const faPatch& p,
    const DimensionedField<Type, areaMesh>& iF
)
:
    parent_bctype(p, iF),
    phiName_("phis")
{
    this->refValue() = Zero;
    this->refGrad() = Zero;
    this->valueFraction() = 0;
}


template<class Type>
Foam::inletOutletFaPatchField<Type>::inletOutletFaPatchField
(
    const this_bctype& ptf,
    const faPatch& p,
    const DimensionedField<Type, areaMesh>& iF,
    const faPatchFieldMapper& mapper
)
:
    parent_bctype(ptf, p, iF, mapper),
    phiName_(ptf.phiName_)
{}


template<class Type>
Foam::inletOutletFaPatchField<Type>::inletOutletFaPatchField
(
    const faPatch& p,
    const DimensionedField<Type, areaMesh>& iF,
    const dictionary& dict
)
:
    // No reading of refValue, refGradient, valueFraction entries
    parent_bctype(p, iF, dict, IOobjectOption::NO_READ),
    phiName_(dict.getOrDefault<word>("phi", "phis"))
{
    // Require inletValue (MUST_READ)
    this->refValue().assign("inletValue", dict, p.size());
    this->refGrad() = Zero;
    this->valueFraction() = 0;

    if (!this->readValueEntry(dict))
    {
        faPatchField<Type>::extrapolateInternal();
    }
}


template<class Type>
Foam::inletOutletFaPatchField<Type>::inletOutletFaPatchField
(
    const this_bctype& ptf,
    const DimensionedField<Type, areaMesh>& iF
)
:
    parent_bctype(ptf, iF),
    phiName_(ptf.phiName_)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class Type>
void Foam::inletOutletFaPatchField<Type>::updateCoeffs()
{
    if (this->updated())
    {
        return;
    }

    const Field<scalar>& phip =
        this->patch().template lookupPatchField<edgeScalarField>(phiName_);

    this->valueFraction() = neg(phip);

    mixedFaPatchField<Type>::updateCoeffs();
}


template<class Type>
void Foam::inletOutletFaPatchField<Type>::write(Ostream& os) const
{
    faPatchField<Type>::write(os);
    os.writeEntryIfDifferent<word>("phi", "phis", phiName_);
    this->refValue().writeEntry("inletValue", os);
    faPatchField<Type>::writeValueEntry(os);
}


// ************************************************************************* //
