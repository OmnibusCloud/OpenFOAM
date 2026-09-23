/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2017 OpenFOAM Foundation
    Copyright (C) 2022-2025 OpenCFD Ltd.
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

#include "lagrangianFieldDecomposer.H"
#include "IOobjectList.H"

// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class Type>
void Foam::lagrangianFieldDecomposer::readFields
(
    const label cloudi,
    const IOobjectList& lagrangianObjects,
    PtrList<PtrList<IOField<Type>>>& lagrangianFields
)
{
    // Lagrangian field objects
    const UPtrList<const IOobject> fieldObjects
    (
        lagrangianObjects.csorted<IOField<Type>>()
    );


    auto& cloudFields =
        lagrangianFields.emplace_set(cloudi, fieldObjects.size());

    forAll(fieldObjects, fieldi)
    {
        cloudFields.emplace_set(fieldi, fieldObjects[fieldi]);
    }
}


template<class Type>
void Foam::lagrangianFieldDecomposer::readFieldFields
(
    const label cloudi,
    const IOobjectList& lagrangianObjects,
    PtrList<PtrList<CompactIOField<Field<Type>>>>& lagrangianFields
)
{
    // Lagrangian field objects
    UPtrList<const IOobject> fieldObjects
    (
        lagrangianObjects.cobjects<IOField<Field<Type>>>()
    );

    // Lagrangian field-field objects
    fieldObjects.push_back
    (
        lagrangianObjects.cobjects<CompactIOField<Field<Type>>>()
    );

    Foam::sort(fieldObjects, nameOp<IOobject>());


    auto& cloudFields =
        lagrangianFields.emplace_set(cloudi, fieldObjects.size());

    forAll(fieldObjects, fieldi)
    {
        cloudFields.emplace_set(fieldi, fieldObjects[fieldi]);
    }
}


template<class Type>
Foam::tmp<Foam::IOField<Type>>
Foam::lagrangianFieldDecomposer::decomposeField
(
    const word& cloudName,
    const IOField<Type>& field
) const
{
    // Create the field for the processor
    return tmp<IOField<Type>>::New
    (
        IOobject
        (
            field.name(),
            procMesh_.time().timeName(),
            cloud::prefix/cloudName,
            procMesh_,
            IOobject::NO_READ,
            IOobject::NO_WRITE,
            IOobject::NO_REGISTER
        ),
        // Mapping internal field values
        Field<Type>(field, particleIndices_)
    );
}


template<class Type>
Foam::tmp<Foam::CompactIOField<Foam::Field<Type>>>
Foam::lagrangianFieldDecomposer::decomposeFieldField
(
    const word& cloudName,
    const CompactIOField<Field<Type>>& field
) const
{
    // Create the field for the processor
    return tmp<CompactIOField<Field<Type>>>::New
    (
        IOobject
        (
            field.name(),
            procMesh_.time().timeName(),
            cloud::prefix/cloudName,
            procMesh_,
            IOobject::NO_READ,
            IOobject::NO_WRITE,
            IOobject::NO_REGISTER
        ),
        // Mapping internal field values
        Field<Field<Type>>(field, particleIndices_)
    );
}


template<class GeoField>
void Foam::lagrangianFieldDecomposer::decomposeFields
(
    const word& cloudName,
    const UPtrList<GeoField>& fields
) const
{
    const bool existsOnProc = (!particleIndices_.empty());

    for (const GeoField& fld : fields)
    {
        decomposeField(cloudName, fld)().write(existsOnProc);
    }
}


template<class GeoField>
void Foam::lagrangianFieldDecomposer::decomposeFieldFields
(
    const word& cloudName,
    const UPtrList<GeoField>& fields
) const
{
    const bool existsOnProc = (!particleIndices_.empty());

    for (const GeoField& fld : fields)
    {
        decomposeFieldField(cloudName, fld)().write(existsOnProc);
    }
}


// ************************************************************************* //
