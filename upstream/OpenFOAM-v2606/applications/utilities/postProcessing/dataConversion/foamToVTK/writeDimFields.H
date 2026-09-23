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
    Code chunk for reading dimensioned fields from disk
    and write with vtk::internalWriter

\*---------------------------------------------------------------------------*/

#ifndef FoamToVTK_writeDimFields_H
#define FoamToVTK_writeDimFields_H

#include "readFields.H"
#include "foamVtkInternalWriter.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

template<class GeoField>
label writeDimFields
(
    autoPtr<vtk::internalWriter>& internalWriter,
    const fvMeshSubsetProxy& proxy,
    const IOobjectList& objects,
    const bool syncPar,
    objectRegistry* cache
)
{
    // Sanity test
    if (!internalWriter) return 0;

    label count = 0;

    for (const word& fieldName : objects.sortedNames<GeoField>())
    {
        tmp<GeoField> tfield =
            getField<GeoField>(proxy, objects, fieldName, syncPar, cache);

        if (tfield)
        {
            internalWriter->write(tfield());

            tfield.clear();
            ++count;
        }
    }

    return count;
}


template<class GeoField>
label writeDimFields
(
    autoPtr<vtk::internalWriter>& internalWriter,
    const autoPtr<volPointInterpolation>& pInterp,

    const fvMeshSubsetProxy& proxy,
    const IOobjectList& objects,
    const bool syncPar,
    objectRegistry* cache
)
{
    // Sanity test
    if (!internalWriter || !pInterp) return 0;

    label count = 0;

    for (const word& fieldName : objects.sortedNames<GeoField>())
    {
        tmp<GeoField> tfield =
            getField<GeoField>(proxy, objects, fieldName, syncPar, cache);

        if (tfield)
        {
            internalWriter->write(tfield(), *pInterp);

            tfield.clear();
            ++count;
        }
    }

    return count;
}


label writeAllDimFields
(
    autoPtr<vtk::internalWriter>& internalWriter,

    const fvMeshSubsetProxy& proxy,
    const IOobjectList& objects,
    const bool syncPar,
    objectRegistry* cache
)
{
    // Sanity test
    if (!internalWriter) return 0;

    label count = 0;
    const label total = objects.size();
    do
    {
        #undef  doLocalWriteCode
        #define doLocalWriteCode(Type)                      \
        {                                                   \
            typedef VolumeInternalField<Type> FieldType;    \
                                                            \
            count += writeDimFields<FieldType>              \
            (                                               \
                internalWriter,                             \
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

        #undef doLocalWriteField
    }
    while (false);

    return count;
}


label writeAllDimFields
(
    autoPtr<vtk::internalWriter>& internalWriter,
    const autoPtr<volPointInterpolation>& pInterp,

    const fvMeshSubsetProxy& proxy,
    const IOobjectList& objects,
    const bool syncPar,
    objectRegistry* cache
)
{
    // Sanity test
    if (!internalWriter || !pInterp) return 0;

    label count = 0;
    const label total = objects.size();
    do
    {
        #undef  doLocalWriteCode
        #define doLocalWriteCode(Type)                      \
        {                                                   \
            typedef VolumeInternalField<Type> FieldType;    \
                                                            \
            count += writeDimFields<FieldType>              \
            (                                               \
                internalWriter, pInterp,                    \
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
