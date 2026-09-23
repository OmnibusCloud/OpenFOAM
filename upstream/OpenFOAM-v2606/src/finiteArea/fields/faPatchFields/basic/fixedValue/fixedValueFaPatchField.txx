/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2016-2017 Wikki Ltd
    Copyright (C) 2023-2025 OpenCFD Ltd.
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

#include "fixedValueFaPatchField.H"

// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class Type>
Foam::fixedValueFaPatchField<Type>::fixedValueFaPatchField
(
    const faPatch& p,
    const DimensionedField<Type, areaMesh>& iF
)
:
    parent_bctype(p, iF)
{}


template<class Type>
Foam::fixedValueFaPatchField<Type>::fixedValueFaPatchField
(
    const faPatch& p,
    const DimensionedField<Type, areaMesh>& iF,
    const Type& value
)
:
    parent_bctype(p, iF, value)
{}


template<class Type>
Foam::fixedValueFaPatchField<Type>::fixedValueFaPatchField
(
    const faPatch& p,
    const DimensionedField<Type, areaMesh>& iF,
    const dictionary& dict,
    IOobjectOption::readOption requireValue
)
:
    parent_bctype(p, iF, dict, requireValue)
{}


template<class Type>
Foam::fixedValueFaPatchField<Type>::fixedValueFaPatchField
(
    const this_bctype& ptf,
    const faPatch& p,
    const DimensionedField<Type, areaMesh>& iF,
    const faPatchFieldMapper& mapper
)
:
    parent_bctype(ptf, p, iF, mapper)
{}


template<class Type>
Foam::fixedValueFaPatchField<Type>::fixedValueFaPatchField
(
    const this_bctype& pfld,
    const faPatch& p,
    const DimensionedField<Type, areaMesh>& iF,
    const Type& value
)
:
    parent_bctype(pfld, p, iF, value)
{}


template<class Type>
Foam::fixedValueFaPatchField<Type>::fixedValueFaPatchField
(
    const this_bctype& pfld,
    const DimensionedField<Type, areaMesh>& iF
)
:
    parent_bctype(pfld, iF)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class Type>
Foam::tmp<Foam::Field<Type>>
Foam::fixedValueFaPatchField<Type>::valueInternalCoeffs
(
    const tmp<scalarField>&
) const
{
    // No contribution from internal values
    return tmp<Field<Type>>::New(this->size(), Foam::zero{});
}


template<class Type>
Foam::tmp<Foam::Field<Type>>
Foam::fixedValueFaPatchField<Type>::valueBoundaryCoeffs
(
    const tmp<scalarField>&
) const
{
    return *this;
}


template<class Type>
Foam::tmp<Foam::Field<Type>>
Foam::fixedValueFaPatchField<Type>::gradientInternalCoeffs() const
{
    return -Type(pTraits<Type>::one)*this->patch().deltaCoeffs();
}


template<class Type>
Foam::tmp<Foam::Field<Type>>
Foam::fixedValueFaPatchField<Type>::gradientBoundaryCoeffs() const
{
    return this->patch().deltaCoeffs()*(*this);
}


template<class Type>
void Foam::fixedValueFaPatchField<Type>::write(Ostream& os) const
{
    faPatchField<Type>::write(os);
    faPatchField<Type>::writeValueEntry(os);
}


// ************************************************************************* //
