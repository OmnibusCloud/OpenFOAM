/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2015 OpenFOAM Foundation
    Copyright (C) 2016-2026 OpenCFD Ltd.
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

#include "parFvFieldDistributor.H"
#include "Time.H"
#include "PtrList.H"
#include "fvPatchFields.H"
#include "fvsPatchFields.H"
#include "emptyFvPatch.H"
#include "IOobjectList.H"
#include "mapDistributePolyMesh.H"
#include "processorFvPatch.H"

#include "distributedFieldMapper.H"
#include "distributedFvPatchFieldMapper.H"

// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

template<class FieldType>
void Foam::parFvFieldDistributor::writeField(const FieldType& fld) const
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
Foam::tmp<Foam::DimensionedField<Type, Foam::volMesh>>
Foam::parFvFieldDistributor::distributeField
(
    const DimensionedField<Type, volMesh>& fld
) const
{
    // Create internalField by remote mapping
    const distributedFieldMapper mapper
    (
        labelUList::null(),
        distMap_.cellMap()
    );

    auto tfield = tmp<DimensionedField<Type, volMesh>>::New
    (
        IOobject
        (
            fld.name(),
            tgtMesh_.time().timeName(),
            fld.local(),
            tgtMesh_,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        tgtMesh_,
        fld.dimensions(),
        Field<Type>(fld, mapper)
    );

    tfield.ref().oriented() = fld.oriented();

    return tfield;
}


template<class Type>
Foam::tmp<Foam::GeometricField<Type, Foam::fvPatchField, Foam::volMesh>>
Foam::parFvFieldDistributor::distributeField
(
    const GeometricField<Type, fvPatchField, volMesh>& fld
) const
{
    // Create internalField by remote mapping
    const distributedFieldMapper mapper
    (
        labelUList::null(),
        distMap_.cellMap()
    );

    DimensionedField<Type, volMesh> internalField
    (
        IOobject
        (
            fld.name(),
            tgtMesh_.time().timeName(),
            fld.local(),
            tgtMesh_,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        tgtMesh_,
        fld.dimensions(),
        Field<Type>(fld.internalField(), mapper)
    );

    internalField.oriented() = fld.oriented();


    // Create patchFields by remote mapping
    // Note: patchFields still on source mesh, not target mesh

    PtrList<fvPatchField<Type>> oldPatchFields(fld.mesh().boundary().size());

    const auto& bfld = fld.boundaryField();

    forAll(bfld, patchi)
    {
        if (patchFaceMaps_.set(patchi))
        {
            distributedFvPatchFieldMapper mapper
            (
                labelUList::null(),
                patchFaceMaps_[patchi]
            );

            // Clone local patch field
            oldPatchFields.set
            (
                patchi,
                bfld[patchi].clone(fld.internalField())
            );

            // Map into local copy
            oldPatchFields[patchi].autoMap(mapper);
        }
    }


    // Clone the oldPatchFields onto the target patches. This is just to reset
    // the reference to the patch, size and content stay the same.

    PtrList<fvPatchField<Type>> newPatchFields(tgtMesh_.boundary().size());

    forAll(oldPatchFields, patchi)
    {
        if (auto pfldPtr = oldPatchFields.release(patchi); pfldPtr)
        {
            const auto& pfld = pfldPtr();

            labelList dummyMap(identity(pfld.size()));
            directFvPatchFieldMapper dummyMapper(dummyMap);

            newPatchFields.set
            (
                patchi,
                fvPatchField<Type>::New
                (
                    pfld,
                    tgtMesh_.boundary()[patchi],
                    fvPatchField<Type>::Internal::null(),
                    dummyMapper
                )
            );
        }
    }

    // Add some empty patches on remaining patches
    // (... probably processor patches)

    forAll(newPatchFields, patchi)
    {
        if (!newPatchFields.set(patchi))
        {
            newPatchFields.set
            (
                patchi,
                fvPatchField<Type>::New
                (
                    fvPatchFieldBase::emptyType(),
                    tgtMesh_.boundary()[patchi],
                    fvPatchField<Type>::Internal::null()
                )
            );
        }
    }

    // Return geometric field

    return tmp<GeometricField<Type, fvPatchField, volMesh>>::New
    (
        std::move(internalField),
        newPatchFields
    );
}


template<class Type>
Foam::tmp<Foam::GeometricField<Type, Foam::fvsPatchField, Foam::surfaceMesh>>
Foam::parFvFieldDistributor::distributeField
(
    const GeometricField<Type, fvsPatchField, surfaceMesh>& fld
) const
{
    // Create internalField by remote mapping
    const distributedFieldMapper mapper
    (
        labelUList::null(),
        distMap_.faceMap()
    );


    const auto internalSize = tgtMesh_.nInternalFaces();

    Field<Type> primitiveField;
    //Field<Type> flatBoundary;
    {
        // Create flat field of internalField + all patch fields
        Field<Type> fullField(fld.mesh().nFaces(), Foam::zero{});

        // Internal field
        fullField.slice(0, fld.internalField().size()) = fld.internalField();

        for (const auto& pfld : fld.boundaryField())
        {
            fullField.slice(pfld.patch().start(), pfld.size()) = pfld;
        }

        // Map all faces
        primitiveField = Field<Type>(fullField, mapper, fld.is_oriented());

        // Trim to internal faces (note: could also have special mapper)
        if (internalSize < primitiveField.size())
        {
            // // Save boundary values
            // flatBoundary = primitiveField.slice(internalSize);

            // Internal values
            primitiveField.resize(internalSize);
        }
    }


    DimensionedField<Type, surfaceMesh> internalField
    (
        IOobject
        (
            fld.name(),
            tgtMesh_.time().timeName(),
            fld.local(),
            tgtMesh_,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        tgtMesh_,
        fld.dimensions(),
        std::move(primitiveField)
    );

    internalField.oriented() = fld.oriented();


    // Create patchFields by remote mapping
    // Note: patchFields still on source mesh, not target mesh

    PtrList<fvsPatchField<Type>> oldPatchFields(fld.mesh().boundary().size());

    const auto& bfld = fld.boundaryField();

    forAll(bfld, patchi)
    {
        if (patchFaceMaps_.set(patchi))
        {
            distributedFvPatchFieldMapper mapper
            (
                labelUList::null(),
                patchFaceMaps_[patchi]
            );

            // Clone local patch field
            oldPatchFields.set
            (
                patchi,
                bfld[patchi].clone(fld.internalField())
            );

            // Map into local copy
            oldPatchFields[patchi].autoMap(mapper);
        }
    }


    PtrList<fvsPatchField<Type>> newPatchFields(tgtMesh_.boundary().size());

    // Clone the patchFields onto the base patches. This is just to reset
    // the reference to the patch, size and content stay the same.
    forAll(oldPatchFields, patchi)
    {
        if (auto pfldPtr = oldPatchFields.release(patchi); pfldPtr)
        {
            const auto& pfld = pfldPtr();

            labelList dummyMap(identity(pfld.size()));
            directFvPatchFieldMapper dummyMapper(dummyMap);

            newPatchFields.set
            (
                patchi,
                fvsPatchField<Type>::New
                (
                    pfld,
                    tgtMesh_.boundary()[patchi],
                    fvsPatchField<Type>::Internal::null(),
                    dummyMapper
                )
            );
        }
    }

    // Add some empty patches on remaining patches
    // (... probably processor patches)
    forAll(newPatchFields, patchi)
    {
        if (!newPatchFields.set(patchi))
        {
            newPatchFields.set
            (
                patchi,
                fvsPatchField<Type>::New
                (
                    fvsPatchFieldBase::emptyType(),
                    tgtMesh_.boundary()[patchi],
                    fvsPatchField<Type>::Internal::null()
                )
            );
        }
    }


    // Return geometric field
    return tmp<GeometricField<Type, fvsPatchField, surfaceMesh>>::New
    (
        std::move(internalField),
        newPatchFields
    );
}


template<class Type>
Foam::tmp<Foam::DimensionedField<Type, Foam::volMesh>>
Foam::parFvFieldDistributor::distributeInternalField
(
    const IOobject& fieldObject
) const
{
    // Read field
    DimensionedField<Type, volMesh> fld
    (
        fieldObject,
        srcMesh_
    );

    // Distribute
    return distributeField(fld);
}


template<class Type>
Foam::tmp<Foam::GeometricField<Type, Foam::fvPatchField, Foam::volMesh>>
Foam::parFvFieldDistributor::distributeVolumeField
(
    const IOobject& fieldObject
) const
{
    // Read field
    GeometricField<Type, fvPatchField, volMesh> fld
    (
        fieldObject,
        srcMesh_
    );

    // Distribute
    return distributeField(fld);
}


template<class Type>
Foam::tmp<Foam::GeometricField<Type, Foam::fvsPatchField, Foam::surfaceMesh>>
Foam::parFvFieldDistributor::distributeSurfaceField
(
    const IOobject& fieldObject
) const
{
    // Read field
    GeometricField<Type, fvsPatchField, surfaceMesh> fld
    (
        fieldObject,
        srcMesh_
    );

    // Distribute
    return distributeField(fld);
}


template<class Type>
Foam::label Foam::parFvFieldDistributor::distributeInternalFields
(
    const IOobjectList& objects,
    const wordRes& selectedFields
) const
{
    typedef DimensionedField<Type, volMesh> fieldType;

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
        if ("cellDist" == io.name())
        {
            // Ignore cellDist (internal or volume) field
            continue;
        }
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

        tmp<fieldType> tfld
        (
            distributeInternalField<Type>(io)
        );

        writeField(tfld());
    }

    if (nFields && verbose_) Info<< endl;
    return nFields;
}


template<class Type>
Foam::label Foam::parFvFieldDistributor::distributeVolumeFields
(
    const IOobjectList& objects,
    const wordRes& selectedFields
) const
{
    typedef GeometricField<Type, fvPatchField, volMesh> fieldType;

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
        if ("cellDist" == io.name())
        {
            // Ignore cellDist (internal or volume) field
            continue;
        }
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

        tmp<fieldType> tfld
        (
            distributeVolumeField<Type>(io)
        );

        writeField(tfld());
    }

    if (nFields && verbose_) Info<< endl;
    return nFields;
}


template<class Type>
Foam::label Foam::parFvFieldDistributor::distributeSurfaceFields
(
    const IOobjectList& objects,
    const wordRes& selectedFields
) const
{
    typedef GeometricField<Type, fvsPatchField, surfaceMesh> fieldType;

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

        tmp<fieldType> tfld
        (
            distributeSurfaceField<Type>(io)
        );

        writeField(tfld());
    }

    if (nFields && verbose_) Info<< endl;
    return nFields;
}


// ************************************************************************* //
