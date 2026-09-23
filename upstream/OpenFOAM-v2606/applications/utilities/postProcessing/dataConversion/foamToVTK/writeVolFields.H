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
    Code chunk for reading volume fields from disk
    and write with vtk::internalWriter and vtk::patchWriter

\*---------------------------------------------------------------------------*/

#ifndef FoamToVTK_writeVolFields_H
#define FoamToVTK_writeVolFields_H

#include "readFields.H"
#include "foamVtkInternalWriter.H"
#include "foamVtkPatchWriter.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

template<class GeoField>
label writeVolFields
(
    autoPtr<vtk::internalWriter>& internalWriter,
    UPtrList<vtk::patchWriter>& patchWriters,
    const fvMeshSubsetProxy& proxy,
    const IOobjectList& objects,
    const bool syncPar,
    objectRegistry* cache
)
{
    // Sanity test
    if (!internalWriter && patchWriters.empty()) return 0;

    label count = 0;

    for (const word& fieldName : objects.sortedNames<GeoField>())
    {
        tmp<GeoField> tfield =
            getField<GeoField>(proxy, objects, fieldName, syncPar, cache);

        if (tfield)
        {
            // Internal
            if (internalWriter)
            {
                internalWriter->write(tfield());
            }

            // Boundary
            for (auto& writer : patchWriters)
            {
                writer.write(tfield());
            }

            tfield.clear();
            ++count;
        }
    }

    return count;
}


template<class GeoField>
label writeVolFields
(
    autoPtr<vtk::internalWriter>& internalWriter,
    const autoPtr<volPointInterpolation>& pInterp,

    UPtrList<vtk::patchWriter>& patchWriters,
    const UPtrList<PrimitivePatchInterpolation<primitivePatch>>& patchInterps,

    const fvMeshSubsetProxy& proxy,
    const IOobjectList& objects,
    const bool syncPar,
    objectRegistry* cache
)
{
    // Sanity test
    if
    (
        (!internalWriter || !pInterp)
     && (patchWriters.empty() || patchInterps.empty())
    )
    {
        return 0;
    }

    label count = 0;

    for (const word& fieldName : objects.sortedNames<GeoField>())
    {
        tmp<GeoField> tfield =
            getField<GeoField>(proxy, objects, fieldName, syncPar, cache);

        if (tfield)
        {
            // Internal
            if (internalWriter && pInterp)
            {
                internalWriter->write(tfield(), *pInterp);
            }

            // Boundary
            label writeri = 0;
            for (auto& writer : patchWriters)
            {
                if (patchInterps.test(writeri))
                {
                    writer.write(tfield(), patchInterps[writeri]);
                }
                ++writeri;
            }

            tfield.clear();
            ++count;
        }
    }

    return count;
}


label writeAllVolFields
(
    autoPtr<vtk::internalWriter>& internalWriter,
    UPtrList<vtk::patchWriter>& patchWriters,

    const fvMeshSubsetProxy& proxy,
    const IOobjectList& objects,
    const bool syncPar,
    objectRegistry* cache
)
{
    // Sanity test
    if (!internalWriter && patchWriters.empty()) return 0;

    label count = 0;
    const label total = objects.size();
    do
    {
        #undef  doLocalWriteCode
        #define doLocalWriteCode(Type)                      \
        {                                                   \
            typedef VolumeField<Type> FieldType;            \
                                                            \
            count += writeVolFields<FieldType>              \
            (                                               \
                internalWriter,                             \
                patchWriters,                               \
                proxy,                                      \
                objects,                                    \
                syncPar,                                    \
                cache                                       \
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


label writeAllVolFields
(
    autoPtr<vtk::internalWriter>& internalWriter,
    const autoPtr<volPointInterpolation>& pInterp,

    UPtrList<vtk::patchWriter>& patchWriters,
    const UPtrList<PrimitivePatchInterpolation<primitivePatch>>& patchInterps,

    const fvMeshSubsetProxy& proxy,
    const IOobjectList& objects,
    const bool syncPar,
    objectRegistry* cache
)
{
    label count = 0;
    const label total = objects.size();
    do
    {
        #undef  doLocalWriteCode
        #define doLocalWriteCode(Type)                      \
        {                                                   \
            typedef VolumeField<Type> FieldType;            \
                                                            \
            count += writeVolFields<FieldType>              \
            (                                               \
                internalWriter, pInterp,                    \
                patchWriters,   patchInterps,               \
                proxy,                                      \
                objects,                                    \
                syncPar,                                    \
                cache                                       \
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
