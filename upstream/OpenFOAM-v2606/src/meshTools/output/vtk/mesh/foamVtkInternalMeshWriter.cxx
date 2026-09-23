/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2017-2026 OpenCFD Ltd.
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

\*---------------------------------------------------------------------------*/

#include "foamVtkInternalMeshWriter.H"
#include "globalIndex.H"
#include "Time.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

int Foam::vtk::internalMeshWriter::debug = 0;


// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

void Foam::vtk::internalMeshWriter::beginPiece()
{
    // Basic sizes
    pointSlab_ = vtuCells_.nFieldPoints();  // With addPointCellLabels
    cellSlab_  = vtuCells_.nFieldCells();   // With decomposed cells

    if (parallel_)
    {
        Foam::reduceOffsets
        (
            UPstream::worldComm,
            pointSlab_,
            cellSlab_
        );
    }

    DebugInFunction
        << "nPoints=" << nTotalPoints()
        << " nCells=" << nTotalCells() << nl;

    // Nothing else to do for legacy
    if (legacy()) return;

    if (format_)
    {
        format()
            .tag
            (
                vtk::fileTag::PIECE,
                vtk::fileAttr::NUMBER_OF_POINTS, nTotalPoints(),
                vtk::fileAttr::NUMBER_OF_CELLS,  nTotalCells()
            );
    }
}


void Foam::vtk::internalMeshWriter::writePoints()
{
    // Additional support points for decomposed polys
    const UIndirectList<point> centres
    (
        mesh_.cellCentres(),
        vtuCells_.addPointCellLabels()
    );

    this->beginPoints(nTotalPoints());

    if (parallel_)
    {
        if (returnReduceAnd(vtuCells_.merged()))
        {
            const UIndirectList<point> points
            (
                mesh_.points(),
                vtuCells_.pointMap()
            );

            vtk::writeListsParallel(format_.ref(), points, centres);
        }
        else
        {
            // No pointMap...
            vtk::writeListsParallel(format_.ref(), mesh_.points(), centres);
        }
    }
    else
    {
        // No pointMap for non-parallel...
        vtk::writeLists(format(), mesh_.points(), centres);
    }

    this->endPoints();
}


void Foam::vtk::internalMeshWriter::writeCells_legacy()
{
    // The processor-local point offset (if not already merged)
    const label pointOffset = (vtuCells_.merged() ? 0 : pointSlab_.start());

    const UList<uint8_t>& cellTypes = vtuCells_.cellTypes();
    const auto tvertLabels = vtuCells_.vertLabels(pointOffset);
    const auto& vertLabels = tvertLabels();

    label nVerts = vertLabels.size();

    if (parallel_)
    {
        reduce(nVerts, sumOp<label>());
    }

    // CELLS
    {
        if (format_)
        {
            os_ << nl
                << "CELLS " << nTotalCells() << ' ' << nVerts << nl;
        }

        if (parallel_)
        {
            vtk::writeListParallel(format_.ref(), vertLabels);
        }
        else
        {
            vtk::writeList(format(), vertLabels);
        }

        if (format_)
        {
            format().flush();
        }
    }


    // CELL_TYPES
    {
        if (format_)
        {
            os_ << nl
                << "CELL_TYPES " << nTotalCells() << nl;
        }

        if (parallel_)
        {
            vtk::writeListParallel(format_.ref(), cellTypes);
        }
        else
        {
            vtk::writeList(format(), cellTypes);
        }

        if (format_)
        {
            format().flush();
        }
    }
}


void Foam::vtk::internalMeshWriter::writeCellsConnectivity()
{
    // The processor-local point offset (if not already merged)
    const label pointOffset = (vtuCells_.merged() ? 0 : pointSlab_.start());

    //
    // 'connectivity'
    //
    {
        const auto tvertLabels = vtuCells_.vertLabels(pointOffset);
        const auto& vertLabels = tvertLabels();
        label nVerts = vertLabels.size();

        if (parallel_)
        {
            reduce(nVerts, sumOp<label>());
        }

        if (format_)
        {
            const uint64_t payLoad =
                vtk::sizeofData<label>(nVerts);

            format().beginDataArray<label>(vtk::dataArrayAttr::CONNECTIVITY);
            format().writeSize(payLoad);
        }

        if (parallel_)
        {
            vtk::writeListParallel(format_.ref(), vertLabels);
        }
        else
        {
            vtk::writeList(format(), vertLabels);
        }

        if (format_)
        {
            format().flush();
            format().endDataArray();
        }
    }


    //
    // 'offsets'  (connectivity offsets)
    // NOTE: in the XML format these are the end offsets only,
    // so they won't overlap when concatenated in parallel.
    //
    {
        const labelUList& vertOffsets = vtuCells_.vertOffsets();
        label nOffsets = vertOffsets.size();

        if (parallel_)
        {
            reduce(nOffsets, sumOp<label>());
        }

        if (format_)
        {
            const uint64_t payLoad =
                vtk::sizeofData<label>(nOffsets);

            format().beginDataArray<label>(vtk::dataArrayAttr::OFFSETS);
            format().writeSize(payLoad);
        }

        if (parallel_)
        {
            // processor-local connectivity offsets
            const globalIndex procOffset
            (
                globalIndex::gatherOnly{},
                (vertOffsets.empty() ? 0 : vertOffsets.back())
            );

            vtk::writeListParallel(format_.ref(), vertOffsets, procOffset);
        }
        else
        {
            vtk::writeList(format(), vertOffsets);
        }

        if (format_)
        {
            format().flush();
            format().endDataArray();
        }
    }


    //
    // 'types' (cell types)
    //
    {
        const UList<uint8_t>& cellTypes = vtuCells_.cellTypes();

        if (format_)
        {
            const uint64_t payLoad =
                vtk::sizeofData<uint8_t>(nTotalCells());

            format().beginDataArray<uint8_t>(vtk::dataArrayAttr::TYPES);
            format().writeSize(payLoad);
        }

        if (parallel_)
        {
            vtk::writeListParallel(format_.ref(), cellTypes);
        }
        else
        {
// FIXME: clang-13 optimization jumps into incorrect branch
            #ifdef __clang__
            checkFormatterValidity();
            #endif

            vtk::writeList(format(), cellTypes);
        }

        if (format_)
        {
            format().flush();
            format().endDataArray();
        }
    }
}


void Foam::vtk::internalMeshWriter::writeCellsFaces()
{
    // The processor-local point offset (if not already merged)
    const label pointOffset = (vtuCells_.merged() ? 0 : pointSlab_.start());

    // Slab addressing for (polyhedral) face labels
    OffsetRange<label> facesSlab(vtuCells_.faceLabels().size());

    if (parallel_)
    {
        Foam::reduceOffset(facesSlab, UPstream::worldComm);
    }

    // Can quit now if there are NO face streams
    if (!facesSlab.total())
    {
        return;
    }

    // --------------------------------------------------

    //
    // 'faces' (face streams)
    //
    const auto tfaceLabels = vtuCells_.faceLabels(pointOffset);
    const auto& faceLabels = tfaceLabels();

    {
        // Already determined the total number of faces (above)

        if (format_)
        {
            const uint64_t payLoad =
                vtk::sizeofData<label>(facesSlab.total());

            format().beginDataArray<label>(vtk::dataArrayAttr::FACES);
            format().writeSize(payLoad);
        }


        if (parallel_)
        {
            vtk::writeListParallel(format_.ref(), faceLabels);
        }
        else
        {
            vtk::writeList(format(), faceLabels);
        }


        if (format_)
        {
            format().flush();
            format().endDataArray();
        }
    }

    // 'faceoffsets' (face stream offsets)
    // -1 to indicate that the cell is a primitive type that does not
    // have a face stream

    // If the processor-local mesh has any polyhedrals, we have a list with
    // the faceoffsets and we just need to renumber.
    // If the processor-local mesh has NO polyhedrals (but others do), we
    // use a dummy list of face offsets for that processor.
    //
    // Result: A face offset value for each cell.
    {
        // Note: have nCells offsets per rank (-1 for primitives) but the
        // The faceoffsets themselves need to be shifted by the
        // real number of faces per rank

        // Processor-local offsets are based on the number of face labels
        const label beginOffset = facesSlab.start();

        const auto tfaceOffsets = vtuCells_.faceOffsets(beginOffset);
        const auto& faceOffsets = tfaceOffsets();

        if (format_)
        {
            const uint64_t payLoad =
                vtk::sizeofData<label>(nTotalCells());

            format().beginDataArray<label>(vtk::dataArrayAttr::FACEOFFSETS);
            format().writeSize(payLoad);
        }

        if (parallel_)
        {
            vtk::writeListParallel(format_.ref(), faceOffsets);
        }
        else
        {
            vtk::writeList(format(), faceOffsets);
        }


        if (format_)
        {
            format().flush();
            format().endDataArray();
        }
    }
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::vtk::internalMeshWriter::internalMeshWriter
(
    const polyMesh& mesh,
    const vtk::vtuCells& cells,
    const vtk::outputOptions opts
)
:
    vtk::fileWriter(vtk::fileTag::UNSTRUCTURED_GRID, opts),
    mesh_(mesh),
    vtuCells_(cells),
    pointSlab_(0),
    cellSlab_(0)
{
    // We do not currently support append mode
    opts_.append(false);
}


Foam::vtk::internalMeshWriter::internalMeshWriter
(
    const polyMesh& mesh,
    const vtk::vtuCells& cells,
    const fileName& file,
    bool parallel
)
:
    internalMeshWriter(mesh, cells)
{
    open(file, parallel);
}


Foam::vtk::internalMeshWriter::internalMeshWriter
(
    const polyMesh& mesh,
    const vtk::vtuCells& cells,
    const vtk::outputOptions opts,
    const fileName& file,
    bool parallel
)
:
    internalMeshWriter(mesh, cells, opts)
{
    open(file, parallel);
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

bool Foam::vtk::internalMeshWriter::beginFile(std::string title)
{
    const auto& runTime = mesh_.time();

    if (title.size())
    {
        return vtk::fileWriter::beginFile(title);
    }

    // Provide default title

    DebugInFunction
        << "case=" << runTime.caseName()
        << " region=" << mesh_.name()
        << " time=" << runTime.timeName()
        << " index=" << runTime.timeIndex() << endl;

    if (legacy())
    {
        return vtk::fileWriter::beginFile
        (
            runTime.globalCaseName()
        );
    }

    // XML (inline)

    return vtk::fileWriter::beginFile
    (
        "case='" + runTime.globalCaseName()
      + "' region='" + mesh_.name()
      + "' time='" + runTime.timeName()
      + "' index='" + Foam::name(runTime.timeIndex())
      + "'"
    );
}


bool Foam::vtk::internalMeshWriter::writeGeometry()
{
    enter_Piece();

    beginPiece();

    writePoints();

    if (legacy())
    {
        writeCells_legacy();
        return true;
    }

    if (format_)
    {
        format().tag(vtk::fileTag::CELLS);
    }

    writeCellsConnectivity();
    writeCellsFaces();

    if (format_)
    {
        format().endTag(vtk::fileTag::CELLS);
    }

    return true;
}


bool Foam::vtk::internalMeshWriter::beginCellData(label nFields)
{
    return enter_CellData(nTotalCells(), nFields);
}


bool Foam::vtk::internalMeshWriter::beginPointData(label nFields)
{
    return enter_PointData(nTotalPoints(), nFields);
}


void Foam::vtk::internalMeshWriter::writeCellIDs()
{
    if (isState(outputState::CELL_DATA))
    {
        ++nCellData_;
    }
    else
    {
        reportBadState(FatalErrorInFunction, outputState::CELL_DATA)
            << " for cellID field" << nl << endl
            << exit(FatalError);
    }

    const labelUList& cellMap = vtuCells_.cellMap();

    this->beginDataArray<label>("cellID", nTotalCells());

    if (parallel_)
    {
        const globalIndex globalCellOffset
        (
            globalIndex::gatherOnly{},
            vtuCells_.nFieldCells()
        );

        vtk::writeListParallel(format_.ref(), cellMap, globalCellOffset);
    }
    else
    {
        vtk::writeList(format(), cellMap);
    }

    this->endDataArray();
}


bool Foam::vtk::internalMeshWriter::writeProcIDs()
{
    if (!parallel_)
    {
        // Disabled in serial output (meaningless)
        return false;
    }

    return vtk::fileWriter::writeProcIDs(vtuCells_.nFieldCells());
}


void Foam::vtk::internalMeshWriter::writePointIDs()
{
    if (isState(outputState::POINT_DATA))
    {
        ++nPointData_;
    }
    else
    {
        reportBadState(FatalErrorInFunction, outputState::POINT_DATA)
            << " for pointID field" << nl << endl
            << exit(FatalError);
    }


    this->beginDataArray<label>("pointID", nTotalPoints());

    // Point offset for regular mesh points (without decomposed)
    const label pointOffset =
    (
        (parallel_ && !vtuCells_.merged())
      ? globalIndex::calcOffset(vtuCells_.nPoints()) : 0
    );

    // Cell offset for regular mesh cells (without decomposed)
    const label cellOffset =
    (
        parallel_ ? globalIndex::calcOffset(vtuCells_.nCells()) : 0
    );


    labelList pointIds = identity(vtuCells_.nFieldPoints(), pointOffset);

    if (const auto& map = vtuCells_.pointMap(); !map.empty())
    {
        // Actually using a point map
        labelList::subList(pointIds, map.size()) = map;
    }

    // The pointID for added points is the cellID, tag as a negative number
    label pointi = vtuCells_.nPoints();
    for (const label celli : vtuCells_.addPointCellLabels())
    {
        pointIds[pointi] = (-1 - celli - cellOffset);
        ++pointi;
    }

    if (parallel_)
    {
        vtk::writeListParallel(format_.ref(), pointIds);
    }
    else
    {
        vtk::writeList(format(), pointIds);
    }

    this->endDataArray();
}


// ************************************************************************* //
