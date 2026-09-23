/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2017 OpenFOAM Foundation
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

#include "timeVaryingUniformFixedValuePointPatchField.H"
#include "Time.H"

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

template<class Type>
Foam::
timeVaryingUniformFixedValuePointPatchField<Type>::
timeVaryingUniformFixedValuePointPatchField
(
    const pointPatch& p,
    const DimensionedField<Type, pointMesh>& iF
)
:
    parent_bctype(p, iF)
{}


template<class Type>
Foam::
timeVaryingUniformFixedValuePointPatchField<Type>::
timeVaryingUniformFixedValuePointPatchField
(
    const this_bctype& ptf,
    const pointPatch& p,
    const DimensionedField<Type, pointMesh>& iF,
    const pointPatchFieldMapper& mapper
)
:
    parent_bctype(ptf, p, iF, mapper),
    timeSeries_(ptf.timeSeries_)
{
    this->operator==(timeSeries_(this->db().time().timeOutputValue()));
}


template<class Type>
Foam::
timeVaryingUniformFixedValuePointPatchField<Type>::
timeVaryingUniformFixedValuePointPatchField
(
    const pointPatch& p,
    const DimensionedField<Type, pointMesh>& iF,
    const dictionary& dict
)
:
    parent_bctype(p, iF),
    timeSeries_(dict)
{
    this->operator==(timeSeries_(this->db().time().timeOutputValue()));
}


template<class Type>
Foam::
timeVaryingUniformFixedValuePointPatchField<Type>::
timeVaryingUniformFixedValuePointPatchField
(
    const this_bctype& ptf,
    const DimensionedField<Type, pointMesh>& iF
)
:
    parent_bctype(ptf, iF),
    timeSeries_(ptf.timeSeries_)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class Type>
void Foam::timeVaryingUniformFixedValuePointPatchField<Type>::updateCoeffs()
{
    if (this->updated())
    {
        return;
    }

    this->operator==(timeSeries_(this->db().time().timeOutputValue()));
    this->parent_bctype::updateCoeffs();
}


template<class Type>
void Foam::timeVaryingUniformFixedValuePointPatchField<Type>::write
(
    Ostream& os
) const
{
    this->parent_bctype::write(os);
    timeSeries_.write(os);
}


// ************************************************************************* //
