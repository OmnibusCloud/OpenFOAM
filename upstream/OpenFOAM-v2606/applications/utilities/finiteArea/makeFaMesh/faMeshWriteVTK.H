/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2021-2026 OpenCFD Ltd.
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM, distributed under GPL-3.0-or-later.

Description
    VTK output of faMesh with some geometric or debug fields

\*---------------------------------------------------------------------------*/

// Preferred output format
const vtk::outputOptions outputOpts =
    vtk::formatType::INLINE_ASCII;  // vtk::formatType::INLINE_BASE64;

// The base name for output files
word vtkBaseFileName(aMesh.regionName());
if (polyMesh::regionName(vtkBaseFileName).empty())
{
    vtkBaseFileName = "finiteArea";
}

Info<< nl;
Info<< "Write finite-area mesh in vtk format:" << nl;

{
    // finiteArea - faces
    const word& outputName = vtkBaseFileName;

    vtk::uindirectPatchWriter writer
    (
        aMesh.patch(),
        outputOpts,
        (aMesh.time().globalPath() / outputName),
        UPstream::parRun()
    );

    writer.writeGeometry();

    // CellData
    writer.beginCellData(writer.parallel() ? 4 : 3);
    writer.writeProcIDs();
    // Cell ids in global numbering
    writer.writeGlobalIDs("cellID");

    writer.write("area", aMesh.S().field());
    writer.write("normal", aMesh.faceAreaNormals());

    // PointData
    writer.beginPointData(1);
    writer.write("normal", aMesh.pointAreaNormals());

    Info<< "    " << writer.output().name() << nl;
}


// Per-proc edge output (debugging)
if (args.found("write-edges-vtk"))
{
    // The base name for output file
    word outputName(vtkBaseFileName + "-edges");

    if (UPstream::parRun())
    {
        outputName += ("-proc" + Foam::name(UPstream::myProcNo()));
    }

    // finiteArea - edges.
    // NB: edges are with reordered boundaries (not primitive patch order)
    vtk::lineWriter writer
    (
        aMesh.points(),
        aMesh.edges(),
        (aMesh.time().globalPath() / outputName),
        false  // parallel = false. every rank writes separately
    );

    writer.writeGeometry();

    // CellData
    writer.beginCellData(UPstream::parRun() ? 4 : 3);
    if (UPstream::parRun())
    {
        // DIY for procID since each writer is non-parallel
        writer.writeUniform("procID", label(UPstream::myProcNo()));
    }

    // Edge ids in rank-local numbering (useful for comparing to edgeMap etc)
    writer.writeLocalIDs("edgeID");
    // Edge ids (patch order) in rank-local numbering
    writer.writeCellData("edgeLabel", faMeshTools::edgeLabels(aMesh));

    // Patch ids
    {
        labelList values(aMesh.nEdges());

        // Internal
        label patchId = -1;
        values = patchId;

        // Boundaries
        label start = aMesh.nInternalEdges();
        for (const auto& p : aMesh.boundary())
        {
            const auto count = p.nEdges();
            values.slice(start, count) = ++patchId;
            start += count;
        }

        writer.writeCellData("patchID", values);
    }

    Pout<< "    " << writer.output().name() << nl;
}


// ************************************************************************* //
