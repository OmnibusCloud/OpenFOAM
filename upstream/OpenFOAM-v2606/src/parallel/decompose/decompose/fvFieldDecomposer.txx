/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
    Copyright (C) 2021-2026 OpenCFD Ltd.
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

#include "fvFieldDecomposer.H"
#include "emptyFvPatchFields.H"
#include "processorFvPatchField.H"
#include "processorFvsPatchField.H"
#include "processorCyclicFvPatchField.H"
#include "processorCyclicFvsPatchField.H"
#include "volFields.H"
#include "surfaceFields.H"

// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class Type>
Foam::tmp<Foam::DimensionedField<Type, Foam::volMesh>>
Foam::fvFieldDecomposer::decomposeField
(
    const DimensionedField<Type, volMesh>& field
) const
{
    // Create the field for the processor

    return DimensionedField<Type, volMesh>::New
    (
        field.name(),
        IOobject::NO_REGISTER,
        procMesh_,
        field.dimensions(),
        // Internal field - mapped values
        Field<Type>(field.field(), cellAddressing_)
    );
}


template<class Type>
Foam::tmp<Foam::GeometricField<Type, Foam::fvPatchField, Foam::volMesh>>
Foam::fvFieldDecomposer::decomposeField
(
    const GeometricField<Type, fvPatchField, volMesh>& field,
    const bool allowUnknownPatchFields
) const
{
    // Create the field for the processor
    // - with dummy patch fields
    auto tresult = GeometricField<Type, fvPatchField, volMesh>::New
    (
        field.name(),
        IOobject::NO_REGISTER,
        procMesh_,
        field.dimensions(),
        // Internal field - mapped values
        Field<Type>(field.primitiveField(), cellAddressing_),
        fvPatchFieldBase::calculatedType()
    );
    auto& result = tresult.ref();
    result.oriented() = field.oriented();


    // Now redo the boundaries
    const auto& origPatchFields = field.boundaryField();
    //const auto nOldPatches = origPatchFields.size();

    const auto& tgtInternal = result.internalField();
    auto& boundaries = result.boundaryFieldRef();

    boundaries.resize_null(procMesh_.boundary().size());

    forAll(boundaries, patchi)
    {
        const auto& tgtPatch = procMesh_.boundary()[patchi];
        const auto oldPatchi = boundaryAddressing_[patchi];

        if (oldPatchi >= 0 && patchFieldDecomposers_.test(patchi))
        {
            boundaries.set
            (
                patchi,
                fvPatchField<Type>::New
                (
                    origPatchFields[oldPatchi],
                    tgtPatch,
                    tgtInternal,
                    patchFieldDecomposers_[patchi]
                )
            );
        }
        else if (isA<processorCyclicFvPatch>(tgtPatch))
        {
            boundaries.set
            (
                patchi,
                new processorCyclicFvPatchField<Type>
                (
                    tgtPatch,
                    tgtInternal,
                    Field<Type>
                    (
                        field.primitiveField(),
                        processorVolPatchFieldDecomposers_[patchi]
                    )
                )
            );
        }
        else if (isA<processorFvPatch>(tgtPatch))
        {
            boundaries.set
            (
                patchi,
                new processorFvPatchField<Type>
                (
                    tgtPatch,
                    tgtInternal,
                    Field<Type>
                    (
                        field.primitiveField(),
                        processorVolPatchFieldDecomposers_[patchi]
                    )
                )
            );
        }
        else if (allowUnknownPatchFields)
        {
            boundaries.set
            (
                patchi,
                new emptyFvPatchField<Type>(tgtPatch, tgtInternal)
            );
        }
        else
        {
            FatalErrorInFunction
                << "Unknown type." << abort(FatalError);
        }
    }

    // Create the field for the processor
    return tresult;
}


template<class Type>
Foam::tmp<Foam::GeometricField<Type, Foam::fvsPatchField, Foam::surfaceMesh>>
Foam::fvFieldDecomposer::decomposeField
(
    const GeometricField<Type, fvsPatchField, surfaceMesh>& field
) const
{
    labelList mapAddr
    (
        faceAddressing_.slice(0, procMesh_.nInternalFaces())
    );
    forAll(mapAddr, i)
    {
        mapAddr[i] -= 1;
    }


    // Problem with addressing when a processor patch picks up both internal
    // faces and faces from cyclic boundaries. This is a bit of a hack, but
    // I cannot find a better solution without making the internal storage
    // mechanism for surfaceFields correspond to the one of faces in polyMesh
    // (i.e. using slices)

    Field<Type> fullField(field.mesh().nFaces());
    {
        // Internal field
        fullField.slice(0, field.size()) = field.primitiveField();

        // Boundary fields
        fullField.slice(field.mesh().nInternalFaces()) = Foam::zero{};
        for (const auto& pfld : field.boundaryField())
        {
            fullField.slice(pfld.patch().start(), pfld.size()) = pfld;
        }
    }


    // Create the field for the processor
    // - with dummy patch fields
    auto tresult = GeometricField<Type, fvsPatchField, surfaceMesh>::New
    (
        field.name(),
        IOobject::NO_REGISTER,
        procMesh_,
        field.dimensions(),
        // Internal field - mapped values
        Field<Type>(field.primitiveField(), mapAddr),
        fvsPatchFieldBase::calculatedType()
    );
    auto& result = tresult.ref();
    result.oriented() = field.oriented();


    // Now redo the boundaries
    const auto& origPatchFields = field.boundaryField();
    //const auto nOldPatches = origPatchFields.size();

    const auto& tgtInternal = result.internalField();
    auto& boundaries = result.boundaryFieldRef();

    boundaries.resize_null(procMesh_.boundary().size());

    forAll(boundaries, patchi)
    {
        bool applyFlips = result.is_oriented();

        const auto& tgtPatch = procMesh_.boundary()[patchi];
        const auto oldPatchi = boundaryAddressing_[patchi];

        if (oldPatchi >= 0 && patchFieldDecomposers_.test(patchi))
        {
            applyFlips = false;  // No field flipping (local mapping)
            boundaries.set
            (
                patchi,
                fvsPatchField<Type>::New
                (
                    origPatchFields[oldPatchi],
                    tgtPatch,
                    tgtInternal,
                    patchFieldDecomposers_[patchi]
                )
            );
        }
        else if (isA<processorCyclicFvPatch>(tgtPatch))
        {
            boundaries.set
            (
                patchi,
                new processorCyclicFvsPatchField<Type>
                (
                    tgtPatch,
                    tgtInternal,
                    Field<Type>
                    (
                        fullField,
                        processorSurfacePatchFieldDecomposers_[patchi]
                    )
                )
            );
        }
        else if (isA<processorFvPatch>(tgtPatch))
        {
            boundaries.set
            (
                patchi,
                new processorFvsPatchField<Type>
                (
                    tgtPatch,
                    tgtInternal,
                    Field<Type>
                    (
                        fullField,
                        processorSurfacePatchFieldDecomposers_[patchi]
                    )
                )
            );
        }
        else
        {
            applyFlips = false;
            FatalErrorInFunction
                << "Unknown type." << abort(FatalError);
        }

        if (applyFlips && faceSigns_.test(patchi))
        {
            boundaries[patchi] *= faceSigns_[patchi];
        }
    }

    // Create the field for the processor
    return tresult;
}


template<class GeoField>
void Foam::fvFieldDecomposer::decomposeFields
(
    const UPtrList<GeoField>& fields
) const
{
    for (const auto& fld : fields)
    {
        decomposeField(fld)().write();
    }
}


// ************************************************************************* //
