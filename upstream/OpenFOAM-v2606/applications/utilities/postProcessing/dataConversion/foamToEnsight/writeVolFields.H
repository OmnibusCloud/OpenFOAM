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
    Read volume fields from disk and write with ensightMesh

\*---------------------------------------------------------------------------*/

#ifndef FoamToEnsight_writeVolFields_H
#define FoamToEnsight_writeVolFields_H

#include "readFields.H"
#include "fvMesh.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

template<class Type>
bool writeVolField
(
    ensightCase& ensCase,
    const ensightMesh& ensMesh,
    const tmp<VolumeField<Type>>& tfield,
    const bool nearCellValue = false
)
{
    if (!tfield)
    {
        return false;
    }
    else if (nearCellValue)
    {
        auto tzgrad = makeZeroGradientField<Type>(tfield);

        // Recursive call
        return writeVolField
        (
            ensCase,
            ensMesh,
            tzgrad,
            false  // No nearCellValue, we already have zero-gradient
        );
    }

    const auto& field = tfield();

    // Forced use of node values?
    const bool nodeValues = ensCase.nodeValues();

    autoPtr<ensightFile> os =
        ensCase.newData<Type>(field.name(), nodeValues);

    bool wrote = ensightOutput::writeVolField<Type>
    (
        os.ref(),
        field,
        ensMesh,
        nodeValues
    );

    tfield.clear();
    return wrote;
}


template<class Type>
label writeVolFields
(
    ensightCase& ensCase,
    const ensightMesh& ensMesh,
    const IOobjectList& objects,
    const bool nearCellValue = false
)
{
    typedef VolumeField<Type> FieldType;

    const auto& mesh = refCast<const fvMesh>(ensMesh.mesh());

    label count = 0;

    for (const IOobject& io : objects.csorted<FieldType>())
    {
        if
        (
            writeVolField<Type>
            (
                ensCase,
                ensMesh,
                getField<FieldType>(io, mesh),
                nearCellValue
            )
        )
        {
            Info<< ' ' << io.name();
            ++count;
        }
    }

    return count;
}


label writeAllVolFields
(
    ensightCase& ensCase,
    const ensightMesh& ensMesh,
    const IOobjectList& objects,
    const bool nearCellValue = false
)
{
    label count = 0;
    const label total = objects.size();
    do
    {
        #undef  doLocalWriteCode
        #define doLocalWriteCode(Type)                      \
        {                                                   \
            count += writeVolFields<Type>                   \
            (                                               \
                ensCase,                                    \
                ensMesh,                                    \
                objects,                                    \
                nearCellValue                               \
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
