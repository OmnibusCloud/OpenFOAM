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
    Code chunk for reading finite-area fields from disk
    and write with vtk::uindirectPatchGeoFieldsWriter

\*---------------------------------------------------------------------------*/

#ifndef FoamToVTK_writeAreaFields_H
#define FoamToVTK_writeAreaFields_H

#include "readFields.H"
#include "foamVtkIndPatchGeoFieldsWriter.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// Writer type for finite-area mesh + fields
typedef
    vtk::uindirectPatchGeoFieldsWriter
    vtkWriterType_areaMesh;


template<class GeoField>
label writeAreaFields
(
    vtkWriterType_areaMesh& writer,
    const typename GeoField::Mesh& mesh,
    const IOobjectList& objects,
    const bool syncPar
)
{
    label count = 0;

    for (const word& fieldName : objects.sortedNames<GeoField>())
    {
        tmp<GeoField> tfield =
            getField<GeoField>(mesh, objects, fieldName, syncPar);

        if (tfield)
        {
            writer.write(tfield());

            tfield.clear();
            ++count;
        }
    }

    return count;
}


label writeAllAreaFields
(
    vtkWriterType_areaMesh& writer,
    const faMesh& mesh,
    const IOobjectList& objects,
    const bool syncPar
)
{
    label count = 0;
    const label total = objects.size();
    do
    {
        #undef  doLocalWriteCode
        #define doLocalWriteCode(Type)                      \
        {                                                   \
            typedef AreaField<Type> FieldType;              \
                                                            \
            count += writeAreaFields<FieldType>             \
            (                                               \
                writer,                                     \
                mesh,                                       \
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
