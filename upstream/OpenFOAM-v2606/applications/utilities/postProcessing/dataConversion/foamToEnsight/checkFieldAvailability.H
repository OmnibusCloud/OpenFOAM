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

Description
    Check field availability for last time.
    Done to avoid mapping 'undefined' when a field only exists as time 0.

Requires
    readFields.H (for the checkData function)

\*---------------------------------------------------------------------------*/

// Initially all possible objects that are available at the final time
List<wordHashSet> availableRegionObjectNames(meshes.size());
List<HashTable<wordHashSet>> availableFaRegionObjectNames(meshes.size());

forAll(meshes, regioni)
{
    const auto& mesh = meshes[regioni];

    IOobjectList objects;
    HashTable<IOobjectList> faObjects;

    if (doConvertFields && !timeDirs.empty())
    {
        const word& checkTimeName = timeDirs.back().name();

        // List of volume mesh objects for this instance
        objects = IOobjectList(mesh, checkTimeName);

        objects.prune_0();  // Remove restart fields

        if (fieldSelector)
        {
            objects.filterObjects(fieldSelector);
        }
        if (!doPointValues)
        {
            // Prune point fields if disabled
            objects.filterClasses(Foam::fieldTypes::is_point, true);
        }


        // The finite-area fields for this volume region and instance
        faObjects.reserve(meshesFa[regioni].size());
        for (const auto& areaMesh : meshesFa[regioni])
        {
            const word& areaName = areaMesh.name();

            IOobjectList objs
            (
                areaMesh,
                checkTimeName,
                IOobjectOption::NO_REGISTER
            );

            objs.prune_0();  // Remove restart fields

            if (fieldSelector)
            {
                objs.filterObjects(fieldSelector);
            }

            faObjects(areaName) = std::move(objs);
        }
    }

    // Volume fields
    if (!objects.empty())
    {
        wordList objectNames(objects.sortedNames());

        // Check availability for all times...
        checkData
        (
            mesh.thisDb(),
            timeDirs,
            objectNames
        );

        availableRegionObjectNames[regioni] = objectNames;
    }

    // Area fields
    for (const auto& areaMesh : meshesFa[regioni])
    {
        const word& areaName = areaMesh.name();

        wordList objectNames;

        if (const auto iter = faObjects.cfind(areaName); iter.good())
        {
            objectNames = iter.val().sortedNames();
        }

        // Check availability for all times...
        checkData
        (
            areaMesh,
            timeDirs,
            objectNames
        );

        availableFaRegionObjectNames[regioni].emplace_set
        (
            areaName,
            std::move(objectNames)
        );
    }
}


// ************************************************************************* //
