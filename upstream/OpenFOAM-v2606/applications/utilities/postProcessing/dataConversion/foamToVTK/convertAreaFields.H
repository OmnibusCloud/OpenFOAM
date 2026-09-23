/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2018-2026 OpenCFD Ltd.
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM, distributed under GPL-3.0-or-later.

Description
    Code chunk for converting finite-area - included by foamToVTK.

    Typedef vtkWriterType_areaMesh is declared in writeAreaFields.H

Note
    Whereas foamToEnsight creates all mesh accounting up front,
    and uses a List of PtrList for managing the area regions,
    foamToVTK handles them every timestep by checking for file contents
    in the respective area regions.
    This incurs a penalty with creating the finite-area meshes for each
    timestep, but increases its flexibility.

\*---------------------------------------------------------------------------*/

//
// Finite-area mesh and fields - need not exist
//

// No subsetting!
if (doFiniteArea && !areaRegionNames.empty())
{
    using reportFields = foamToVtkReportFields;

    // The (region) polyMesh being used. No subsetting possible
    const auto& basePolyMesh = meshProxy.baseMesh();

    for (const word& areaName : areaRegionNames)
    {
        const bool isDefaultRegion(polyMesh::regionName(areaName).empty());

        // CAUTION
        // If we want to have constant access to the HashTable:
        //
        // (SEGFAULT)
        // const auto& faObjs = faObjects.lookup(areaName, IOobjectList());
        //
        // Use an empty fallback to avoid binding to a temporary:
        //
        // const IOobjectList emptyObjectList;
        // const auto& faObjs = faObjects.lookup(areaName, emptyObjectList);

        // Since we do not need the area fields afterwards,
        // just move them out from the HashTable

        IOobjectList faObjs;
        if (auto iter = faObjects.find(areaName); iter.good())
        {
            faObjs = std::move(iter.val());
        }

        const label nAreaFields = faObjs.count(Foam::fieldTypes::is_area);
        const label nEdgeFields =
        (
            doEdgeFields ? faObjs.count(Foam::fieldTypes::is_edge) : 0
        );

        autoPtr<faMesh> faMeshPtr;

        if (nAreaFields || nEdgeFields || withMeshIds)
        {
            faMeshPtr = faMesh::TryNew(areaName, basePolyMesh);
        }

        if (!faMeshPtr)
        {
            if (!isDefaultRegion)
            {
                // Report any area region specified but missing
                // - silently ignore region0
                Info<< "No area-mesh [" << polyMesh::regionName(areaName)
                    << "] on volume-region ["
                    << basePolyMesh.regionName() << "]" << endl;
            }
            continue;
        }
        const auto& areaMesh = faMeshPtr();

        Info<< "Using area-mesh [" << polyMesh::regionName(areaName)
            << "] on volume-region ["
            << basePolyMesh.regionName() << "]" << endl;


        reportFields::area(Info, faObjs);
        if (nEdgeFields)
        {
            reportFields::edge(Info, faObjs);
        }

        const auto& pp = faMeshPtr->patch();

        vtkWriterType_areaMesh writer
        (
            pp,
            writeOpts,
            (
                outputDir/regionDir/"finite-area"
              / (
                   isDefaultRegion
                 ? fileName("finiteArea")
                 : fileName(areaName/areaName)
                )
              + timeDesc
            ),
            UPstream::parRun()
        );
        Info<< "    Area      : "
            << args.relativePath(writer.output()) << nl;

        writer.beginFile(areaMesh.name());

        writer.writeTimeValue(timeValue);
        writer.writeGeometry();

        // Optionally with (cellID, patchID, faceLabels, procID) fields
        writer.beginCellData
        (
            (withMeshIds ? 3 + (writer.parallel() ? 1 : 0) : 0)
          + nAreaFields
        );

        if (withMeshIds)
        {
            // Cell ids in global numbering
            writer.writeGlobalIDs("cellID");

            // The patch ids can also be quite useful
            const polyBoundaryMesh& pbm = areaMesh.mesh().boundaryMesh();

            labelList patchIds
            (
                pbm.patchID(areaMesh.faceLabels())
            );

            writer.writeCellData("patchID", patchIds);

            // Use proc-local data for faceLabels
            // (confusing enough already without renumbering)
            writer.writeCellData("faceLabels", areaMesh.faceLabels());

            writer.writeProcIDs();  // parallel only
        }

        writeAllAreaFields
        (
            writer,
            areaMesh,
            faObjs,
            true  // syncPar
        );

        if (nEdgeFields)
        {
            vtkWriterType_edgeMesh eWriter
            (
                areaMesh.points(),
                areaMesh.edges(),
                writeOpts,
                (
                    outputDir/regionDir/"edge-fields"
                  / (
                       isDefaultRegion
                     ? fileName("finiteArea")
                     : fileName(areaName/areaName)
                    )
                  + timeDesc
                ),
                UPstream::parRun()
            );
            Info<< "    Edge      : "
                << args.relativePath(eWriter.output()) << nl;

            eWriter.beginFile(areaMesh.name());

            eWriter.writeTimeValue(timeValue);
            eWriter.writeGeometry();

            // Optionally with (edgeID, procID) fields
            eWriter.beginCellData
            (
                (withMeshIds ? 1 + (eWriter.parallel() ? 1 : 0) : 0)
              + nEdgeFields
            );

            if (withMeshIds)
            {
                eWriter.writeProcIDs();  // parallel only

                // Edge ids in rank-local numbering
                eWriter.writeLocalIDs("edgeID");
            }

            writeAllEdgeFields
            (
                eWriter,
                areaMesh,
                faObjs,
                true  // syncPar
            );
        }

        fileName outputName(writer.output());

        writer.close();

        if (UPstream::master())
        {
            // Add to file-series and emit as JSON

            fileName seriesName(vtk::seriesWriter::base(outputName));

            vtk::seriesWriter& series = vtkSeries(seriesName);

            // First time?
            // Load from file, verify against filesystem,
            // prune time >= currentTime
            if (series.empty())
            {
                series.load(seriesName, true, timeValue);
            }

            series.append(timeValue, outputName);
            series.write(seriesName);
        }
    }
}


// ************************************************************************* //
