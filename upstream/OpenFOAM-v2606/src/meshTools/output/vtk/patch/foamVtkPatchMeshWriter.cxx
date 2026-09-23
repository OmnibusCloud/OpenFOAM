/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2016-2025 OpenCFD Ltd.
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

#include "foamVtkPatchMeshWriter.H"
#include "foamVtkOutput.H"
#include "globalIndex.H"
#include "Time.H"
#include "processorPolyPatch.H"

// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

void Foam::vtk::patchMeshWriter::beginPiece()
{
    // Note: also store the connectivity counts for this writer.
    // The additional information has minimal storage and it avoids
    // re-walking all faces of all patch types (again) and repeating
    // the communication.

    // Basic sizes
    const polyBoundaryMesh& patches = mesh_.boundaryMesh();

    label nPoints = 0;      // Number of points
    label nFaces = 0;       // Number of faces
    label nConnectivity = 0; // Connectivity = sum of number of face points

    for (const label patchId : patchIDs_)
    {
        const polyPatch& pp = patches[patchId];

        nPoints += pp.nPoints();
        nFaces  += pp.size();

        for (const face& f : pp)
        {
            nConnectivity += f.size();
        }
    }

    pointSlab_ = nPoints;
    cellSlab_  = nFaces;
    connectivitySlab_ = nConnectivity;

    if (parallel_)
    {
        Foam::reduceOffsets
        (
            UPstream::worldComm,
            pointSlab_,
            cellSlab_,
            connectivitySlab_
        );
    }


    // Nothing else to do for legacy
    if (legacy()) return;


    if (format_)
    {
        format()
            .tag
            (
                vtk::fileTag::PIECE,
                vtk::fileAttr::NUMBER_OF_POINTS, nTotalPoints(),
                vtk::fileAttr::NUMBER_OF_POLYS,  nTotalCells()
            );
    }
}


void Foam::vtk::patchMeshWriter::writePoints()
{
    const polyBoundaryMesh& patches = mesh_.boundaryMesh();

    this->beginPoints(nTotalPoints());

    if (parallel_ ? UPstream::master() : bool(format_))
    {
        for (const label patchId : patchIDs_)
        {
            const polyPatch& pp = patches[patchId];

            vtk::writeList(format(), pp.localPoints());
        }
    }


    if (parallel_)
    {
        // Patch Ids are identical across all processes
        const label nPatches = patchIDs_.size();

        if (UPstream::master())
        {
            pointField recv;

            // Receive each point field and write
            for (const int proci : UPstream::subProcs())
            {
                IPstream fromProc(UPstream::commsTypes::scheduled, proci);

                for (label i=0; i < nPatches; ++i)
                {
                    fromProc >> recv;

                    vtk::writeList(format(), recv);
                }
            }
        }
        else
        {
            // Send each point field
            OPstream toProc
            (
                UPstream::commsTypes::scheduled,
                UPstream::masterNo()
            );

            for (const label patchId : patchIDs_)
            {
                const polyPatch& pp = patches[patchId];

                toProc << pp.localPoints();
            }
        }
    }


    this->endPoints();
}


void Foam::vtk::patchMeshWriter::writePolys_legacy()
{
    // The processor-local point offset
    const label pointOffset = pointSlab_.start();

    const polyBoundaryMesh& patches = mesh_.boundaryMesh();

    // Connectivity count without additional storage (done internally)
    const label nVerts = connectivitySlab_.total();

    legacy::beginPolys(os_, nTotalCells(), nVerts);

    // Local work array for the face connectivity.
    // The legacy format includes extra nPts prefix for each face
    labelList vertLabels
    (
        cellSlab_.size() + connectivitySlab_.size()
    );

    {
        // Legacy: size + connectivity together
        // [nPts, id1, id2, ..., nPts, id1, id2, ...]

        auto iter = vertLabels.begin();

        label off = pointOffset;

        for (const label patchId : patchIDs_)
        {
            const polyPatch& pp = patches[patchId];

            for (const face& f : pp.localFaces())
            {
                *iter = f.size();       // The size prefix
                ++iter;

                for (const label id : f)
                {
                    *iter = id + off;   // Face vertex label
                    ++iter;
                }
            }
            off += pp.nPoints();
        }
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


void Foam::vtk::patchMeshWriter::writePolys()
{
    // The processor-local point offset
    const label pointOffset = pointSlab_.start();

    if (format_)
    {
        format().tag(vtk::fileTag::POLYS);
    }

    const polyBoundaryMesh& patches = mesh_.boundaryMesh();

    //
    // 'connectivity'
    //
    {
        // Local work array for the face connectivity
        labelList vertLabels(connectivitySlab_.size());

        const label nVerts = connectivitySlab_.total();

        if (format_)
        {
            const uint64_t payLoad =
                vtk::sizeofData<label>(nVerts);

            format().beginDataArray<label>(vtk::dataArrayAttr::CONNECTIVITY);
            format().writeSize(payLoad);
        }

        {
            // XML: connectivity only
            // [id1, id2, ..., id1, id2, ...]

            auto iter = vertLabels.begin();

            label off = pointOffset;

            for (const label patchId : patchIDs_)
            {
                const polyPatch& pp = patches[patchId];

                for (const face& f : pp.localFaces())
                {
                    for (const label id : f)
                    {
                        *iter = id + off;   // Face vertex label
                        ++iter;
                    }
                }
                off += pp.nPoints();
            }
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
    // For XML these are non-overlapping [end] offsets. One per element.
    //
    {
        // Local work array for the offsets into connectivity
        labelList vertOffsets(cellSlab_.size());

        const label nOffsets = cellSlab_.total();

        if (format_)
        {
            const uint64_t payLoad =
                vtk::sizeofData<label>(nOffsets);

            format().beginDataArray<label>(vtk::dataArrayAttr::OFFSETS);
            format().writeSize(payLoad);
        }


        // processor-local connectivity offsets
        label off = connectivitySlab_.start();

        auto iter = vertOffsets.begin();

        for (const label patchId : patchIDs_)
        {
            const polyPatch& pp = patches[patchId];

            for (const face& f : pp)
            {
                off += f.size();   // End offset
                *iter = off;
                ++iter;
            }
        }


        if (parallel_)
        {
            vtk::writeListParallel(format_.ref(), vertOffsets);
        }
        else
        {
            vtk::writeList(format_.ref(), vertOffsets);
        }


        if (format_)
        {
            format().flush();
            format().endDataArray();
        }
    }

    if (format_)
    {
        format().endTag(vtk::fileTag::POLYS);
    }
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::vtk::patchMeshWriter::patchMeshWriter
(
    const polyMesh& mesh,
    const labelUList& patchIDs,
    const vtk::outputOptions opts
)
:
    vtk::fileWriter(vtk::fileTag::POLY_DATA, opts),
    mesh_(mesh),
    patchIDs_(),
    pointSlab_(0),
    cellSlab_(0),
    connectivitySlab_(0)
{
    // We do not currently support append mode
    opts_.append(false);

    if (const label len = patchIDs.size(); len > 1)
    {
        // Have two or more patches selected.
        // Sort and remove any possible duplicates for overall consistency.

        // Can use patchIDs_ both for the initial sort order as well
        // as the output, since it will be non-overlapping.

        auto& order = patchIDs_;
        Foam::sortedOrder(patchIDs, order);

        patchIDs_[0] = patchIDs[order[0]];

        label nUnique = 1;
        label prev = patchIDs_[0];

        for (label i = 1; i < len; ++i)
        {
            label patchId = patchIDs[order[i]];

            if (prev != patchId)
            {
                prev = patchId;
                patchIDs_[nUnique] = patchId;
                ++nUnique;
            }
        }

        patchIDs_.resize(nUnique);
    }
    else
    {
        // Trivial case
        patchIDs_ = patchIDs;
    }
}


Foam::vtk::patchMeshWriter::patchMeshWriter
(
    const polyMesh& mesh,
    const labelUList& patchIDs,
    const fileName& file,
    bool parallel
)
:
    patchMeshWriter(mesh, patchIDs)
{
    open(file, parallel);
}


Foam::vtk::patchMeshWriter::patchMeshWriter
(
    const polyMesh& mesh,
    const labelUList& patchIDs,
    const vtk::outputOptions opts,
    const fileName& file,
    bool parallel
)
:
    patchMeshWriter(mesh, patchIDs, opts)
{
    open(file, parallel);
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

bool Foam::vtk::patchMeshWriter::beginFile(std::string title)
{
    const auto& runTime = mesh_.time();

    if (title.size())
    {
        return vtk::fileWriter::beginFile(title);
    }

    // Provide default title

    if (legacy())
    {
        title =
        (
            patchIDs_.size() == 1
          ? mesh_.boundaryMesh()[patchIDs_.first()].name()
          : "patches"
        );

        return vtk::fileWriter::beginFile(title);
    }


    // XML (inline)

    if (patchIDs_.size() == 1)
    {
        title =
        (
            "patch='" + mesh_.boundaryMesh()[patchIDs_.first()].name() + "'"
        );
    }
    else
    {
        title =
        (
            "npatches='" + Foam::name(patchIDs_.size()) + "'"
        );
    }

    title +=
    (
        " time='" + runTime.timeName()
      + "' index='" + Foam::name(runTime.timeIndex())
      + "'"
    );

    return vtk::fileWriter::beginFile(title);
}


bool Foam::vtk::patchMeshWriter::writeGeometry()
{
    enter_Piece();

    beginPiece();

    writePoints();

    if (legacy())
    {
        writePolys_legacy();
    }
    else
    {
        writePolys();
    }

    return true;
}


bool Foam::vtk::patchMeshWriter::beginCellData(label nFields)
{
    return enter_CellData(nTotalCells(), nFields);
}


bool Foam::vtk::patchMeshWriter::beginPointData(label nFields)
{
    return enter_PointData(nTotalPoints(), nFields);
}


void Foam::vtk::patchMeshWriter::writePatchIDs()
{
    if (isState(outputState::CELL_DATA))
    {
        ++nCellData_;
    }
    else
    {
        reportBadState(FatalErrorInFunction, outputState::CELL_DATA)
            << " for patchID field" << nl << endl
            << exit(FatalError);
    }

    const polyBoundaryMesh& patches = mesh_.boundaryMesh();


    this->beginDataArray<label>("patchID", nTotalCells());

    if (parallel_ ? UPstream::master() : bool(format_))
    {
        for (const label patchId : patchIDs_)
        {
            vtk::write(format(), patchId, patches[patchId].size());
        }
    }

    if (parallel_)
    {
        // Same number of patches on all ranks,
        // can use a constant size buffer

        labelList buffer(2*patchIDs_.size());

        if (UPstream::master())
        {
            for (const int proci : UPstream::subProcs())
            {
                UIPstream::read
                (
                    UPstream::commsTypes::scheduled,
                    proci,
                    buffer
                );

                // Receive as [id, size] tuples
                for (label i = 0; i < buffer.size(); i += 2)
                {
                    const label val = buffer[i];
                    const label len = buffer[i+1];

                    vtk::write(format(), val, len);
                }
            }
        }
        else
        {
            // Encode as [id, size] tuples
            label i = 0;

            for (const label patchId : patchIDs_)
            {
                buffer[i] = patchId;
                buffer[i+1] = patches[patchId].size();
                i += 2;
            }

            UOPstream::write
            (
                UPstream::commsTypes::scheduled,
                UPstream::masterNo(),
                buffer
            );
        }
    }

    this->endDataArray();
}


bool Foam::vtk::patchMeshWriter::writeProcIDs()
{
    return vtk::fileWriter::writeProcIDs
    (
        // Appropriate rank-local size:
        (this->isPointData() ? pointSlab_.size() : cellSlab_.size())
    );
}


bool Foam::vtk::patchMeshWriter::writeNeighIDs()
{
    if (!UPstream::parRun())
    {
        // Skip in non-parallel
        return false;
    }

    if (isState(outputState::CELL_DATA))
    {
        ++nCellData_;
    }
    else
    {
        reportBadState(FatalErrorInFunction, outputState::CELL_DATA)
            << " for patchID field" << nl << endl
            << exit(FatalError);
    }

    const polyBoundaryMesh& patches = mesh_.boundaryMesh();

    this->beginDataArray<label>("neighID", nTotalCells());

    bool good = true;

    if (parallel_ ? UPstream::master() : bool(format_))
    {
        for (const label patchId : patchIDs_)
        {
            const auto* pp = isA<processorPolyPatch>(patches[patchId]);

            const label val = (pp ? pp->neighbProcNo() : -1);

            vtk::write(format(), val, patches[patchId].size());
        }
    }

    if (parallel_)
    {
        // Same number of patches on all ranks,
        // can use a constant size buffer

        labelList buffer(2*patchIDs_.size());

        if (UPstream::master())
        {
            for (const int proci : UPstream::subProcs())
            {
                UIPstream::read
                (
                    UPstream::commsTypes::scheduled,
                    proci,
                    buffer
                );

                // Receive as [id, size] tuples
                for (label i = 0; i < buffer.size(); i += 2)
                {
                    const label val = buffer[i];
                    const label len = buffer[i+1];

                    vtk::write(format(), val, len);
                }
            }
        }
        else
        {
            // Encode as [id, size] tuples
            label i = 0;
            for (const label patchId : patchIDs_)
            {
                const auto* pp = isA<processorPolyPatch>(patches[patchId]);

                buffer[i] = (pp ? pp->neighbProcNo() : -1);
                buffer[i+1] = patches[patchId].size();
                i += 2;
            }

            UOPstream::write
            (
                UPstream::commsTypes::scheduled,
                UPstream::masterNo(),
                buffer
            );
        }

        // MPI barrier
        Pstream::broadcast(good);
    }

    this->endDataArray();

    return good;
}


// ************************************************************************* //
