/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2018-2025 OpenCFD Ltd.
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM, distributed under GPL-3.0-or-later.

Description
    Code chunk for reading point fields from disk
    and write with vtk::internalWriter and vtk::patchWriter

\*---------------------------------------------------------------------------*/

#ifndef FoamToVTK_writePointFields_H
#define FoamToVTK_writePointFields_H

#include "readFields.H"
#include "foamVtkInternalWriter.H"
#include "foamVtkPatchWriter.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

template<class GeoField>
bool writePointField
(
    autoPtr<vtk::internalWriter>& internalWriter,
    UPtrList<vtk::patchWriter>& patchWriters,

    const tmp<GeoField>& tfield,
    const fvMeshSubsetProxy& proxy
)
{
    // Sanity test
    if (!internalWriter && patchWriters.empty()) return false;

    if (!tfield)
    {
        return false;
    }

    tmp<GeoField> tproxied;
    if (proxy.useSubMesh())
    {
        tproxied = proxy.interpolate(tfield());
        tfield.clear();
    }
    else
    {
        tproxied = tfield;
    }

    if (!tproxied)
    {
        // Or Error?
        return false;
    }


    const auto& field = tproxied();

    // Internal
    if (internalWriter)
    {
        internalWriter->write(field);
    }

    // Boundary
    for (auto& writer : patchWriters)
    {
        writer.write(field);
    }


    tproxied.clear();

    return true;
}


template<class GeoField>
label writePointFields
(
    autoPtr<vtk::internalWriter>& internalWriter,
    UPtrList<vtk::patchWriter>& patchWriters,

    const fvMeshSubsetProxy& proxy,
    const typename GeoField::Mesh& ptMesh,
    const IOobjectList& objects,
    const bool syncPar
)
{
    // Sanity test
    if (!internalWriter && patchWriters.empty()) return 0;

    label count = 0;

    for (const word& fieldName : objects.sortedNames<GeoField>())
    {
        if
        (
            writePointField<GeoField>
            (
                internalWriter,
                patchWriters,
                getField<GeoField>(ptMesh, objects, fieldName, syncPar),
                proxy
            )
        )
        {
            ++count;
        }
    }

    return count;
}


label writeAllPointFields
(
    autoPtr<vtk::internalWriter>& internalWriter,
    UPtrList<vtk::patchWriter>& patchWriters,

    const fvMeshSubsetProxy& proxy,
    const IOobjectList& objects,
    const bool syncPar
)
{
    // Sanity test
    if (!internalWriter && patchWriters.empty()) return 0;

    const pointMesh& ptMesh = pointMesh::New(proxy.baseMesh());

    label count = 0;
    const label total = objects.size();
    do
    {
        #undef  doLocalWriteCode
        #define doLocalWriteCode(Type)                      \
        {                                                   \
            typedef PointField<Type> FieldType;             \
                                                            \
            count += writePointFields<FieldType>            \
            (                                               \
                internalWriter,                             \
                patchWriters,                               \
                proxy, ptMesh,                              \
                objects,                                    \
                syncPar                                     \
            );                                              \
            if (count >= total) break;  /* early exit */    \
        }

        doLocalWriteCode(scalar);
        doLocalWriteCode(vector);
        doLocalWriteCode(sphericalTensor);
        doLocalWriteCode(symmTensor);
        doLocalWriteCode(tensor);

        #undef doLocalWriteCode
    }
    while (false);

    return count;
}


} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
