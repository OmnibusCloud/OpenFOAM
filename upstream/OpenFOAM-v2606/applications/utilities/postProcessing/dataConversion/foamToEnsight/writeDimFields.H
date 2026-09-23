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

InNamespace
    Foam

Description
    Read dimensioned fields from disk and write with ensightMesh

\*---------------------------------------------------------------------------*/

#ifndef FoamToEnsight_writeDimFields_H
#define FoamToEnsight_writeDimFields_H

#include "writeVolFields.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

template<class Type>
bool writeDimField
(
    ensightCase& ensCase,
    const ensightMesh& ensMesh,
    const tmp<VolumeInternalField<Type>>& tdf
)
{
    if (!tdf)
    {
        return false;
    }

    auto tfield = makeZeroGradientField<Type>(tdf);

    // Now a volField with zero-gradient boundaries

    return writeVolField<Type>
    (
        ensCase,
        ensMesh,
        tfield,
        false  // No nearCellValue, we already have zero-gradient
    );
}


template<class Type>
label writeDimFields
(
    ensightCase& ensCase,
    const ensightMesh& ensMesh,
    const IOobjectList& objects
)
{
    typedef VolumeInternalField<Type> FieldType;

    const auto& mesh = refCast<const fvMesh>(ensMesh.mesh());

    label count = 0;

    for (const IOobject& io : objects.csorted<FieldType>())
    {
        if
        (
            writeDimField<Type>
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


label writeAllDimFields
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
            count += writeDimFields<Type>                   \
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
