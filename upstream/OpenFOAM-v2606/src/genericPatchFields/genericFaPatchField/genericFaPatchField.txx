/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
    Copyright (C) 2019-2026 OpenCFD Ltd.
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

#include "genericFaPatchField.H"
#include "faPatchFieldMapper.H"

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

template<class Type>
Foam::genericFaPatchField<Type>::genericFaPatchField
(
    const faPatch& p,
    const DimensionedField<Type, areaMesh>& iF
)
:
    parent_bctype(p, iF)
{
    FatalErrorInFunction
        << "Trying to construct generic patchField on patch "
        << this->patch().name()
        << " of field " << this->internalField().name() << nl
        << abort(FatalError);
}


template<class Type>
Foam::genericFaPatchField<Type>::genericFaPatchField
(
    const faPatch& p,
    const DimensionedField<Type, areaMesh>& iF,
    const dictionary& dict
)
:
    genericPatchFieldBase(dict),
    parent_bctype(p, iF, dict)
{
    const label patchSize = this->size();
    const word& patchName = this->patch().name();
    const IOobject& io = this->internalField();

    if (!dict.findEntry("value", keyType::LITERAL))
    {
        reportMissingEntry("value", patchName, io);
    }

    // Handle "value" separately
    processGeneric(patchSize, patchName, io, true);
}


template<class Type>
Foam::genericFaPatchField<Type>::genericFaPatchField
(
    const this_bctype& rhs,
    const faPatch& p,
    const DimensionedField<Type, areaMesh>& iF,
    const faPatchFieldMapper& mapper
)
:
    genericPatchFieldBase(Foam::zero{}, rhs),
    parent_bctype(rhs, p, iF, mapper)
{
    this->mapGeneric(rhs, mapper);
}


template<class Type>
Foam::genericFaPatchField<Type>::genericFaPatchField
(
    const this_bctype& rhs,
    const DimensionedField<Type, areaMesh>& iF
)
:
    genericPatchFieldBase(rhs),
    parent_bctype(rhs, iF)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class Type>
void Foam::genericFaPatchField<Type>::write(Ostream& os) const
{
    // Handle "value" separately
    genericPatchFieldBase::writeGeneric(os, true);
    faPatchField<Type>::writeValueEntry(os);
}


template<class Type>
void Foam::genericFaPatchField<Type>::autoMap
(
    const faPatchFieldMapper& m
)
{
    parent_bctype::autoMap(m);
    this->autoMapGeneric(m);
}


template<class Type>
void Foam::genericFaPatchField<Type>::rmap
(
    const faPatchField<Type>& rhs,
    const labelList& addr
)
{
    parent_bctype::rmap(rhs, addr);

    if (const auto* base = isA<genericPatchFieldBase>(rhs); base)
    {
        this->rmapGeneric(*base, addr);
    }
}


template<class Type>
Foam::tmp<Foam::Field<Type>>
Foam::genericFaPatchField<Type>::valueInternalCoeffs
(
    const tmp<scalarField>&
) const
{
    FatalErrorInFunction
        << "Cannot be called for a generic patchField";

    genericFatalSolveError
    (
        this->patch().name(),
        this->internalField()
    );
    FatalError << abort(FatalError);

    return *this;
}


template<class Type>
Foam::tmp<Foam::Field<Type>>
Foam::genericFaPatchField<Type>::valueBoundaryCoeffs
(
    const tmp<scalarField>&
) const
{
    FatalErrorInFunction
        << "Cannot be called for a generic patchField";

    genericFatalSolveError
    (
        this->patch().name(),
        this->internalField()
    );
    FatalError << abort(FatalError);

    return *this;
}


template<class Type>
Foam::tmp<Foam::Field<Type>>
Foam::genericFaPatchField<Type>::gradientInternalCoeffs() const
{
    FatalErrorInFunction
        << "Cannot be called for a generic patchField";

    genericFatalSolveError
    (
        this->patch().name(),
        this->internalField()
    );
    FatalError << abort(FatalError);

    return *this;
}


template<class Type>
Foam::tmp<Foam::Field<Type>>
Foam::genericFaPatchField<Type>::gradientBoundaryCoeffs() const
{
    FatalErrorInFunction
        << "Cannot be called for a generic patchField";

    genericFatalSolveError
    (
        this->patch().name(),
        this->internalField()
    );
    FatalError << abort(FatalError);

    return *this;
}


// ************************************************************************* //
