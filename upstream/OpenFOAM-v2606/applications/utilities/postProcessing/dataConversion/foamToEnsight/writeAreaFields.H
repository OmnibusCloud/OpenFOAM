/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2021-2025 OpenCFD Ltd.
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM, distributed under GPL-3.0-or-later.

InNamespace
    Foam

Description
    Read finite-area fields from disk and write ensightFaMesh

\*---------------------------------------------------------------------------*/

#ifndef FoamToEnsight_writeAreaFields_H
#define FoamToEnsight_writeAreaFields_H

#include "readFields.H"
#include "areaFaMesh.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

template<class Type>
bool writeAreaField
(
    ensightCase& ensCase,
    const ensightFaMesh& ensMesh,
    const tmp<AreaField<Type>>& tfield
)
{
    if (!tfield)
    {
        return false;
    }
    const auto& field = tfield();

    autoPtr<ensightFile> os =
        ensCase.newData<Type>(field.name());

    bool wrote = ensightOutput::writeAreaField<Type>
    (
        os.ref(),
        field,
        ensMesh
    );

    tfield.clear();
    return wrote;
}


template<class Type>
label writeAreaFields
(
    ensightCase& ensCase,
    const ensightFaMesh& ensMesh,
    const IOobjectList& objects
)
{
    typedef AreaField<Type> FieldType;

    const faMesh& mesh = ensMesh.mesh();

    label count = 0;

    for (const IOobject& io : objects.csorted<FieldType>())
    {
        if
        (
            writeAreaField<Type>
            (
                ensCase,
                ensMesh,
                getField<FieldType>(io, mesh)
            )
        )
        {
            Info<< ' ' << io.name();
            ++count;
        }
    }

    return count;
}


label writeAllAreaFields
(
    ensightCase& ensCase,
    const ensightFaMesh& ensMesh,
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
            count += writeAreaFields<Type>                  \
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
