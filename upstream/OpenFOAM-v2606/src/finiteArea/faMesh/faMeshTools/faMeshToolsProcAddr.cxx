/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2022-2026 OpenCFD Ltd.
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

#include "faMeshTools.H"
#include "BitOps.H"
#include "fileOperation.H"
#include "areaFields.H"
#include "edgeFields.H"
#include "IOmapDistributePolyMesh.H"

// * * * * * * * * * * * * * * * Local Functions * * * * * * * * * * * * * * //

namespace Foam
{

// Create a reconstruct map.

static mapDistributePolyMesh createReconstructMap
(
    const faMesh& mesh,
    const faMesh* baseMeshPtr,
    const labelUList& faceProcAddr,
    const labelUList& edgeProcAddr,
    const labelUList& pointProcAddr,
    const labelUList& boundaryProcAddr
)
{
    const label nOldPoints = mesh.nPoints();
    const label nOldFaces = mesh.nFaces();
    const label nOldEdges = mesh.nEdges();

    const label numProc = UPstream::nProcs();
    const label myProci = UPstream::myProcNo();

    #if 0
    Perr<< "old sizes"
        << " points:" << nOldPoints
        << " faces:" << nOldFaces
        << " edges:" << nOldEdges << nl;
    #endif

    const faBoundaryMesh& oldBndMesh = mesh.boundary();
    labelList oldPatchStarts(oldBndMesh.patchStarts());

    // Patches: purge -1 entries
    labelList patchProcAddr
    (
        IndirectList<label>::subset_if
        (
            boundaryProcAddr,
            labelRange::ge0()
        )
    );


    labelListList faceSubMap(numProc);
    faceSubMap[UPstream::masterNo()] = identity(nOldFaces);

    labelListList edgeSubMap(numProc);
    edgeSubMap[UPstream::masterNo()] = identity(nOldEdges);

    labelListList pointSubMap(numProc);
    pointSubMap[UPstream::masterNo()] = identity(nOldPoints);

    labelListList patchSubMap(numProc);
    patchSubMap[UPstream::masterNo()] = patchProcAddr;

    bool edgeMapHasFlip = true;

    // A '0' value : only occurs without encoding.
    // Check for 0 or -ve values (faster) and then decide
    if
    (
        auto i = ListOps::find_if(edgeProcAddr, labelRange::le0());
        (i >= 0 && !edgeProcAddr[i])
    )
    {
        edgeMapHasFlip = false;
    }
    UPstream::reduceAnd(edgeMapHasFlip);

    // Gather addressing on the master
    labelListList faceAddressing(numProc);
    faceAddressing[myProci] = faceProcAddr;
    Pstream::gatherList(faceAddressing);

    labelListList edgeAddressing(numProc);
    edgeAddressing[myProci] = edgeProcAddr;
    Pstream::gatherList(edgeAddressing);

    labelListList pointAddressing(numProc);
    pointAddressing[myProci] = pointProcAddr;
    Pstream::gatherList(pointAddressing);

    labelListList patchAddressing(numProc);
    patchAddressing[myProci] = patchProcAddr;
    Pstream::gatherList(patchAddressing);


    // NB: can only have a reconstruct on master!
    if (UPstream::master() && baseMeshPtr && baseMeshPtr->nFaces())
    {
        const faMesh& baseMesh = *baseMeshPtr;

        const label nNewPoints = baseMesh.nPoints();
        const label nNewFaces = baseMesh.nFaces();
        const label nNewEdges = baseMesh.nEdges();
        const label nNewPatches = baseMesh.boundary().size();

        #if 0
        Perr<< "new sizes"
            << " points:" << nNewPoints
            << " faces:" << nNewFaces
            << " edges:" << nNewEdges
            << " (flip:" << edgeMapHasFlip << ')' << nl;
        #endif

        mapDistribute faFaceMap
        (
            nNewFaces,
            std::move(faceSubMap),
            std::move(faceAddressing)
        );

        mapDistribute faEdgeMap
        (
            nNewEdges,
            std::move(edgeSubMap),
            std::move(edgeAddressing),
            false,  // subHasFlip
            edgeMapHasFlip  // constructHasFlip
        );

        mapDistribute faPointMap
        (
            nNewPoints,
            std::move(pointSubMap),
            std::move(pointAddressing)
        );

        mapDistribute faPatchMap
        (
            nNewPatches,
            std::move(patchSubMap),
            std::move(patchAddressing)
        );

        return mapDistributePolyMesh
        (
            // Mesh before changes
            nOldPoints,
            nOldEdges,          // area: nOldEdges (volume: nOldFaces)
            nOldFaces,          // area: nOldFaces (volume: nOldCells)

            std::move(oldPatchStarts),
            labelList(),        // oldPatchNMeshPoints [unused]

            mapDistribute(std::move(faPointMap)),
            mapDistribute(std::move(faEdgeMap)), // edgeMap (volume: faceMap)
            mapDistribute(std::move(faFaceMap)), // faceMap (volume: cellMap)
            mapDistribute(std::move(faPatchMap))
        );
    }
    else
    {
        mapDistribute faFaceMap
        (
            0,  // nNewFaces
            std::move(faceSubMap),
            labelListList(numProc)      // constructMap
        );

        mapDistribute faEdgeMap
        (
            0,  // nNewEdges
            std::move(edgeSubMap),
            labelListList(numProc),     // constructMap
            false,  // subHasFlip
            edgeMapHasFlip  // constructHasFlip
        );

        mapDistribute faPointMap
        (
            0,  // nNewPoints
            std::move(pointSubMap),
            labelListList(numProc)      // constructMap
        );

        mapDistribute faPatchMap
        (
            0,  // nNewPatches
            std::move(patchSubMap),
            labelListList(numProc)      // constructMap
        );

        return mapDistributePolyMesh
        (
            // Mesh before changes
            nOldPoints,
            nOldEdges,          // area: nOldEdges (volume: nOldFaces)
            nOldFaces,          // area: nOldFaces (volume: nOldCells)

            std::move(oldPatchStarts),
            labelList(),        // oldPatchNMeshPoints [unused]

            mapDistribute(std::move(faPointMap)),
            mapDistribute(std::move(faEdgeMap)), // edgeMap (volume: faceMap)
            mapDistribute(std::move(faFaceMap)), // faceMap (volume: cellMap)
            mapDistribute(std::move(faPatchMap))
        );
    }
}

} // End namespace Foam


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

Foam::mapDistributePolyMesh
Foam::faMeshTools::readProcAddressing
(
    const faMesh& mesh,
    const faMesh* baseMeshPtr
)
{
    // Processor-local reading
    IOobject ioAddr
    (
        "procAddressing",
        mesh.facesInstance(),
        faMesh::meshSubDir,
        mesh.thisDb(),
        IOobjectOption::READ_IF_PRESENT,
        IOobjectOption::NO_WRITE,
        IOobjectOption::NO_REGISTER
    );

    //if (ioAddr.typeHeaderOk<labelIOList>(true))
    //{
    //    Pout<< "Reading addressing from " << io.name() << " at "
    //        << mesh.facesInstance() << nl << endl;
    //    mapDistributePolyMesh distMap = IOmapDistributePolyMesh(ioAddr);
    //    return distMap;
    //}
    //else

    {
        Info<< "Reading (face|edge|point|boundary)ProcAddressing from "
            << mesh.facesInstance().c_str() << '/'
            << mesh.meshDir().c_str() << nl << endl;

        ioAddr.resetHeader("faceProcAddressing");
        labelIOList faceProcAddressing(ioAddr);

        ioAddr.resetHeader("edgeProcAddressing");
        labelIOList edgeProcAddressing(ioAddr);

        ioAddr.resetHeader("pointProcAddressing");
        labelIOList pointProcAddressing(ioAddr);

        ioAddr.resetHeader("boundaryProcAddressing");
        labelIOList boundaryProcAddressing(ioAddr);

        if
        (
            mesh.nFaces() != faceProcAddressing.size()
         || mesh.nEdges() != edgeProcAddressing.size()
         || mesh.nPoints() != pointProcAddressing.size()
         || mesh.boundary().size() != boundaryProcAddressing.size()
        )
        {
            FatalErrorInFunction
                << "Read addressing inconsistent with mesh sizes" << nl
                << "faces:" << mesh.nFaces()
                << " addressing:" << faceProcAddressing.objectRelPath()
                << " size:" << faceProcAddressing.size() << nl
                << "edges:" << mesh.nEdges()
                << " addressing:" << edgeProcAddressing.objectRelPath()
                << " size:" << edgeProcAddressing.size() << nl
                << "points:" << mesh.nPoints()
                << " addressing:" << pointProcAddressing.objectRelPath()
                << " size:" << pointProcAddressing.size()
                << "patches:" << mesh.boundary().size()
                << " addressing:" << boundaryProcAddressing.objectRelPath()
                << " size:" << boundaryProcAddressing.size()
                << exit(FatalError);
        }

        return createReconstructMap
        (
            mesh,
            baseMeshPtr,
            faceProcAddressing,
            edgeProcAddressing,
            pointProcAddressing,
            boundaryProcAddressing
        );
    }
}


void Foam::faMeshTools::writeProcAddressing
(
    const faMesh& mesh,
    const mapDistributePolyMesh& map,
    const bool decompose,
    const refPtr<fileOperation>& writeHandler,
    const faMesh* procMesh
)
{
    Info<< "Writing ("
        << (decompose ? "decompose" : "reconstruct")
        << ") procAddressing files to "
        << mesh.facesInstance().c_str() << '/'
        << mesh.meshDir().c_str() << endl;

    IOobject ioAddr
    (
        "proc-addressing",
        mesh.facesInstance(),
        faMesh::meshSubDir,
        (procMesh && !decompose ? procMesh->thisDb() : mesh.thisDb()),
        IOobjectOption::NO_READ,
        IOobjectOption::NO_WRITE,
        IOobjectOption::NO_REGISTER
    );


    // faceProcAddressing (faMesh)
    ioAddr.rename("faceProcAddressing");
    labelIOList faceMap(ioAddr);

    // edgeProcAddressing (faMesh)
    ioAddr.rename("edgeProcAddressing");
    labelIOList edgeMap(ioAddr);

    // pointProcAddressing (faMesh)
    ioAddr.rename("pointProcAddressing");
    labelIOList pointMap(ioAddr);

    // boundaryProcAddressing (faMesh)
    ioAddr.rename("boundaryProcAddressing");
    labelIOList patchMap(ioAddr);

    if (decompose)
    {
        // Decompose
        // - forward map:  [undecomposed] -> [decomposed]

        // area:faces (volume:cells)
        faceMap = identity(map.nOldCells());
        map.cellMap().distribute(faceMap);

        // area:edges (volume:faces)
        {
            const auto srcLen = map.nOldFaces();

            const mapDistribute& distMap = map.faceMap();

            if (distMap.hasAnyFlip())
            {
                // Offset by 1
                edgeMap = Foam::identity(srcLen, 1);

                distMap.mapDistributeBase::distribute
                (
                    UPstream::commsTypes::nonBlocking,
                    edgeMap,
                    flipLabelOp()   // Apply flips
                );
            }
            else
            {
                // No edge encoding
                edgeMap = Foam::identity(srcLen);

                distMap.distribute(edgeMap);
            }
        }

        pointMap = identity(map.nOldPoints());
        map.distributePointData(pointMap);

        patchMap = identity(map.patchMap().constructSize());
        map.patchMap().mapDistributeBase::distribute
        (
            UPstream::commsTypes::nonBlocking,
            label(-1),  // nullValue for new patches...
            patchMap,
            flipOp()    // negate op
        );
    }
    else    // reconstruct
    {
        // Reconstruct
        // - reverse map:  [undecomposed] <- [decomposed]

        // area:faces (volume:cells)
        faceMap = identity(mesh.nFaces());
        map.cellMap().reverseDistribute(map.nOldCells(), faceMap);

        // area:edges (volume:faces)
        {
            const auto oldLen = map.nOldFaces();
            const auto tgtLen = mesh.patch().nEdges();

            const mapDistribute& distMap = map.faceMap();

            if (distMap.hasAnyFlip())
            {
                // Offset by 1
                edgeMap = Foam::identity(tgtLen, 1);

                distMap.mapDistributeBase::reverseDistribute
                (
                    UPstream::commsTypes::nonBlocking,
                    oldLen,
                    edgeMap,
                    flipLabelOp()   // Apply flips
                );
            }
            else
            {
                edgeMap = Foam::identity(tgtLen);
                distMap.reverseDistribute(oldLen, edgeMap);
            }
        }

        pointMap = identity(mesh.nPoints());
        map.pointMap().reverseDistribute(map.nOldPoints(), pointMap);

        patchMap = identity(mesh.boundary().size());
        map.patchMap().mapDistributeBase::reverseDistribute
        (
            UPstream::commsTypes::nonBlocking,
            map.oldPatchSizes().size(),
            label(-1),  // nullValue for unmapped patches...
            patchMap
        );
    }


    auto handler = writeHandler.shallowClone();
    handler = fileOperation::fileHandler(handler);

    // If we want procAddressing, need to manually write it ourselves
    // since it was not registered anywhere

    IOmapDistributePolyMeshRef procAddrMap
    (
        IOobject
        (
            "procAddressing",
            mesh.facesInstance(),
            faMesh::meshSubDir,
            mesh.thisDb(),
            IOobject::NO_READ,
            IOobject::NO_WRITE,
            IOobject::NO_REGISTER
        ),
        map
    );


    if (decompose)
    {
        // Write into proc directories
        procAddrMap.write();
    }
    else
    {
        // Reconstruct: "procAddressing" only meaningful for rank 0
        // and written into base (serial) location (if at all).

        if (UPstream::master())
        {
            const auto oldParRun = UPstream::parRun(false);
            procAddrMap.write();
            UPstream::parRun(oldParRun);
        }
    }


    const bool faceOk = faceMap.write();
    const bool edgeOk = edgeMap.write();
    const bool pointOk = pointMap.write();
    const bool patchOk = patchMap.write();

    // Restore the handler
    (void)fileOperation::fileHandler(handler);

    if (!edgeOk || !faceOk || !pointOk || !patchOk)
    {
        WarningInFunction
            << "Failed to write some of "
            << faceMap.objectRelPath() << ", "
            << edgeMap.objectRelPath() << ", "
            << pointMap.objectRelPath() << ", "
            << patchMap.objectRelPath() << endl;
    }
}


// * * * * * * * * * * * * * * * Housekeeping  * * * * * * * * * * * * * * * //

Foam::mapDistributePolyMesh
Foam::faMeshTools::readProcAddressing
(
    const faMesh& procMesh,
    const autoPtr<faMesh>& baseMeshPtr
)
{
    return readProcAddressing(procMesh, baseMeshPtr.get());
}


// ************************************************************************* //
