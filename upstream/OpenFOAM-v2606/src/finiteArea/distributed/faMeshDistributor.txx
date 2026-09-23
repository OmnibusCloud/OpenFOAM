/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2022-2026 OpenCFD Ltd.
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

#include "Time.H"
#include "faPatchFields.H"
#include "faePatchFields.H"
#include "IOobjectList.H"
#include "polyMesh.H"
#include "polyPatch.H"
#include "processorFaPatch.H"
#include "mapDistribute.H"
#include "mapDistributePolyMesh.H"
#include "areaFields.H"
#include "edgeFields.H"

#include "distributedFieldMapper.H"
#include "distributedFaPatchFieldMapper.H"

// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

template<class FieldType>
void Foam::faMeshDistributor::writeField(const FieldType& fld) const
{
    if (writeHandler_)
    {
        // Writing control via handler
        auto handler = writeHandler_.shallowClone();
        handler = fileOperation::fileHandler(handler);
        auto oldComm = UPstream::commWorld(fileHandler().comm());

        fld.write();

        // Restore
        (void)UPstream::commWorld(oldComm);
        (void)fileOperation::fileHandler(handler);
    }
    else if (isWriteProc_)
    {
        // Writing with bool control (uses current fileHandler)
        fld.write();
    }
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class Type>
Foam::tmp<Foam::GeometricField<Type, Foam::faPatchField, Foam::areaMesh>>
Foam::faMeshDistributor::distributeField
(
    const GeometricField<Type, faPatchField, areaMesh>& fld
) const
{
    if (tgtMesh_.boundary().size() && patchEdgeMaps_.empty())
    {
        createPatchMaps();
    }

    // Create internalField by remote mapping
    const distributedFieldMapper mapper
    (
        labelUList::null(),
        distMap_.cellMap()  // area: faceMap (volume: cellMap)
    );

    // The result (with placeholder patch fields)
    auto tresult = tmp<GeometricField<Type, faPatchField, areaMesh>>::New
    (
        DimensionedField<Type, areaMesh>
        (
            IOobject
            (
                fld.name(),
                tgtMesh_.time().timeName(),
                fld.local(),
                tgtMesh_.thisDb(),
                IOobject::NO_READ,
                IOobject::NO_WRITE
            ),
            tgtMesh_,
            fld.dimensions(),
            Field<Type>(fld.internalField(), mapper)
        ),
        UPtrList<faPatchField<Type>>()
    );

    tresult.ref().oriented() = fld.oriented();


    // Now do the boundaries
    const auto& oldPatchFields = fld.boundaryField();
    const auto nOldPatches = oldPatchFields.size();

    const auto& tgtInternal = tresult.ref().internalField();
    auto& boundaries = tresult.ref().boundaryFieldRef();

    boundaries.resize_null(tgtMesh_.boundary().size());

    forAll(boundaries, patchi)
    {
        const auto& tgtPatch = tgtMesh_.boundary()[patchi];

        if
        (
            const auto* patchMap = patchEdgeMaps_.get(patchi);
            (patchMap && patchi < nOldPatches)
        )
        {
            // Construct by mapping
            const distributedFaPatchFieldMapper mapper
            (
                labelUList::null(),
               *patchMap
            );

            boundaries.set
            (
                patchi,
                faPatchField<Type>::New
                (
                    oldPatchFields[patchi],
                    tgtPatch,
                    tgtInternal,
                    mapper
                )
            );
        }
        else
        {
            // Add non-mapped patchFields as "empty", but this will also
            // internally handle processor fields and any other constraint
            // type patches

            boundaries.set
            (
                patchi,
                faPatchField<Type>::New
                (
                    faPatchFieldBase::emptyType(),
                    tgtPatch,
                    tgtInternal
                )
            );
        }
    }

    boundaries.template evaluateCoupled<processorFaPatch>();

    return tresult;
}


template<class Type>
Foam::tmp<Foam::GeometricField<Type, Foam::faePatchField, Foam::edgeMesh>>
Foam::faMeshDistributor::distributeField
(
    const GeometricField<Type, faePatchField, edgeMesh>& fld
) const
{
    // Create internalField by remote mapping
    const distributedFieldMapper mapper
    (
        labelUList::null(),
        distMap_.faceMap()  // area: edgeMap (volume: faceMap)
    );

    const auto internalSize = tgtMesh_.nInternalEdges();

    Field<Type> primitiveField;
    Field<Type> flatBoundary;
    {
        // Create flat field of internalField + all patch fields
        Field<Type> fullField(fld.mesh().nEdges(), Foam::zero{});

        // Internal field
        fullField.slice(0, fld.primitiveField().size()) = fld.primitiveField();

        // Boundary fields
        for (const auto& pfld : fld.boundaryField())
        {
            fullField.slice(pfld.patch().start(), pfld.size()) = pfld;
        }

        // Map all edges
        primitiveField = Field<Type>(fullField, mapper, fld.is_oriented());

        // Extract boundary values, trim internal to the correct size
        if (internalSize < primitiveField.size())
        {
            // Boundary values
            flatBoundary = primitiveField.slice(internalSize);

            // Internal values
            primitiveField.resize(internalSize);
        }
    }

    // The result (with placeholder patch fields)
    auto tresult = tmp<GeometricField<Type, faePatchField, edgeMesh>>::New
    (
        DimensionedField<Type, edgeMesh>
        (
            IOobject
            (
                fld.name(),
                tgtMesh_.time().timeName(),
                fld.local(),
                tgtMesh_.thisDb(),
                IOobject::NO_READ,
                IOobject::NO_WRITE
            ),
            tgtMesh_,
            fld.dimensions(),
            std::move(primitiveField)
        ),
        UPtrList<faePatchField<Type>>()
    );

    tresult.ref().oriented() = fld.oriented();


    // Now do the boundaries
    const auto& oldPatchFields = fld.boundaryField();
    const auto nOldPatches = oldPatchFields.size();

    const auto& tgtInternal = tresult().internalField();
    auto& boundaries = tresult.ref().boundaryFieldRef();

    boundaries.resize_null(tgtMesh_.boundary().size());

    label boundaryStart = 0;

    forAll(boundaries, patchi)
    {
        const auto& tgtPatch = tgtMesh_.boundary()[patchi];
        const auto count = tgtPatch.nEdges();

        if
        (
            const auto* patchMap = patchEdgeMaps_.get(patchi);
            (patchMap && patchi < nOldPatches)
        )
        {
            // Construct by mapping
            const distributedFaPatchFieldMapper mapper
            (
                labelUList::null(),
                *patchMap
            );

            boundaries.set
            (
                patchi,
                faePatchField<Type>::New
                (
                    oldPatchFields[patchi],
                    tgtPatch,
                    tgtInternal,
                    mapper
                )
            );
        }
        else
        {
            // Add non-mapped patchFields as "empty", but this will also
            // internally handle processor fields and any other constraint
            // type patches

            boundaries.set
            (
                patchi,
                faePatchField<Type>::New
                (
                    faePatchFieldBase::emptyType(),
                    tgtPatch,
                    tgtInternal
                )
            );
        }

        auto& pfld = boundaries[patchi];

        // Slight hack - copy the mapped internalField values to the
        // processor patches. These are otherwise not initialized.

        if (isA<processorFaPatch>(tgtPatch))
        {
            pfld = flatBoundary.slice(boundaryStart, pfld.size());
        }
        boundaryStart += count;
    }

    return tresult;
}


template<class Type>
Foam::tmp<Foam::GeometricField<Type, Foam::faPatchField, Foam::areaMesh>>
Foam::faMeshDistributor::distributeAreaField
(
    const IOobject& fieldObject
) const
{
    // Read field
    GeometricField<Type, faPatchField, areaMesh> fld
    (
        fieldObject,
        srcMesh_
    );

    // Redistribute
    return distributeField(fld);
}


template<class Type>
Foam::tmp<Foam::GeometricField<Type, Foam::faePatchField, Foam::edgeMesh>>
Foam::faMeshDistributor::distributeEdgeField
(
    const IOobject& fieldObject
) const
{
    // Read field
    GeometricField<Type, faePatchField, edgeMesh> fld
    (
        fieldObject,
        srcMesh_
    );

    // Redistribute
    return distributeField(fld);
}


template<class Type>
Foam::label Foam::faMeshDistributor::distributeAreaFields
(
    const IOobjectList& objects,
    const wordRes& selectedFields
) const
{
    typedef GeometricField<Type, faPatchField, areaMesh> fieldType;

    label nFields = 0;

    for
    (
        const IOobject& io :
        (
            selectedFields.empty()
          ? objects.csorted<fieldType>()
          : objects.csorted<fieldType>(selectedFields)
        )
    )
    {
        if (verbose_)
        {
            if (!nFields)
            {
                Info<< "    Distributing "
                    << fieldType::typeName << "s\n" << nl;
            }
            Info<< "        " << io.name() << nl;
        }
        ++nFields;

        tmp<fieldType> tfld(distributeAreaField<Type>(io));

        writeField(tfld());
    }

    if (nFields && verbose_) Info<< endl;
    return nFields;
}


template<class Type>
Foam::label Foam::faMeshDistributor::distributeEdgeFields
(
    const IOobjectList& objects,
    const wordRes& selectedFields
) const
{
    typedef GeometricField<Type, faePatchField, edgeMesh> fieldType;

    label nFields = 0;

    for
    (
        const IOobject& io :
        (
            selectedFields.empty()
          ? objects.csorted<fieldType>()
          : objects.csorted<fieldType>(selectedFields)
        )
    )
    {
        if (verbose_)
        {
            if (!nFields)
            {
                Info<< "    Distributing "
                    << fieldType::typeName << "s\n" << nl;
            }
            Info<< "        " << io.name() << nl;
        }
        ++nFields;

        tmp<fieldType> tfld(distributeEdgeField<Type>(io));

        writeField(tfld());
    }

    if (nFields && verbose_) Info<< endl;
    return nFields;
}


template<class Type>
void Foam::faMeshDistributor::redistributeAndWrite
(
    UPtrList<GeometricField<Type, faPatchField, areaMesh>>& flds
) const
{
    using GeoField = GeometricField<Type, faPatchField, areaMesh>;

    for (auto& fld : flds)
    {
        tmp<GeoField> tfld(this->distributeField(fld));

        writeField(tfld());
    }
}


template<class Type>
void Foam::faMeshDistributor::redistributeAndWrite
(
    UPtrList<GeometricField<Type, faePatchField, edgeMesh>>& flds
) const
{
    using GeoField = GeometricField<Type, faePatchField, edgeMesh>;

    for (auto& fld : flds)
    {
        tmp<GeoField> tfld(this->distributeField(fld));

        writeField(tfld());
    }
}


// ************************************************************************* //
