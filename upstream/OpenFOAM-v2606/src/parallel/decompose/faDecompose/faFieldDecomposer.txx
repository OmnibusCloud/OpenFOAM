/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2016-2017 Wikki Ltd
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

#include "faFieldDecomposer.H"
#include "GeometricField.H"
#include "processorFaPatchField.H"
#include "processorFaePatchField.H"

// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class Type>
Foam::tmp<Foam::GeometricField<Type, Foam::faPatchField, Foam::areaMesh>>
Foam::faFieldDecomposer::decomposeField
(
    const GeometricField<Type, faPatchField, areaMesh>& field
) const
{
    // Create the field for the processor
    // - with dummy patch fields
    auto tresult = GeometricField<Type, faPatchField, areaMesh>::New
    (
        field.name(),
        IOobject::NO_REGISTER,
        procMesh_,
        field.dimensions(),
        // Internal field - mapped values
        Field<Type>(field.primitiveField(), faceAddressing_),
        #if (OPENFOAM <= 2512)
        faPatchFieldBase::calculatedType()
        #else
        UPtrList<faPatchField<Type>>()
        #endif
    );
    auto& result = tresult.ref();
    result.oriented() = field.oriented();

    // Now do the boundaries
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
                faPatchField<Type>::New
                (
                    origPatchFields[oldPatchi],
                    tgtPatch,
                    tgtInternal,
                    patchFieldDecomposers_[patchi]
                )
            );
        }
        else
        {
            boundaries.set
            (
                patchi,
                new processorFaPatchField<Type>
                (
                    tgtPatch,
                    tgtInternal,
                    Field<Type>
                    (
                        field.internalField(),
                        processorAreaPatchFieldDecomposers_[patchi]
                    )
                )
            );
        }
    }

    return tresult;
}


template<class Type>
Foam::tmp<Foam::GeometricField<Type, Foam::faePatchField, Foam::edgeMesh>>
Foam::faFieldDecomposer::decomposeField
(
    const GeometricField<Type, faePatchField, edgeMesh>& field
) const
{
    labelList mapAddr
    (
        edgeAddressing_.slice(0, procMesh_.nInternalEdges())
    );

    if (!noEdgeEncoding_)
    {
        for (auto& addr : mapAddr)
        {
            addr -= 1;
        }
    }

    // Problem with addressing when a processor patch picks up both internal
    // edges and edges from cyclic boundaries. This is a bit of a hack, but
    // I cannot find a better solution without making the internal storage
    // mechanism for edgeFields correspond to the one of edges in polyMesh
    // (i.e. using slices)

    // Same as faMeshTools::flattenEdgeField()
    Field<Type> fullField(field.mesh().nEdges());
    {
        // Internal field
        fullField.slice(0, field.size()) = field.primitiveField();

        // Boundary fields
        fullField.slice(field.mesh().nInternalEdges()) = Foam::zero{};
        for (const auto& pfld : field.boundaryField())
        {
            fullField.slice(pfld.patch().start(), pfld.size()) = pfld;
        }
    }


    // Create the field for the processor
    // - with dummy patch fields
    auto tresult = GeometricField<Type, faePatchField, edgeMesh>::New
    (
        field.name(),
        IOobject::NO_REGISTER,
        procMesh_,
        field.dimensions(),
        // Internal field - mapped values
        Field<Type>(field.internalField(), mapAddr),
        #if (OPENFOAM <= 2512)
        faePatchFieldBase::calculatedType()
        #else
        UPtrList<faePatchField<Type>>()
        #endif
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
        #if (OPENFOAM <= 2512)
        // HACK (2026-02-16) - edge fields are currently not marked
        // as oriented, but mostly have "phi", which is oriented.
        bool applyFlips = true;
        #else
        bool applyFlips = result.is_oriented();
        #endif

        const auto& tgtPatch = procMesh_.boundary()[patchi];
        const auto oldPatchi = boundaryAddressing_[patchi];

        if (oldPatchi >= 0 && patchFieldDecomposers_.test(patchi))
        {
            applyFlips = false;  // No field flipping (local mapping)
            boundaries.set
            (
                patchi,
                faePatchField<Type>::New
                (
                    origPatchFields[oldPatchi],
                    tgtPatch,
                    tgtInternal,
                    patchFieldDecomposers_[patchi]
                )
            );
        }
        else
        {
            boundaries.set
            (
                patchi,
                new processorFaePatchField<Type>
                (
                    tgtPatch,
                    tgtInternal,
                    Field<Type>
                    (
                        fullField,
                        processorEdgePatchFieldDecomposers_[patchi]
                    )
                )
            );
        }

        if (applyFlips && edgeSigns_.test(patchi))
        {
            boundaries[patchi] *= edgeSigns_[patchi];
        }
    }

    return tresult;
}


template<class GeoField>
void Foam::faFieldDecomposer::decomposeFields
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
