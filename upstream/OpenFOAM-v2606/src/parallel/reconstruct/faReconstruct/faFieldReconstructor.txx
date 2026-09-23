/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2016-2017 Wikki Ltd
    Copyright (C) 2018-2026 OpenCFD Ltd.
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

#include "faFieldReconstructor.H"
#include "Time.H"
#include "emptyFaPatch.H"
#include "faPatchFields.H"
#include "faePatchFields.H"

// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class Type>
Foam::tmp<Foam::GeometricField<Type, Foam::faPatchField, Foam::areaMesh>>
Foam::faFieldReconstructor::reconstructField
(
    const IOobject& fieldObject,
    const UPtrList<GeometricField<Type, faPatchField, areaMesh>>& procFields
) const
{
    // Construct field with blank slate for internal, and with null boundaries
    auto tfield = tmp<GeometricField<Type, faPatchField, areaMesh>>::New
    (
        fieldObject,
        mesh_,
        procFields[0].dimensions(),
        Field<Type>(mesh_.nFaces()),
        UPtrList<faPatchField<Type>>()
    );

    tfield.ref().oriented() = procFields[0].oriented();

    auto& tgtInternal = tfield.ref().internalFieldRef();
    auto& boundaries = tfield.ref().boundaryFieldRef();

    boundaries.resize_null(mesh_.boundary().size());  // extra safety

    // The patch starts (global mesh)
    const labelList gStarts(mesh_.boundary().patchStarts());

    forAll(procMeshes_, proci)
    {
        const auto& procField = procFields[proci];
        const auto& procMesh = procMeshes_[proci];

        // The (edge,face,boundary)ProcAddressing for the current procMesh
        const auto& edgeProcAddr = edgeProcAddressing_[proci];
        const auto& faceProcAddr = faceProcAddressing_[proci];
        const auto& boundaryProcAddr = boundaryProcAddressing_[proci];

        // Set the face values in the reconstructed field
        tgtInternal.rmap(procField.primitiveField(), faceProcAddr);


        // The patch starts (local mesh)
        const labelList starts(procMesh.boundary().patchStarts());

        // Set the boundary patch values in the reconstructed field

        forAll(boundaryProcAddr, patchi)
        {
            // Get addressing slice for this patch
//             const auto cp =
//                 procMesh.boundary()[patchi].patchSlice(edgeProcAddr);
            const auto cp =
                edgeProcAddr.slice
                (
                    starts[patchi],
                    procMesh.boundary()[patchi].size()
                );


            // Get patch index of the original patch,
            // check if the boundary patch is not a processor patch
            if
            (
                const auto tgtPatchi = boundaryProcAddr[patchi];
                (tgtPatchi >= 0)
            )
            {
                // Regular patch. Fast looping

                const auto& oldPatchField = procField.boundaryField()[patchi];

                if (!boundaries.test(tgtPatchi))
                {
                    boundaries.set
                    (
                        tgtPatchi,
                        faPatchField<Type>::New
                        (
                            oldPatchField,
                            mesh_.boundary()[tgtPatchi],
                            tgtInternal,
                            faPatchFieldReconstructor
                            (
                                mesh_.boundary()[tgtPatchi].size(),
                                oldPatchField.size()
                            )
                        )
                    );
                }

                const label tgtPatchStart = gStarts[tgtPatchi];
//                     mesh_.boundary()[tgtPatchi].start();

                labelList reverseAddressing(cp.size());

                if (noEdgeEncoding_)
                {
                    forAll(cp, edgei)
                    {
                        // FatalError if (cp[edgei] < 0)
                        reverseAddressing[edgei] =
                            (cp[edgei] - tgtPatchStart);
                    }
                }
                else
                {
                    // With '-1' to account for face direction
                    forAll(cp, edgei)
                    {
                        // FatalError if (cp[edgei] <= 0)
                        reverseAddressing[edgei] =
                            (cp[edgei] - 1 - tgtPatchStart);
                    }
                }

                boundaries[tgtPatchi].rmap(oldPatchField, reverseAddressing);
            }
            else
            {
                // Processor patch

                const auto& oldPatchField = procField.boundaryField()[patchi];

                // In processor patches, there's a mix of internal faces (some
                // of them turned) and possible cyclics. Slow loop
                forAll(cp, patchEdgei)
                {
                    const auto& edgeValue = oldPatchField[patchEdgei];

                    // The target edge, with '-1' to account for edge direction
                    label tgtEdgei =
                    (
                        noEdgeEncoding_
                      ? cp[patchEdgei]
                      : (Foam::mag(cp[patchEdgei] - 1))
                    );

                    if (cp[patchEdgei] < 0)
                    {
                        // The incoming edge is flipped (eg, neigbour side)
                        // - ignore its contribution
                    }
                    else if (tgtEdgei < mesh_.nInternalEdges())
                    {
                        // Target edge is an internal edge - ignore
                    }
                    else
                    {
                        // Target edge is a boundary, find which one.

//                     label tgtPatchi =
//                         mesh_.boundary().whichPatch(tgtEdgei);

                        // Binary search in patch starts (with +1 to
                        // include the start in the comparison)

                        const label tgtPatchi =
                            Foam::findLower(gStarts, (tgtEdgei+1));

                        if (tgtPatchi < 0)
                        {
                            FatalErrorInFunction
                                << "Edge " << tgtEdgei
                                << " not found in any of the patches" << nl
                                << "The patches appear to be inconsistent"
                                   " with the mesh :" << endl
                                << abort(FatalError);
                        }

                        if (!boundaries.test(tgtPatchi))
                        {
                            boundaries.set
                            (
                                tgtPatchi,
                                faPatchField<Type>::New
                                (
                                    mesh_.boundary()[tgtPatchi].type(),
                                    mesh_.boundary()[tgtPatchi],
                                    tgtInternal
                                )
                            );
                        }

                        // add the edge
//                         label tgtPatchEdgei =
//                             mesh_.boundary()[tgtPatchi].whichEdge(tgtEdgei);

                        label tgtPatchEdgei = (tgtEdgei - gStarts[tgtPatchi]);

                        boundaries[tgtPatchi][tgtPatchEdgei] = edgeValue;
                    }
                }
            }
        }
    }

    forAll(mesh_.boundary(), patchi)
    {
        // add empty patches
        if
        (
            isA<emptyFaPatch>(mesh_.boundary()[patchi])
         && !boundaries.test(patchi)
        )
        {
            boundaries.set
            (
                patchi,
                faPatchField<Type>::New
                (
                    faPatchFieldBase::emptyType(),
                    mesh_.boundary()[patchi],
                    tgtInternal
                )
            );
        }
    }

    return tfield;
}


template<class Type>
Foam::tmp<Foam::GeometricField<Type, Foam::faePatchField, Foam::edgeMesh>>
Foam::faFieldReconstructor::reconstructField
(
    const IOobject& fieldObject,
    const UPtrList<GeometricField<Type, faePatchField, edgeMesh>>& procFields
) const
{
    // Construct field with blank slate for internal, and with null boundaries
    auto tfield = tmp<GeometricField<Type, faePatchField, edgeMesh>>::New
    (
        fieldObject,
        mesh_,
        procFields[0].dimensions(),
        Field<Type>(mesh_.nInternalEdges()),
        UPtrList<faePatchField<Type>>()
    );

    tfield.ref().oriented() = procFields[0].oriented();

    auto& tgtInternal = tfield.ref().internalFieldRef();
    auto& boundaries = tfield.ref().boundaryFieldRef();

    boundaries.resize_null(mesh_.boundary().size());  // extra safety

    // The patch starts (global mesh)
    const labelList gStarts(mesh_.boundary().patchStarts());


    // HACK: until we add flip information there is no other way
    // to track when the processor boundary value has flipped

    const auto& edgeOwner = mesh_.edgeOwner();

    bitSet boundaryEdgeFlips;

    forAll(procMeshes_, proci)
    {
        const auto& procField = procFields[proci];
        const auto& procMesh = procMeshes_[proci];

        // The (edge,face,boundary)ProcAddressing for the current procMesh
        const auto& edgeProcAddr = edgeProcAddressing_[proci];
        const auto& faceProcAddr = faceProcAddressing_[proci];
        const auto& boundaryProcAddr = boundaryProcAddressing_[proci];

        // Set the edge values in the reconstructed field

        // It is necessary to create a copy of the addressing array to
        // take care of the face direction offset trick.
        {
            const auto& edgeMap = edgeProcAddressing_[proci];

            if (noEdgeEncoding_)
            {
                // Set the edge values in the reconstructed field
                tgtInternal.rmap(procField.primitiveField(), edgeMap);

                // HACK: track sign flips without any edge flip information!!
                boundaryEdgeFlips.clear();
                const auto nInternalEdges = procMesh.nInternalEdges();
                for
                (
                    label edgei = nInternalEdges;
                    edgei < procMesh.nEdges();
                    ++edgei
                )
                {
                    // The corresponding owner face in the serial mesh:
                    auto serialEdgei = edgeProcAddr[edgei];
                    auto ownFacei = faceProcAddr[procMesh.edgeOwner()[edgei]];

                    if (edgeOwner[serialEdgei] != ownFacei)
                    {
                        boundaryEdgeFlips.set(edgei - nInternalEdges);
                    }
                }
            }
            else
            {
                // Addressing into original field
                labelList curAddr(edgeMap.size());

                std::transform
                (
                    edgeMap.begin(), edgeMap.end(), curAddr.begin(),
                    [](auto val){ return (Foam::mag(val)-1); }
                );

                if (procField.is_oriented())
                {
                    // Correctly oriented copy of internal field
                    Field<Type> orientedInternal(procField.primitiveField());
                    forAll(orientedInternal, i)
                    {
                        if (edgeMap[i] < 0)
                        {
                            orientedInternal[i] = -orientedInternal[i];
                        }
                    }

                    // Set the edge values in the reconstructed field
                    tgtInternal.rmap(orientedInternal, curAddr);
                }
                else
                {
                    // Set the edge values in the reconstructed field
                    tgtInternal.rmap(procField.primitiveField(), curAddr);
                }
            }
        }

        // The patch starts (local mesh)
        const labelList starts(procMesh.boundary().patchStarts());

        // Set the boundary patch values in the reconstructed field

        forAll(boundaryProcAddr, patchi)
        {
            // Get addressing slice for this patch
//             const auto cp =
//                 procMesh.boundary()[patchi].patchSlice(edgeProcAddr);

            const auto cp =
                edgeProcAddr.slice
                (
                    starts[patchi],
                    procMesh.boundary()[patchi].size()
                );

            // Get patch index of the original patch,
            // check if the boundary patch is not a processor patch
            if
            (
                const auto tgtPatchi = boundaryProcAddr[patchi];
                (tgtPatchi >= 0)
            )
            {
                // Regular patch. Fast looping

                const auto& oldPatchField = procField.boundaryField()[patchi];

                if (!boundaries.test(tgtPatchi))
                {
                    boundaries.set
                    (
                        tgtPatchi,
                        faePatchField<Type>::New
                        (
                            oldPatchField,
                            mesh_.boundary()[tgtPatchi],
                            tgtInternal,
                            faPatchFieldReconstructor
                            (
                                mesh_.boundary()[tgtPatchi].size(),
                                procField.boundaryField()[patchi].size()
                            )
                        )
                    );
                }

                const label tgtPatchStart = gStarts[tgtPatchi];
//                     mesh_.boundary()[tgtPatchi].start();

                labelList reverseAddressing(cp.size());

                if (noEdgeEncoding_)
                {
                    forAll(cp, edgei)
                    {
                        reverseAddressing[edgei] =
                            (cp[edgei] - tgtPatchStart);
                    }
                }
                else
                {
                    forAll(cp, edgei)
                    {
                        // With -1 to account for edge direction.
                        reverseAddressing[edgei] =
                            (cp[edgei] - 1 - tgtPatchStart);
                    }
                }

                boundaries[tgtPatchi].rmap(oldPatchField, reverseAddressing);
            }
            else
            {
                // Processor patch

                const auto& oldPatchField = procField.boundaryField()[patchi];

                // In processor patches, there's a mix of internal faces (some
                // of them turned) and possible cyclics. Slow loop

                // For the loop:
                // - track boundaryEdgei on the proc-local mesh as addressing
                //   into boundaryEdgeFlips.
                // - track patchEdgei on the proc-local mesh patch.
                const auto patchOffset =
                    (starts[patchi] - procMesh.nInternalEdges());

                for (label patchEdgei = 0; patchEdgei < cp.size(); ++patchEdgei)
                {
                    const auto& edgeValue = oldPatchField[patchEdgei];

                    // The target edge, with '-1' to account for edge direction
                    label tgtEdgei =
                    (
                        noEdgeEncoding_
                      ? cp[patchEdgei]
                      : (Foam::mag(cp[patchEdgei]) - 1)
                    );

                    if
                    (
                        (cp[patchEdgei] < 0) ||
                        (
                            noEdgeEncoding_
                         && boundaryEdgeFlips.test(patchOffset+patchEdgei)
                        )
                    )
                    {
                        // The incoming edge is flipped (eg, neigbour side)
                        // - ignore its contribution
                    }
                    else if (tgtEdgei < mesh_.nInternalEdges())
                    {
                        // Target edge is an internal edge

                        // Processor patch -> internal face
                        tgtInternal[tgtEdgei] = edgeValue;
                    }
                    else
                    {
                        // Target edge is a boundary, find which one.

//                         label tgtPatchi =
//                             mesh_.boundary().whichPatch(tgtEdgei);

                        // Binary search in patch starts (with +1 to
                        // include the start in the comparison)

                        const label tgtPatchi =
                            Foam::findLower(gStarts, (tgtEdgei+1));

                        if (tgtPatchi < 0)
                        {
                            FatalErrorInFunction
                                << "Edge " << tgtEdgei
                                << " not found in any of the patches" << nl
                                << "The patches appear to be inconsistent"
                                   " with the mesh :" << endl
                                << abort(FatalError);
                        }

                        if (!boundaries.test(tgtPatchi))
                        {
                            boundaries.set
                            (
                                tgtPatchi,
                                faePatchField<Type>::New
                                (
                                    mesh_.boundary()[tgtPatchi].type(),
                                    mesh_.boundary()[tgtPatchi],
                                    tgtInternal
                                )
                            );
                        }

                        // add the value
//                         label tgtPatchEdgei =
//                             mesh_.boundary()[tgtPatchi].whichEdge(tgtEdgei);

                        label tgtPatchEdgei(tgtEdgei - gStarts[tgtPatchi]);

                        boundaries[tgtPatchi][tgtPatchEdgei] = edgeValue;
                    }
                }
            }
        }
    }

    forAll(mesh_.boundary(), patchi)
    {
        // add empty patches
        if
        (
            isA<emptyFaPatch>(mesh_.boundary()[patchi])
         && !boundaries.test(patchi)
        )
        {
            boundaries.set
            (
                patchi,
                faePatchField<Type>::New
                (
                    faePatchFieldBase::emptyType(),
                    mesh_.boundary()[patchi],
                    tgtInternal
                )
            );
        }
    }

    return tfield;
}


template<class Type>
Foam::tmp<Foam::GeometricField<Type, Foam::faPatchField, Foam::areaMesh>>
Foam::faFieldReconstructor::reconstructAreaField
(
    const IOobject& fieldObject
)
{
    // Read the field for all the processors
    PtrList<GeometricField<Type, faPatchField, areaMesh>> procFields
    (
        procMeshes_.size()
    );

    forAll(procMeshes_, proci)
    {
        procFields.emplace_set
        (
            proci,
            IOobject
            (
                fieldObject.name(),
                procMeshes_[proci].thisDb().time().timeName(),
                procMeshes_[proci].thisDb(),
                IOobjectOption::MUST_READ,
                IOobjectOption::NO_WRITE,
                IOobjectOption::NO_REGISTER
            ),
            procMeshes_[proci]
        );
    }

    return reconstructField
    (
        IOobject
        (
            fieldObject.name(),
            mesh_.thisDb().time().timeName(),
            mesh_.thisDb(),
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        procFields
    );
}


template<class Type>
Foam::tmp<Foam::GeometricField<Type, Foam::faePatchField, Foam::edgeMesh>>
Foam::faFieldReconstructor::reconstructEdgeField
(
    const IOobject& fieldObject
)
{
    // Read the field for all the processors
    PtrList<GeometricField<Type, faePatchField, edgeMesh>> procFields
    (
        procMeshes_.size()
    );

    forAll(procMeshes_, proci)
    {
        procFields.emplace_set
        (
            proci,
            IOobject
            (
                fieldObject.name(),
                procMeshes_[proci].thisDb().time().timeName(),
                procMeshes_[proci].thisDb(),
                IOobjectOption::MUST_READ,
                IOobjectOption::NO_WRITE,
                IOobjectOption::NO_REGISTER
            ),
            procMeshes_[proci]
        );
    }

    return reconstructField
    (
        IOobject
        (
            fieldObject.name(),
            mesh_.thisDb().time().timeName(),
            mesh_.thisDb(),
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        procFields
    );
}


template<class Type>
Foam::label Foam::faFieldReconstructor::reconstructAreaFields
(
    const UPtrList<const IOobject>& fieldObjects
)
{
    typedef GeometricField<Type, faPatchField, areaMesh> fieldType;

    label nFields = 0;

    for (const IOobject& io : fieldObjects)
    {
        if (io.isHeaderClass<fieldType>())
        {
            if (verbose_)
            {
                if (!nFields)
                {
                    Info<< "    Reconstructing "
                        << fieldType::typeName << "s\n" << nl;
                }
                Info<< "        " << io.name() << endl;
            }
            ++nFields;

            reconstructAreaField<Type>(io)().write();
            ++nReconstructed_;
        }
    }

    if (verbose_ && nFields) Info<< endl;
    return nFields;
}


template<class Type>
Foam::label Foam::faFieldReconstructor::reconstructEdgeFields
(
    const UPtrList<const IOobject>& fieldObjects
)
{
    typedef GeometricField<Type, faePatchField, edgeMesh> fieldType;

    label nFields = 0;

    for (const IOobject& io : fieldObjects)
    {
        if (io.isHeaderClass<fieldType>())
        {
            if (verbose_)
            {
                if (!nFields)
                {
                    Info<< "    Reconstructing "
                        << fieldType::typeName << "s\n" << nl;
                }
                Info<< "        " << io.name() << endl;
            }
            ++nFields;

            reconstructEdgeField<Type>(io)().write();
            ++nReconstructed_;
        }
    }

    if (verbose_ && nFields) Info<< endl;
    return nFields;
}


template<class Type>
Foam::label Foam::faFieldReconstructor::reconstructAreaFields
(
    const IOobjectList& objects,
    const wordRes& selectedFields
)
{
    typedef GeometricField<Type, faPatchField, areaMesh> fieldType;

    return reconstructAreaFields<Type>
    (
        (
            selectedFields.empty()
          ? objects.csorted<fieldType>()
          : objects.csorted<fieldType>(selectedFields)
        )
    );
}


template<class Type>
Foam::label Foam::faFieldReconstructor::reconstructEdgeFields
(
    const IOobjectList& objects,
    const wordRes& selectedFields
)
{
    typedef GeometricField<Type, faePatchField, edgeMesh> fieldType;

    return reconstructEdgeFields<Type>
    (
        (
            selectedFields.empty()
          ? objects.csorted<fieldType>()
          : objects.csorted<fieldType>(selectedFields)
        )
    );
}


// ************************************************************************* //
