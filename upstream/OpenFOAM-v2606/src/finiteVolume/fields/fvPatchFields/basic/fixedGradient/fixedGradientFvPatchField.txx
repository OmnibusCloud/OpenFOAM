/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
    Copyright (C) 2023 OpenCFD Ltd.
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

#include "fixedGradientFvPatchField.H"
#include "dictionary.H"

// * * * * * * * * * * * * Protected Member Functions  * * * * * * * * * * * //

template<class Type>
bool Foam::fixedGradientFvPatchField<Type>::readGradientEntry
(
    const dictionary& dict,
    IOobjectOption::readOption readOpt
)
{
    if (!IOobjectOption::isAnyRead(readOpt)) return false;
    const auto& p = fvPatchFieldBase::patch();


    const auto* eptr = dict.findEntry("gradient", keyType::LITERAL);

    if (eptr)
    {
        gradient_.assign(*eptr, p.size());
        return true;
    }

    if (IOobjectOption::isReadRequired(readOpt))
    {
        FatalIOErrorInFunction(dict)
            << "Required entry 'gradient' : missing for patch " << p.name()
            << " in dictionary " << dict.relativeName() << nl
            << exit(FatalIOError);
    }

    return false;
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class Type>
Foam::fixedGradientFvPatchField<Type>::fixedGradientFvPatchField
(
    const fvPatch& p,
    const DimensionedField<Type, volMesh>& iF
)
:
    parent_bctype(p, iF),
    gradient_(p.size(), Zero)
{}


template<class Type>
Foam::fixedGradientFvPatchField<Type>::fixedGradientFvPatchField
(
    const fvPatch& p,
    const DimensionedField<Type, volMesh>& iF,
    const dictionary& dict,
    IOobjectOption::readOption requireGrad
)
:
    parent_bctype(p, iF, dict, IOobjectOption::NO_READ),
    gradient_(p.size())
{
    if (readGradientEntry(dict, requireGrad))
    {
        evaluate();
    }
    else
    {
        // Not read (eg, optional and missing):
        // - treat as zero-gradient, do not evaluate
        fvPatchField<Type>::extrapolateInternal();
        gradient_ = Zero;
    }
}


template<class Type>
Foam::fixedGradientFvPatchField<Type>::fixedGradientFvPatchField
(
    const this_bctype& ptf,
    const fvPatch& p,
    const DimensionedField<Type, volMesh>& iF,
    const fvPatchFieldMapper& mapper
)
:
    parent_bctype(ptf, p, iF, mapper),
    gradient_(ptf.gradient_, mapper)
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
Foam::fixedGradientFvPatchField<Type>::fixedGradientFvPatchField
(
    const this_bctype& ptf,
    const DimensionedField<Type, volMesh>& iF
)
:
    parent_bctype(ptf, iF),
    gradient_(ptf.gradient_)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class Type>
void Foam::fixedGradientFvPatchField<Type>::autoMap
(
    const fvPatchFieldMapper& m
)
{
    this->parent_bctype::autoMap(m);
    gradient_.autoMap(m);
}


template<class Type>
void Foam::fixedGradientFvPatchField<Type>::rmap
(
    const fvPatchField<Type>& ptf,
    const labelList& addr
)
{
    this->parent_bctype::rmap(ptf, addr);

    const auto& fgptf = refCast<const this_bctype>(ptf);

    gradient_.rmap(fgptf.gradient_, addr);
}


template<class Type>
void Foam::fixedGradientFvPatchField<Type>::evaluate(const Pstream::commsTypes)
{
    if (!this->updated())
    {
        this->updateCoeffs();
    }

    Field<Type>::operator=
    (
        this->patchInternalField() + gradient_/this->patch().deltaCoeffs()
    );

    fvPatchField<Type>::evaluate();
}


template<class Type>
Foam::tmp<Foam::Field<Type>>
Foam::fixedGradientFvPatchField<Type>::valueInternalCoeffs
(
    const tmp<scalarField>&
) const
{
    return tmp<Field<Type>>::New(this->size(), pTraits<Type>::one);
}


template<class Type>
Foam::tmp<Foam::Field<Type>>
Foam::fixedGradientFvPatchField<Type>::valueBoundaryCoeffs
(
    const tmp<scalarField>&
) const
{
    return gradient()/this->patch().deltaCoeffs();
}


template<class Type>
Foam::tmp<Foam::Field<Type>>
Foam::fixedGradientFvPatchField<Type>::gradientInternalCoeffs() const
{
    return tmp<Field<Type>>::New(this->size(), Zero);
}


template<class Type>
Foam::tmp<Foam::Field<Type>>
Foam::fixedGradientFvPatchField<Type>::gradientBoundaryCoeffs() const
{
    return gradient();
}


template<class Type>
void Foam::fixedGradientFvPatchField<Type>::write(Ostream& os) const
{
    this->parent_bctype::write(os);
    gradient_.writeEntry("gradient", os);
}


// ************************************************************************* //
