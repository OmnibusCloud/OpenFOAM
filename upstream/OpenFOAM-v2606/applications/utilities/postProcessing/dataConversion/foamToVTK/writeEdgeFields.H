/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2026 OpenCFD Ltd.
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM, distributed under GPL-3.0-or-later.

Description
    Code chunk for reading finite-area edge fields from disk
    and write with vtk::lineWriter

\*---------------------------------------------------------------------------*/

#ifndef FoamToVTK_writeEdgeFields_H
#define FoamToVTK_writeEdgeFields_H

#include "readFields.H"
#include "foamVtkLineWriter.H"
#include "faMeshTools.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// Writer type for finite-area mesh + fields
typedef
    vtk::lineWriter
    vtkWriterType_edgeMesh;


template<class GeoField>
label writeEdgeFields
(
    vtkWriterType_edgeMesh& writer,
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
            auto tflat = faMeshTools::flattenEdgeField(tfield());

            writer.write(fieldName, tflat());

            tfield.clear();
            ++count;
        }
    }

    return count;
}


label writeAllEdgeFields
(
    vtkWriterType_edgeMesh& writer,
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
        #define doLocalWriteCode(Type)                                        \
        {                                                                     \
            typedef GeometricField<Type, faePatchField, edgeMesh> FieldType;  \
                                                                              \
            count += writeEdgeFields<FieldType>                               \
            (                                                                 \
                writer,                                                       \
                mesh,                                                         \
                objects,                                                      \
                syncPar                                                       \
            );                                                                \
            if (count >= total) break;  /* early exit */                      \
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
