/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2020-2025 OpenCFD Ltd.
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM, distributed under GPL-3.0-or-later.

InNamespace
    Foam

Description
    Read point fields from disk and write as ensight data

\*---------------------------------------------------------------------------*/

#ifndef FoamToEnsight_writePointFields_H
#define FoamToEnsight_writePointFields_H

#include "readFields.H"
#include "ensightMesh.H"
#include "fvMesh.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

template<class Type>
bool writePointField
(
    ensightCase& ensCase,
    const ensightMesh& ensMesh,
    const tmp<PointField<Type>>& tfield
)
{
    if (!tfield)
    {
        return false;
    }

    const auto& field = tfield();

    // PointData = true
    autoPtr<ensightFile> os = ensCase.newData<Type>(field.name(), true);

    bool wrote = ensightOutput::writePointField<Type>
    (
        os.ref(),
        field,
        ensMesh
    );

    tfield.clear();
    return wrote;
}


template<class Type>
label writePointFields
(
    ensightCase& ensCase,
    const ensightMesh& ensMesh,
    const IOobjectList& objects
)
{
    typedef PointField<Type> FieldType;

    const pointMesh& pMesh = pointMesh::New(ensMesh.mesh());

    label count = 0;

    for (const IOobject& io : objects.csorted<FieldType>())
    {
        if
        (
            writePointField<Type>
            (
                ensCase,
                ensMesh,
                getField<FieldType>(io, pMesh)
            )
        )
        {
            Info<< ' ' << io.name();
            ++count;
        }
    }

    return count;
}


label writeAllPointFields
(
    ensightCase& ensCase,
    const ensightMesh& ensMesh,
    const IOobjectList& objects
)
{
    label count = 0;
    const label total = objects.size();
    do
    {
        #undef  doLocalWriteCode
        #define doLocalWriteCode(Type)                      \
        {                                                   \
            count += writePointFields<Type>                 \
            (                                               \
                ensCase,                                    \
                ensMesh,                                    \
                objects                                     \
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
