/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
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

#include "fixedValueFvPatchField.H"

// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class Type>
Foam::fixedValueFvPatchField<Type>::fixedValueFvPatchField
(
    const fvPatch& p,
    const DimensionedField<Type, volMesh>& iF
)
:
    parent_bctype(p, iF)
{}


template<class Type>
Foam::fixedValueFvPatchField<Type>::fixedValueFvPatchField
(
    const fvPatch& p,
    const DimensionedField<Type, volMesh>& iF,
    const Type& value
)
:
    parent_bctype(p, iF, value)
{}


template<class Type>
Foam::fixedValueFvPatchField<Type>::fixedValueFvPatchField
(
    const fvPatch& p,
    const DimensionedField<Type, volMesh>& iF,
    const dictionary& dict,
    IOobjectOption::readOption requireValue
)
:
    parent_bctype(p, iF, dict, requireValue)
{}


template<class Type>
Foam::fixedValueFvPatchField<Type>::fixedValueFvPatchField
(
    const this_bctype& ptf,
    const fvPatch& p,
    const DimensionedField<Type, volMesh>& iF,
    const fvPatchFieldMapper& mapper
)
:
    parent_bctype(ptf, p, iF, mapper)
{
    if (notNull(iF) && mapper.hasUnmapped())
    {
        WarningInFunction
            << "On field " << iF.name() << " patch " << p.name()
            << " patchField " << this->type()
            << " : mapper does not map all values." << nl
            << "    To avoid this warning fully specify the mapping in derived"
            << " patch fields." << endl;
    }
}


template<class Type>
Foam::fixedValueFvPatchField<Type>::fixedValueFvPatchField
(
    const this_bctype& pfld,
    const fvPatch& p,
    const DimensionedField<Type, volMesh>& iF,
    const Type& value
)
:
    parent_bctype(pfld, p, iF, value)
{}


template<class Type>
Foam::fixedValueFvPatchField<Type>::fixedValueFvPatchField
(
    const this_bctype& pfld,
    const DimensionedField<Type, volMesh>& iF
)
:
    parent_bctype(pfld, iF)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class Type>
Foam::tmp<Foam::Field<Type>>
Foam::fixedValueFvPatchField<Type>::valueInternalCoeffs
(
    const tmp<scalarField>&
) const
{
    // No contribution from internal values
    return tmp<Field<Type>>::New(this->size(), Foam::zero{});
}


template<class Type>
Foam::tmp<Foam::Field<Type>>
Foam::fixedValueFvPatchField<Type>::valueBoundaryCoeffs
(
    const tmp<scalarField>&
) const
{
    return *this;
}


template<class Type>
Foam::tmp<Foam::Field<Type>>
Foam::fixedValueFvPatchField<Type>::gradientInternalCoeffs() const
{
    return -pTraits<Type>::one*this->patch().deltaCoeffs();
}


template<class Type>
Foam::tmp<Foam::Field<Type>>
Foam::fixedValueFvPatchField<Type>::gradientBoundaryCoeffs() const
{
    return this->patch().deltaCoeffs()*(*this);
}


template<class Type>
void Foam::fixedValueFvPatchField<Type>::write(Ostream& os) const
{
    this->parent_bctype::write(os);
    fvPatchField<Type>::writeValueEntry(os);
}


// ************************************************************************* //
