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

    writer.writeCellData("area", aMesh.S().field());
    writer.writeCellData("normal", aMesh.faceAreaNormals());

    // PointData
    writer.beginPointData(1);
    writer.writePointData("normal", aMesh.pointAreaNormals());

    Info<< "    " << writer.output().name() << nl;
}

{
    // finiteArea - edges
    const word outputName(vtkBaseFileName + "-edges");

    // NB: edges are upper triangular, not primitive patch order
    vtk::lineWriter writer
    (
        aMesh.points(),
        aMesh.edges(),
        outputOpts,
        (aMesh.time().globalPath() / outputName),
        UPstream::parRun()
    );

    writer.writeGeometry();

    // CellData
    writer.beginCellData(writer.parallel() ? 4 : 3);
    writer.writeProcIDs();
    // Edge ids in rank-local numbering (useful for comparing to edgeMap etc)
    writer.writeLocalIDs("edgeID");
    // Edge ids (patch order) in rank-local numbering
    writer.writeCellData("edgeLabel", faMeshTools::edgeLabels(aMesh));

    {
        Field<scalar> fld
        (
            faMeshTools::flattenEdgeField(aMesh.magLe())
        );
        writer.writeCellData("magLe", fld);
    }

    // PointData
    writer.beginPointData(1);
    writer.writePointData("normal", aMesh.pointAreaNormals());

    Info<< "    " << writer.output().name() << nl;
}

{
    // finiteArea - edgesCentres
    // (no other convenient way to display vectors on the edges)
    const word outputName(vtkBaseFileName + "-edgesCentres");

    // NB: edges are upper triangular, not primitive patch order
    const Field<vector> edgeCentres
    (
        faMeshTools::flattenEdgeField(aMesh.edgeCentres())
    );

    vtk::vertexWriter writer
    (
        edgeCentres,
        outputOpts,
        (aMesh.time().globalPath() / outputName),
        UPstream::parRun()
    );

    writer.writeGeometry();

    // CellData
    writer.beginCellData(writer.parallel() ? 3 : 2);
    writer.writeProcIDs();
    // Edge ids in rank-local numbering (useful for comparing to edgeMap etc)
    writer.writeLocalIDs("edgeID");
    // Edge ids (patch order) in rank-local numbering
    writer.writeCellData("edgeLabel", faMeshTools::edgeLabels(aMesh));


    // PointData
    writer.beginPointData(3);
    writer.writeLocalIDs("edgeID");
    {
        Field<vector> fld(aMesh.nEdges());

        faMeshTools::flattenEdgeField(fld, aMesh.Le());
        writer.writePointData("Le", fld);

        faMeshTools::flattenEdgeField(fld, aMesh.edgeAreaNormals());
        writer.writePointData("normal", fld);
    }

    Info<< "    " << writer.output().name() << nl;
}


// Per-proc edge output (debugging)
if (UPstream::parRun() && args.found("write-edges-vtk"))
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
        outputOpts,
        (aMesh.time().globalPath() / outputName),
        false  // parallel = false. Every rank writes into separate files!
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

    Info<< "    " << writer.output().name() << nl;
}


if (UPstream::parRun() && args.found("write-edges-vtk"))
{
    // finiteArea - edgesCentres
    // (no other convenient way to display vectors on the edges)
    word outputName(vtkBaseFileName + "-edgesCentres");

    if (UPstream::parRun())
    {
        outputName += ("-proc" + Foam::name(UPstream::myProcNo()));
    }

    // NB: edges are upper triangular, not primitive patch order
    const Field<vector> edgeCentres
    (
        faMeshTools::flattenEdgeField(aMesh.edgeCentres())
    );

    vtk::vertexWriter writer
    (
        edgeCentres,
        outputOpts,
        (aMesh.time().globalPath() / outputName),
        false  // parallel = false. Every rank writes into separate files!
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


    // PointData
    writer.beginPointData(3);
    writer.writeLocalIDs("edgeID");
    {
        Field<vector> fld(aMesh.nEdges());

        faMeshTools::flattenEdgeField(fld, aMesh.Le());
        writer.writePointData("Le", fld);

        faMeshTools::flattenEdgeField(fld, aMesh.edgeAreaNormals());
        writer.writePointData("normal", fld);
    }

    Pout<< "    " << writer.output().name() << nl;
}


// ************************************************************************* //
