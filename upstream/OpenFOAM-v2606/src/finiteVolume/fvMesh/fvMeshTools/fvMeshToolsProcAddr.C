/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2015-2026 OpenCFD Ltd.
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

#include "fvMeshTools.H"
#include "fileOperation.H"
#include "IndirectList.H"
#include "labelRange.H"
#include "IOmapDistributePolyMesh.H"
#include "OSspecific.H"

// * * * * * * * * * * * * * * * Local Functions * * * * * * * * * * * * * * //

namespace Foam
{

// Create a reconstruct map.
// The baseMeshPtr is non-null (and probably has cells) on the master
// is ignored elsewhere.
//
// The incoming faceProcAddressing is assumed to have flip addressing.
static autoPtr<mapDistributePolyMesh> createReconstructMap
(
    const fvMesh& mesh,
    const fvMesh* baseMeshPtr,
    const labelUList& cellProcAddressing,
    const labelUList& faceProcAddressing,
    const labelUList& pointProcAddressing,
    const labelUList& boundaryProcAddressing
)
{
    const label nOldPoints = mesh.nPoints();
    const label nOldFaces = mesh.nFaces();
    const label nOldCells = mesh.nCells();

    const label numProc = UPstream::nProcs();
    const label myProci = UPstream::myProcNo();

    const polyBoundaryMesh& pbm = mesh.boundaryMesh();

    labelList oldPatchStarts(pbm.size());
    labelList oldPatchNumPoints(pbm.size());
    forAll(pbm, patchi)
    {
        oldPatchStarts[patchi] = pbm[patchi].start();
        oldPatchNumPoints[patchi] = pbm[patchi].nPoints();
    }

    // Patches: purge -1 entries
    labelList patchProcAddressing
    (
        IndirectList<label>::subset_if
        (
            boundaryProcAddressing,
            labelRange::ge0()
        )
    );


    labelListList cellSubMap(numProc);
    cellSubMap[Pstream::masterNo()] = identity(nOldCells);

    labelListList faceSubMap(numProc);
    faceSubMap[Pstream::masterNo()] = identity(nOldFaces);

    labelListList pointSubMap(numProc);
    pointSubMap[Pstream::masterNo()] = identity(nOldPoints);

    labelListList patchSubMap(numProc);
    patchSubMap[Pstream::masterNo()] = patchProcAddressing;


    // Gather addressing on master
    labelListList cellAddressing(numProc);
    cellAddressing[myProci] = cellProcAddressing;
    Pstream::gatherList(cellAddressing);

    labelListList faceAddressing(numProc);
    faceAddressing[myProci] = faceProcAddressing;
    Pstream::gatherList(faceAddressing);

    labelListList pointAddressing(numProc);
    pointAddressing[myProci] = pointProcAddressing;
    Pstream::gatherList(pointAddressing);

    labelListList patchAddressing(numProc);
    patchAddressing[myProci] = patchProcAddressing;
    Pstream::gatherList(patchAddressing);


    // NB: can only have a reconstruct on master!
    if (UPstream::master() && baseMeshPtr && baseMeshPtr->nCells())
    {
        const fvMesh& baseMesh = *baseMeshPtr;

        const label nNewPoints = baseMesh.nPoints();
        const label nNewFaces = baseMesh.nFaces();
        const label nNewCells = baseMesh.nCells();
        const label nNewPatches = baseMesh.boundaryMesh().size();

        mapDistribute cellMap
        (
            nNewCells,
            std::move(cellSubMap),
            std::move(cellAddressing)
        );

        mapDistribute faceMap
        (
            nNewFaces,
            std::move(faceSubMap),
            std::move(faceAddressing),
            false,  // subHasFlip
            true    // constructHasFlip
        );

        mapDistribute pointMap
        (
            nNewPoints,
            std::move(pointSubMap),
            std::move(pointAddressing)
        );

        mapDistribute patchMap
        (
            nNewPatches,
            std::move(patchSubMap),
            std::move(patchAddressing)
        );

        return autoPtr<mapDistributePolyMesh>::New
        (
            nOldPoints,
            nOldFaces,
            nOldCells,
            std::move(oldPatchStarts),
            std::move(oldPatchNumPoints),
            std::move(pointMap),
            std::move(faceMap),
            std::move(cellMap),
            std::move(patchMap)
        );
    }
    else
    {
        // Zero-sized mesh (eg, processor mesh)

        mapDistribute cellMap
        (
            0,  // nNewCells
            std::move(cellSubMap),
            labelListList(numProc)      // constructMap
        );

        mapDistribute faceMap
        (
            0,  // nNewFaces
            std::move(faceSubMap),
            labelListList(numProc),     // constructMap
            false,  // subHasFlip
            true    // constructHasFlip
        );

        mapDistribute pointMap
        (
            0,  // nNewPoints
            std::move(pointSubMap),
            labelListList(numProc)      // constructMap
        );

        mapDistribute patchMap
        (
            0,  // nNewPatches
            std::move(patchSubMap),
            labelListList(numProc)      // constructMap
        );

        return autoPtr<mapDistributePolyMesh>::New
        (
            nOldPoints,
            nOldFaces,
            nOldCells,
            std::move(oldPatchStarts),
            std::move(oldPatchNumPoints),
            std::move(pointMap),
            std::move(faceMap),
            std::move(cellMap),
            std::move(patchMap)
        );
    }
}

} // End namespace Foam


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

Foam::autoPtr<Foam::mapDistributePolyMesh>
Foam::fvMeshTools::readProcAddressing
(
    const fvMesh& mesh,
    const fvMesh* baseMeshPtr
)
{
    // Processor-local reading
    IOobject ioAddr
    (
        "procAddressing",
        mesh.facesInstance(),
        polyMesh::meshSubDir,
        mesh.thisDb(),
        IOobject::READ_IF_PRESENT,
        IOobject::NO_WRITE,
        IOobject::NO_REGISTER
    );

    //if (ioAddr.typeHeaderOk<labelIOList>(true))
    //{
    //    Pout<< "Reading addressing from " << io.name() << " at "
    //        << mesh.facesInstance() << nl << endl;
    //    mapDistributePolyMesh distMap = IOmapDistributePolyMesh(ioAddr);
    //    return autoPtr<mapDistributePolyMesh>::New(std::move(distMap));
    //}
    //else

    {
        Info<< "Reading (cell|face|point|boundary)ProcAddressing from "
            << mesh.facesInstance().c_str() << '/'
            << mesh.meshDir().c_str() << nl << endl;

        ioAddr.resetHeader("cellProcAddressing");
        labelIOList cellProcAddressing(ioAddr);

        ioAddr.resetHeader("faceProcAddressing");
        labelIOList faceProcAddressing(ioAddr);

        ioAddr.resetHeader("pointProcAddressing");
        labelIOList pointProcAddressing(ioAddr);

        ioAddr.resetHeader("boundaryProcAddressing");
        labelIOList boundaryProcAddressing(ioAddr);

        if
        (
            mesh.nCells() != cellProcAddressing.size()
         || mesh.nPoints() != pointProcAddressing.size()
         || mesh.nFaces() != faceProcAddressing.size()
         || mesh.boundaryMesh().size() != boundaryProcAddressing.size()
        )
        {
            FatalErrorInFunction
                << "Read addressing inconsistent with mesh sizes" << nl
                << "cells:" << mesh.nCells()
                << " addressing:" << cellProcAddressing.objectRelPath()
                << " size:" << cellProcAddressing.size() << nl
                << "faces:" << mesh.nFaces()
                << " addressing:" << faceProcAddressing.objectRelPath()
                << " size:" << faceProcAddressing.size() << nl
                << "points:" << mesh.nPoints()
                << " addressing:" << pointProcAddressing.objectRelPath()
                << " size:" << pointProcAddressing.size()
                << "patches:" << mesh.boundaryMesh().size()
                << " addressing:" << boundaryProcAddressing.objectRelPath()
                << " size:" << boundaryProcAddressing.size()
                << exit(FatalError);
        }

        return createReconstructMap
        (
            mesh,
            baseMeshPtr,
            cellProcAddressing,
            faceProcAddressing,
            pointProcAddressing,
            boundaryProcAddressing
        );
    }
}


void Foam::fvMeshTools::writeProcAddressing
(
    const fvMesh& mesh,
    const mapDistributePolyMesh& map,
    const bool decompose,
    const fileName& writeInstance,
    const refPtr<fileOperation>& writeHandler
)
{
    Info<< "Writing ("
        << (decompose ? "decompose" : "reconstruct")
        << ") procAddressing files to "
        << mesh.facesInstance().c_str() << '/'
        << mesh.meshDir().c_str() << endl;

    // Processor-local outputs for components
    // NB: the full "procAddressing" output is presumed to already have
    // been done independently (as a registered object)
    IOobject ioAddr
    (
        "proc-addressing",
        mesh.facesInstance(),
        polyMesh::meshSubDir,
        mesh.thisDb(),
        IOobject::NO_READ,
        IOobject::NO_WRITE,
        IOobject::NO_REGISTER
    );

    // cellProcAddressing (polyMesh)
    ioAddr.rename("cellProcAddressing");
    labelIOList cellMap(ioAddr);

    // faceProcAddressing (polyMesh)
    ioAddr.rename("faceProcAddressing");
    labelIOList faceMap(ioAddr);

    // pointProcAddressing (polyMesh)
    ioAddr.rename("pointProcAddressing");
    labelIOList pointMap(ioAddr);

    // boundaryProcAddressing (polyMesh)
    ioAddr.rename("boundaryProcAddressing");
    labelIOList patchMap(ioAddr);


    if (decompose)
    {
        // Decompose
        // - forward map:  [undecomposed] -> [decomposed]

        cellMap = identity(map.nOldCells());
        map.distributeCellData(cellMap);

        // faceMap
        if (map.faceMap().hasAnyFlip())
        {
            // Offset by 1
            faceMap = identity(map.nOldFaces(), 1);

            map.faceMap().mapDistributeBase::distribute
            (
                UPstream::commsTypes::nonBlocking,
                faceMap,
                flipLabelOp()   // Apply flips
            );
        }
        else
        {
            faceMap = identity(map.nOldFaces());
            map.faceMap().distribute(faceMap);
        }

        pointMap = identity(map.nOldPoints());
        map.distributePointData(pointMap);

        patchMap = identity(map.oldPatchSizes().size());
        map.patchMap().mapDistributeBase::distribute
        (
            UPstream::commsTypes::nonBlocking,
            label(-1),  // nullValue for new patches...
            patchMap,
            flipOp()    // negate op
        );
    }
    else
    {
        // Reconstruct
        // - reverse map:  [undecomposed] <- [decomposed]

        cellMap = identity(mesh.nCells());
        map.cellMap().reverseDistribute(map.nOldCells(), cellMap);

        // faceMap
        if (map.faceMap().hasAnyFlip())
        {
            // Offset by 1
            faceMap = identity(mesh.nFaces(), 1);

            map.faceMap().mapDistributeBase::reverseDistribute
            (
                UPstream::commsTypes::nonBlocking,
                map.nOldFaces(),
                faceMap,
                flipLabelOp()   // Apply flips
            );
        }
        else
        {
            faceMap = identity(mesh.nFaces());
            map.faceMap().reverseDistribute(map.nOldFaces(), faceMap);
        }

        pointMap = identity(mesh.nPoints());
        map.pointMap().reverseDistribute(map.nOldPoints(), pointMap);

        patchMap = identity(mesh.boundaryMesh().size());
        map.patchMap().mapDistributeBase::reverseDistribute
        (
            UPstream::commsTypes::nonBlocking,
            map.oldPatchSizes().size(),
            label(-1),  // nullValue for unmapped patches...
            patchMap
        );
    }


    // Switch to using the correct
    // - fileHandler
    // - instance
    // to write to the original mesh/time in the original format. Clunky!
    // Bypass regIOobject writing to avoid taking over the current time
    // as instance so instead of e.g. 'celllMap.write()' directly call
    // the chosen file-handler.

    if (!writeInstance.empty())
    {
        cellMap.instance() = writeInstance;
        faceMap.instance() = writeInstance;
        pointMap.instance() = writeInstance;
        patchMap.instance() = writeInstance;
    }

    const auto& tm = cellMap.time();
    const IOstreamOption opt(tm.writeStreamOption());
    {
        auto handler = writeHandler.shallowClone();
        handler = fileOperation::fileHandler(handler);

        const bool cellOk = fileHandler().writeObject(cellMap, opt, true);
        const bool faceOk = fileHandler().writeObject(faceMap, opt, true);
        const bool pointOk = fileHandler().writeObject(pointMap, opt, true);
        const bool patchOk = fileHandler().writeObject(patchMap, opt, true);

        (void)fileOperation::fileHandler(handler);

        if (!cellOk || !faceOk || !pointOk || !patchOk)
        {
            WarningInFunction
                << "Failed to write some of "
                << cellMap.objectRelPath() << ", "
                << faceMap.objectRelPath() << ", "
                << pointMap.objectRelPath() << ", "
                << patchMap.objectRelPath() << endl;
        }
    }
}


// * * * * * * * * * * * * * * * Housekeeping  * * * * * * * * * * * * * * * //

Foam::autoPtr<Foam::mapDistributePolyMesh>
Foam::fvMeshTools::readProcAddressing
(
    const fvMesh& procMesh,
    const autoPtr<fvMesh>& baseMeshPtr
)
{
    return readProcAddressing(procMesh, baseMeshPtr.get());
}


// ************************************************************************* //
