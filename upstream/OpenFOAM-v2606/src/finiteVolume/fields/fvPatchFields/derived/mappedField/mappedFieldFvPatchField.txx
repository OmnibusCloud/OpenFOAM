/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2018 OpenFOAM Foundation
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

#include "mappedFieldFvPatchField.H"
#include "volFields.H"
#include "interpolationCell.H"

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

template<class Type>
Foam::mappedFieldFvPatchField<Type>::mappedFieldFvPatchField
(
    const fvPatch& p,
    const DimensionedField<Type, volMesh>& iF
)
:
    parent_bctype(p, iF),
    mappedPatchBase(p.patch()),
    mappedPatchFieldBase<Type>(*this, *this)
{}


template<class Type>
Foam::mappedFieldFvPatchField<Type>::mappedFieldFvPatchField
(
    const fvPatch& p,
    const DimensionedField<Type, volMesh>& iF,
    const dictionary& dict
)
:
    parent_bctype(p, iF, dict),
    mappedPatchBase(p.patch(), dict),
    mappedPatchFieldBase<Type>(*this, *this, dict, *this)
{}


template<class Type>
Foam::mappedFieldFvPatchField<Type>::mappedFieldFvPatchField
(
    const this_bctype& ptf,
    const fvPatch& p,
    const DimensionedField<Type, volMesh>& iF,
    const fvPatchFieldMapper& mapper
)
:
    parent_bctype(ptf, p, iF, mapper),
    mappedPatchBase(p.patch(), ptf),
    mappedPatchFieldBase<Type>(*this, *this, ptf)
{}


template<class Type>
Foam::mappedFieldFvPatchField<Type>::mappedFieldFvPatchField
(
    const fvPatch& p,
    const DimensionedField<Type, volMesh>& iF,

    // mappedPatchBase
    const word& sampleRegion,
    const sampleMode sampleMode,
    const word& samplePatch,
    const scalar distance,

    // My settings
    const word& fieldName,
    const bool setAverage,
    const Type average,
    const word& interpolationScheme
)
:
    parent_bctype(p, iF),
    mappedPatchBase
    (
        p.patch(),
        sampleRegion,
        sampleMode,
        samplePatch,
        distance
    ),
    mappedPatchFieldBase<Type>
    (
        *this,
        *this,
        fieldName,
        setAverage,
        average,
        interpolationScheme
    )
{}


template<class Type>
Foam::mappedFieldFvPatchField<Type>::mappedFieldFvPatchField
(
    const this_bctype& ptf,
    const DimensionedField<Type, volMesh>& iF
)
:
    parent_bctype(ptf, iF),
    mappedPatchBase(ptf.patch().patch(), ptf),
    mappedPatchFieldBase<Type>(*this, *this, ptf)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class Type>
void Foam::mappedFieldFvPatchField<Type>::autoMap
(
    const fvPatchFieldMapper& m
)
{
    this->parent_bctype::autoMap(m);
    mappedPatchBase::clearOut();
}


template<class Type>
void Foam::mappedFieldFvPatchField<Type>::rmap
(
    const fvPatchField<Type>& ptf,
    const labelList& addr
)
{
    this->parent_bctype::rmap(ptf, addr);
    mappedPatchBase::clearOut();
}


template<class Type>
void Foam::mappedFieldFvPatchField<Type>::updateCoeffs()
{
    if (this->updated())
    {
        return;
    }

    this->operator==(this->mappedField());

    if (debug)
    {
        auto limits = gMinMax(*this);
        auto avg = gAverage(*this);

        Info<< "operating on field:" << this->internalField().name()
            << " patch:" << this->patch().name()
            << "  avg:" << avg
            << "  min:" << limits.min()
            << "  max:" << limits.max()
            << endl;
    }

    this->parent_bctype::updateCoeffs();
}


template<class Type>
void Foam::mappedFieldFvPatchField<Type>::write(Ostream& os) const
{
    fvPatchField<Type>::write(os);
    mappedPatchBase::write(os);
    mappedPatchFieldBase<Type>::write(os);
    fvPatchField<Type>::writeValueEntry(os);
}


// ************************************************************************* //
