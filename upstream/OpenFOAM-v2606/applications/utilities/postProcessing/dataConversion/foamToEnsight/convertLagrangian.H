/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2018-2022 OpenCFD Ltd.
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM, distributed under GPL-3.0-or-later.

Description
    Code chunk for post-processing conversion of cloud(s) to Ensight

\*---------------------------------------------------------------------------*/

// Cloud field data output
if (doLagrangian)
{
    // Lagrangian
    const auto& cloudFields = regionCloudFields[regioni];
    const auto& cloudNames = regionCloudNames[regioni];

    for (const word& cloudName : cloudNames)
    {
        const HashTable<word>& theseCloudFields = cloudFields[cloudName];

        fileNameList currentCloudDirs
        (
            readDir
            (
                runTime.timePath()/regionDir/cloud::prefix,
                fileName::DIRECTORY
            )
        );

        Info<< "Cloud " << cloudName << " (";

        const bool cloudExists =
            returnReduceOr(currentCloudDirs.found(cloudName));

        {
            autoPtr<ensightFile> os = ensCase.newCloud(cloudName);

            ensightOutput::writeCloudPositions
            (
                os.ref(),
                mesh,
                cloudName,
                cloudExists
            );

            Info<< " positions";
            if (!cloudExists)
            {
                Info<< "{0}"; // report empty field
            }
        }

        // Field order may differ on individual processors, so sort by name
        for (const word& fieldName : theseCloudFields.sortedToc())
        {
            const word& fieldType = theseCloudFields[fieldName];

            IOobject fieldObject
            (
                fieldName,
                mesh.time().timeName(),
                cloud::prefix/cloudName,
                mesh,
                IOobject::MUST_READ
            );

            bool fieldExists = cloudExists; // No field without positions
            if (cloudExists)
            {
                // Want MUST_READ (globally) and valid=false (locally),
                // but that combination does not work.
                // So check the header and sync globally

                const bool oldParRun = UPstream::parRun(false);
                fieldExists = fieldObject.typeHeaderOk<IOField<scalar>>(false);
                UPstream::parRun(oldParRun);   // Restore parallel state
                UPstream::reduceOr(fieldExists, UPstream::worldComm);
            }

            bool wrote = false;
            do
            {
                #undef  doLocalWriteCode
                #define doLocalWriteCode(Type)                                \
                                                                              \
                if (fieldType == IOField<Type>::typeName)                     \
                {                                                             \
                    autoPtr<ensightFile> os =                                 \
                        ensCase.newCloudData<Type>                            \
                        (                                                     \
                            cloudName,                                        \
                            fieldName                                         \
                        );                                                    \
                                                                              \
                    wrote = ensightOutput::readWriteCloudField<Type>          \
                    (                                                         \
                        os.ref(),                                             \
                        fieldObject,                                          \
                        fieldExists                                           \
                    );                                                        \
                    break;                                                    \
                }

                doLocalWriteCode(scalar);
                doLocalWriteCode(vector);

                #undef doLocalWriteCode
            }
            while (false);

            if (wrote)
            {
                Info<< ' ' << fieldName;
                if (!fieldExists)
                {
                    Info<< "{0}"; // report empty field
                }
            }
        }
        Info<< " )" << nl;
    }
}


// ************************************************************************* //
