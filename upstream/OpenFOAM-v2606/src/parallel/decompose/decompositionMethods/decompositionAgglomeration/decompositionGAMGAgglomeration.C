/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2023 M. Janssens
    Copyright (C) 2025 OpenCFD Ltd.
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

#include "decompositionGAMGAgglomeration.H"
#include "fvMesh.H"
#include "addToRunTimeSelectionTable.H"
#include "decompositionMethod.H"
// #include "OBJstream.H"
#include "bandCompression.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
    defineTypeNameAndDebug(decompositionGAMGAgglomeration, 0);

    addToRunTimeSelectionTable
    (
        GAMGAgglomeration,
        decompositionGAMGAgglomeration,
        lduMesh
    );

    addToRunTimeSelectionTable
    (
        GAMGAgglomeration,
        decompositionGAMGAgglomeration,
        geometry
    );
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::decompositionGAMGAgglomeration::decompositionGAMGAgglomeration
(
    const lduMesh& mesh,
    const dictionary& controlDict
)
:
    GAMGAgglomeration(mesh, controlDict),
    doRenumber_(controlDict.getOrDefault<bool>("renumber", false)),
    forceConnected_(controlDict.getOrDefault<bool>("forceConnected", true)),
    decomposerPtr_
    (
        decompositionMethod::New
        (
            controlDict.optionalSubDict(type() + "Coeffs")
        )
    ),  // regionName
    clusterSize_(decomposerPtr_().nDomains()),
    hasWarned_(false)
{
    hasWarned_ = false;
    agglomerate
    (
        nCellsInCoarsestLevel_,
        0,          //  starting mesh,
        scalarField(mesh.lduAddr().upperAddr().size(), scalar(1.0)),
        true
    );
}


Foam::decompositionGAMGAgglomeration::decompositionGAMGAgglomeration
(
    const lduMesh& mesh,
    const scalarField& cellVolumes,
    const vectorField& faceAreas,
    const dictionary& controlDict
)
:
    GAMGAgglomeration(mesh, controlDict),
    doRenumber_(controlDict.getOrDefault<bool>("renumber", false)),
    forceConnected_(controlDict.getOrDefault<bool>("forceConnected", true)),
    decomposerPtr_
    (
        decompositionMethod::New
        (
            controlDict.optionalSubDict(type() + "Coeffs")
        )
    ),  // regionName
    clusterSize_(decomposerPtr_().nDomains()),
    hasWarned_(false)
{
    agglomerate
    (
        nCellsInCoarsestLevel_,
        0,          // starting mesh,
        mag(faceAreas),
        true
    );
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

bool Foam::decompositionGAMGAgglomeration::checkRestriction
(
    labelList& newRestrict,
    label& nNewCoarse,
    const lduAddressing& fineAddressing,
    const labelUList& restriction,
    const label nCoarse
)
{
    // A decomposition method does not guarantee a single connected
    // region - it is an approximate minimisation of some cost function -
    // so split additionally the regions into locally connected bits. Usually
    // there is one large bit and a few small bits.
    // Done by walking.

    if (fineAddressing.size() != restriction.size())
    {
        FatalErrorInFunction
            << "nCells:" << fineAddressing.size()
            << " agglom:" << restriction.size()
            << abort(FatalError);
    }

    // Seed (master) for every region
    labelList master(identity(fineAddressing.size()));

    // Now loop and transport master through region. Could maintain a front
    // but that requires cell addressing. Instead just keep on looping
    // in face order and hope the fineAddressing isn't a single stack of
    // cells ...
    const labelUList& lower = fineAddressing.lowerAddr();
    const labelUList& upper = fineAddressing.upperAddr();

    while (true)
    {
        label nChanged = 0;

        forAll(lower, facei)
        {
            const label own = lower[facei];
            const label nei = upper[facei];

            if (restriction[own] == restriction[nei])
            {
                // coarse-mesh-internal face

                if (master[own] < master[nei])
                {
                    master[nei] = master[own];
                    nChanged++;
                }
                else if (master[own] > master[nei])
                {
                    master[own] = master[nei];
                    nChanged++;
                }
            }
        }

        if (nChanged == 0)
        {
            break;
        }
    }


    // Now master[celli] will be the lowest numbered cell for each disconnected
    // region.
    labelList oldToNewRegion(restriction.size(), -1);
    labelList masterToRegion(restriction.size(), -1);
    nNewCoarse = 0;
    forAll(restriction, celli)
    {
        if (master[celli] == celli) // 'master' of the region
        {
            // Allocate new coarse
            masterToRegion[celli] = nNewCoarse++;
            const label oldRegion = restriction[celli];
            oldToNewRegion[oldRegion] = masterToRegion[celli];
        }
    }
    if (nNewCoarse != nCoarse)
    {
        newRestrict = UIndirectList<label>(masterToRegion, master);
        return false;
    }

    return true;
}


Foam::bitSet Foam::decompositionGAMGAgglomeration::blockedFaces
(
    const lduAddressing& addr,
    const labelUList& region        // region per cell
)
{
    const labelUList& nbr = addr.upperAddr();
    const labelUList& own = addr.lowerAddr();

    bitSet isBlockedFace(nbr.size());
    forAll(nbr, facei)
    {
        isBlockedFace[facei] = (region[own[facei]] != region[nbr[facei]]);
    }
    return isBlockedFace;
}


void Foam::decompositionGAMGAgglomeration::localCellCells
(
    const lduAddressing& addr,
    const bitSet& isBlockedFace,
    CompactListList<label>& cellCells
)
{
    // Since have only internal addressing cannot get global cell connectivity?

    const labelUList& nbr = addr.upperAddr();
    const labelUList& own = addr.lowerAddr();

    labelList nNbrs(addr.size(), Zero);
    forAll(nbr, facei)
    {
        if (!isBlockedFace[facei])
        {
            nNbrs[nbr[facei]]++;
            nNbrs[own[facei]]++;
        }
    }

    // Calculate&store offsets
    cellCells.resize_nocopy(nNbrs);

    auto& values = cellCells.values();
    auto& offsets = cellCells.offsets();

    // Fill in neighbours
    nNbrs = Zero;
    forAll(nbr, facei)
    {
        if (!isBlockedFace[facei])
        {
            const label n = nbr[facei];
            const label o = own[facei];
            values[offsets[n]+(nNbrs[n]++)] = o;
            values[offsets[o]+(nNbrs[o]++)] = n;
        }
    }
}


Foam::labelList Foam::decompositionGAMGAgglomeration::localCellCells
(
    const lduAddressing& addr,
    const labelList& regions,   // marker per cell
    const label regioni,        // which marker to keep
    CompactListList<label>& cellCells
)
{
    const labelUList& nbr = addr.upperAddr();
    const labelUList& own = addr.lowerAddr();

    if (regions.size() != addr.size())
    {
        FatalErrorInFunction<< "Wrong size" << exit(FatalError);
    }

    // Compact cell numbering for region cells
    labelList oldToNew(regions.size(), -1);
    label nCells = 0;
    forAll(regions, i)
    {
        if (regions[i] == regioni)
        {
            oldToNew[i] = nCells++;
        }
    }

    labelList nNbrs(nCells, Zero);
    forAll(nbr, facei)
    {
        const label n = oldToNew[nbr[facei]];
        const label o = oldToNew[own[facei]];
        if (n != -1 && o != -1)
        {
            nNbrs[n]++;
            nNbrs[o]++;
        }
    }

    // Calculate&store offsets
    cellCells.resize_nocopy(nNbrs);

    auto& values = cellCells.values();
    const auto& offsets = cellCells.offsets();

    // Fill in neighbours
    nNbrs = Zero;
    forAll(nbr, facei)
    {
        const label n = oldToNew[nbr[facei]];
        const label o = oldToNew[own[facei]];
        if (n != -1 && o != -1)
        {
            values[offsets[n]+(nNbrs[n]++)] = o;
            values[offsets[o]+(nNbrs[o]++)] = n;
        }
    }
    return oldToNew;
}


void Foam::decompositionGAMGAgglomeration::coarseCellCells
(
    const lduAddressing& addr,
    const labelList& regions,           // coarse cell
    CompactListList<label>& cellCells   // coarse-to-coarse cells
)
{
    const labelUList& nbr = addr.upperAddr();
    const labelUList& own = addr.lowerAddr();

    const label nCoarse = max(regions)+1;

    labelList nNbrs(nCoarse, Zero);
    forAll(nbr, facei)
    {
        const label n = regions[nbr[facei]];
        const label o = regions[own[facei]];
        if (n != o)
        {
            nNbrs[n]++;
            nNbrs[o]++;
        }
    }
    // Calculate&store offsets
    cellCells.resize_nocopy(nNbrs);

    auto& values = cellCells.values();
    const auto& offsets = cellCells.offsets();

    // Fill in neighbours
    nNbrs = Zero;
    forAll(nbr, facei)
    {
        const label n = regions[nbr[facei]];
        const label o = regions[own[facei]];
        if (n != o)
        {
            values[offsets[n]+(nNbrs[n]++)] = o;
            values[offsets[o]+(nNbrs[o]++)] = n;
        }
    }
}


Foam::tmp<Foam::labelField> Foam::decompositionGAMGAgglomeration::agglomerate
(
    const bool partByPart,
    const label nCoarseCells,

    // Current mesh
    const lduMesh& mesh,
    const pointField& cellCentres,
    const scalarField& cellWeights,
    const scalarField& faceWeights,

    // Agglomeration of current mesh
    const labelUList& region
) const
{
    const lduAddressing& addr = mesh.lduAddr();

    // Return addressing from current mesh (addr) to coarse mesh
    decompositionMethod& decomposer = decomposerPtr_();

    const label nRegions = max(region) + 1;

    auto tagglom = tmp<labelField>::New(addr.size(), -1);
    auto& agglom = tagglom.ref();

    if (partByPart)
    {
        // Most decomposition methods cannot handle multiple graphs so
        // split the original mesh according to the marker and do each
        // individually.

        for (label regioni = 0; regioni < nRegions; regioni++)
        {
            CompactListList<label> cellCells;
            const labelList oldToNew
            (
                localCellCells
                (
                    addr,
                    region,         // marker per cell
                    regioni,        // which marker to keep
                    cellCells
                )
            );

            auto cc(avg(cellCentres, cellCells.size(), oldToNew));
            decomposer.nDomains(nCoarseCells);
            const bool oldParRun = UPstream::parRun(false);
            const label nRegionCells = cellCells.size();
            labelList cellToProc;
            if (nRegionCells >= 2*nCoarseCells)
            {
                cellToProc = decomposer.decompose(cellCells, cc);
            }
            else
            {
                // Small cluster. Can
                //  - call decomposition method anyway (but e.g. scotch will
                //    fail)
                //  - or assign each to different region
                //  - or all to same region
                static label fallBackCell = 0;
                cellToProc.resize_nocopy(nRegionCells);
                cellToProc = fallBackCell; //identity(nRegionCells)
                fallBackCell = ((fallBackCell +1) % nCoarseCells);
            }
            UPstream::parRun(oldParRun);

            // Combine
            forAll(oldToNew, celli)
            {
                const label compactCelli = oldToNew[celli];
                if (compactCelli != -1)
                {
                    agglom[celli] =
                        regioni*nCoarseCells + cellToProc[compactCelli];
                }
            }
        }
    }
    else
    {
        // Single step
        const bitSet isBlockedFace(blockedFaces(addr, region));

        CompactListList<label> cellCells;
        localCellCells(addr, isBlockedFace, cellCells);

        decomposer.nDomains(nCoarseCells);
        const bool oldParRun = UPstream::parRun(false);
        agglom = decomposer.decompose
        (
            cellCells,
            cellCentres     // pointField(cellCells.size(), Zero)
        );
        UPstream::parRun(oldParRun);
    }

    if (forceConnected_)
    {
        label nAgglom = max(agglom)+1;

        label nNewAgglom;
        labelList newAgglom;
        const bool ok = checkRestriction
        (
            newAgglom,
            nNewAgglom,
            addr,
            agglom,
            nAgglom
        );

        if (!ok)
        {
            DebugInfo
                << "Extending agglomeration from:" << nAgglom << " to:"
                << nNewAgglom << endl;
            agglom = std::move(newAgglom);
        }
        reduce(nAgglom, sumOp<label>(), UPstream::msgType(), mesh.comm());
        reduce(nNewAgglom, sumOp<label>(), UPstream::msgType(), mesh.comm());

        if (nNewAgglom > nAgglom)
        {
            if (!hasWarned_)
            {
                hasWarned_ = true;
                WarningInFunction
                    << "When splitting coarse mesh into " << nAgglom
                    << " cells created " << nNewAgglom
                    << " disconnected domains." << nl
                    << "    This might give numerical problems. Either choose"
                    << " a decomposition method that guarantees a single domain"
                    << " or keep the result of the decomposition by setting"
                    << " 'forceConnected' to false" << nl
                    << "    Disabling further warnings."
                    << endl;
            }
        }
    }

    // Renumbering - does not seem to help
    if (doRenumber_)
    {
        // Get addressing between the agglomerated cells
        CompactListList<label> cellCells;
        coarseCellCells(addr, agglom, cellCells);
        const labelList newToOld(meshTools::bandCompression(cellCells));
        const labelList oldToNew(invert(cellCells.size(), newToOld));
        forAll(agglom, celli)
        {
            agglom[celli] = oldToNew[agglom[celli]];
        }
    }

    return tagglom;
}


void Foam::decompositionGAMGAgglomeration::agglomerate
(
    const label nCellsInCoarsestLevel,
    const label startLevel,
    // const pointField& startCellCentres,
    // const scalarField& startCellWeights,
    const scalarField& startFaceWeights,
    const bool doProcessorAgglomerate
)
{
    // Straight copy of pairGAMGAgglomeration::agglomerate without the
    // mergeLevels

    if (nCells_.size() < maxLevels_)
    {
        // See compactLevels. Make space if not enough
        nCells_.resize(maxLevels_);
        restrictAddressing_.resize(maxLevels_);
        nFaces_.resize(maxLevels_);
        faceRestrictAddressing_.resize(maxLevels_);
        faceFlipMap_.resize(maxLevels_);
        nPatchFaces_.resize(maxLevels_);
        patchFaceRestrictAddressing_.resize(maxLevels_);
        meshLevels_.resize(maxLevels_);
        // Have procCommunicator_ always, even if not procAgglomerating.
        // Use value -1 to indicate nothing is proc-agglomerated
        procCommunicator_.resize(maxLevels_ + 1, -1);
        if (processorAgglomerate())
        {
            procAgglomMap_.resize(maxLevels_);
            agglomProcIDs_.resize(maxLevels_);
            procCommunicator_.resize(maxLevels_);
            procCellOffsets_.resize(maxLevels_);
            procFaceMap_.resize(maxLevels_);
            procBoundaryMap_.resize(maxLevels_);
            procBoundaryFaceMap_.resize(maxLevels_);
        }
    }

   const auto& startMesh = meshLevel(startLevel);
   const lduAddressing& startAddr = startMesh.lduAddr();


    // Start geometric agglomeration from the given faceWeights
    scalarField faceWeights(startFaceWeights);

    // TBD. Fudge - cell centres not yet passed through
    pointField cellCentres;
    scalarField cellWeights;
    const auto* startMeshPtr = isA<primitiveMesh>(startMesh);
    if (startMeshPtr)
    {
        cellCentres = startMeshPtr->cellCentres();
        cellWeights = startMeshPtr->cellVolumes();
    }

    // Note: per level (starting from coarsest), per start mesh cell the
    //       agglomeration (= region)
    DynamicList<labelList> startAgglomeration(maxLevels_);
    {
        // Fine mesh is a single region so can be done one-shot? But not when
        // running parallel - decomposition methods do not guarantee this.
        // Pout<< "Doing initial level nCells:" << startAddr.size() << endl;

        // Determine reasonable nCellsInCoarsestLevel to have better
        // first level
        label nCoarsestCells = startAddr.size();
        while (nCoarsestCells/clusterSize_  >= nCellsInCoarsestLevel)
        {
            nCoarsestCells /= clusterSize_;
        }
        DebugPout
            << "Doing coarsest:" << nCoarsestCells
            << " instead of:" << startAddr.size()
            << endl;
        const labelList fineToCoarsest
        (
            agglomerate
            (
                UPstream::parRun(),                 // partByPart,
                nCoarsestCells,                     //nCellsInCoarsestLevel,

                // Current mesh
                startMesh,
                cellCentres,
                cellWeights,
                faceWeights,

                labelList(startAddr.size(), 0)
            )
        );
        // {
        //     auto cc(avg(cellCentres, nCellsInCoarsestLevel, fineToCoarsest));
        //     const label level = startAgglomeration.size();
        //     OBJstream os("cellCentres_" + Foam::name(level) + ".obj");
        //     forAll(fineToCoarsest, celli)
        //     {
        //         const label coarsei = fineToCoarsest[celli];
        //         os.write(linePointRef(cellCentres[celli], cc()[coarsei]));
        //     }
        // }
        // From fine to coarsest mesh
        startAgglomeration.append(std::move(fineToCoarsest));
        // Pout<< "Done initial level nCells:" << startAddr.size() << endl;
    }
    while (startAgglomeration.size() < maxLevels_)
    {
        const label nFineCells = max(startAgglomeration.last())+1;
        // Pout<< "Doing level:" << startAgglomeration.size()
        //      << " nCells:" << nFineCells << endl;

        if
        (
            returnReduceAnd
            (
                nFineCells >= startAddr.size()/(2*clusterSize_),
                startMesh.comm()
            )
        )
        {
            break;
        }

        // Do next levels
        // - construct the less-coarse to coarse agglomeration
        const labelList fineToCoarse
        (
            agglomerate
            (
                true,                   // multi-pass
                clusterSize_,           //nCellsInCoarsestLevel,

                // Current mesh
                startMesh,
                cellCentres,
                cellWeights,
                faceWeights,

                startAgglomeration.last()
            )
        );
        // {
        //     auto cc(avg(cellCentres, max(fineToCoarse)+1, fineToCoarse));
        //     const label level = startAgglomeration.size();
        //     OBJstream os("cellCentres_" + Foam::name(level) + ".obj");
        //     forAll(fineToCoarse, celli)
        //     {
        //         const label coarsei = fineToCoarse[celli];
        //         os.write(linePointRef(cellCentres[celli], cc()[coarsei]));
        //     }
        // }

        startAgglomeration.append(fineToCoarse);
        // Pout<< "Done level:" << startAgglomeration.size()
        //     << " nCells:" << max(startAgglomeration.last())+1 << endl;
    }
    reverse(startAgglomeration);


    // Agglomerate until the required number of cells in the coarsest level
    // is reached
    label nCreatedLevels = startLevel;

    forAll(startAgglomeration, i)
    {
        if (!hasMeshLevel(nCreatedLevels))
        {
            FatalErrorInFunction<< "No mesh at nCreatedLevels:"
                << nCreatedLevels
                << exit(FatalError);
        }

        const auto& fineMesh = meshLevel(nCreatedLevels);

        // Determine mapping from fine to coarse. This normally calls
        // a single-level agglomeration but we use the previously calculated
        // splitting.
        const auto& startToCoarse = startAgglomeration[i];
        const label nCoarseCells = max(startToCoarse)+1;
        auto tfinalAgglomPtr = tmp<labelField>::New(fineMesh.lduAddr().size());
        auto& finalAgglom = tfinalAgglomPtr.ref();
        if (i == 0)
        {
            finalAgglom = startToCoarse;
        }
        else
        {
            // Renumber since agglomeration is from the starting mesh, not the
            // previous mesh
            const auto& startToPrev = startAgglomeration[i-1];
            UIndirectList<label>(finalAgglom, startToPrev) = startToCoarse;
        }
        nCells_[nCreatedLevels] = nCoarseCells;
        restrictAddressing_.set(nCreatedLevels, tfinalAgglomPtr);

        // Create coarse mesh
        agglomerateLduAddressing(nCreatedLevels);

        // // Agglomerate the faceWeights field for the next level
        // {
        //     scalarField aggFaceWeights
        //     (
        //         meshLevels_[nCreatedLevels].upperAddr().size(),
        //         0.0
        //     );

        //     restrictFaceField
        //     (
        //         aggFaceWeights,
        //         faceWeights,
        //         nCreatedLevels
        //     );

        //     faceWeights = std::move(aggFaceWeights);
        // }

        nCreatedLevels++;
    }

    // Shrink the storage of the levels to those created
    compactLevels(nCreatedLevels, doProcessorAgglomerate);
}


// Foam::autoPtr<Foam::lduPrimitiveMesh>
// Foam::decompositionGAMGAgglomeration::createMesh
// (
//     const label comm,
//     const CompactListList<label>& cellCells
// )
// {
//     // CSR connections
//     const labelList& connect = cellCells.values();
//
//     DynamicList<label> lower(connect.size()/2);
//     DynamicList<label> upper(connect.size()/2);
//
//     DynamicList<label> nbrs;
//     forAll(cellCells, celli)
//     {
//         const SubList<label> row(connect, cellCells.range(celli));
//         nbrs.clear();
//         for (const label nbr : row)
//         {
//             if (nbr > celli)
//             {
//                 nbrs.append(nbr);
//             }
//         }
//         sort(nbrs);
//         for (const label nbr : nbrs)
//         {
//             lower.append(celli);
//             upper.append(nbr);
//         }
//     }
//
//     autoPtr<lduPrimitiveMesh> coarseMeshPtr
//     (
//         new lduPrimitiveMesh
//         (
//             cellCells.size(),
//             lower,
//             upper,
//             comm,
//             true
//         )
//     );
//
//     // Debug
//     {
//         const auto& coarseMesh = coarseMeshPtr();
//         Pout<< "CoarseMesh:" << coarseMesh.lduAddr().size() << endl;
//         forAll(coarseMesh.lduAddr().lowerAddr(), facei)
//         {
//             Pout<< "    coarseface:" << facei
//                 << " owner:" << coarseMesh.lduAddr().lowerAddr()[facei]
//                 << " neighbour:"
//                 << coarseMesh.lduAddr().upperAddr()[facei]
//                 << endl;
//         }
//         Pout<< endl;
//     }
//     return coarseMeshPtr;
// }
// Foam::autoPtr<Foam::lduPrimitiveMesh>
// Foam::decompositionGAMGAgglomeration::createMesh
// (
//     const lduMesh& mesh,
//     const labelUList& agglom
// )
// {
//     // Convert mesh and agglomeration into lduPrimitiveMesh
//     if (!agglom.size())
//     {
//         return nullptr;
//     }
//
//     const label nCoarseCells = max(agglom)+1;
//     const auto& lower = mesh.lduAddr().lowerAddr();
//     const auto& upper = mesh.lduAddr().upperAddr();
//
//     labelPairHashSet doneCellCell(lower.size());
//
//     labelList faceMap(lower.size(), -1);
//     labelList coarseLower(lower.size());
//     labelList coarseUpper(lower.size());
//     label nCoarseFaces = 0;
//     forAll(lower, facei)
//     {
//         if (faceMap[facei] == -1)
//         {
//             const label l = agglom[lower[facei]];
//             const label u = agglom[upper[facei]];
//
//             if (l < u)
//             {
//                 if (doneCellCell.insert(labelPair(l, u)))
//                 {
//                     const label coarseFacei = nCoarseFaces++;
//                     faceMap[facei] = coarseFacei;
//                     coarseLower[coarseFacei] = l;
//                     coarseUpper[coarseFacei] = u;
//                 }
//             }
//             else if (u < l)
//             {
//                 if (doneCellCell.insert(labelPair(u, l)))
//                 {
//                     const label coarseFacei = nCoarseFaces++;
//                     faceMap[facei] = coarseFacei;
//                     coarseLower[coarseFacei] = u;
//                     coarseUpper[coarseFacei] = l;
//                 }
//             }
//         }
//     }
//     coarseLower.setSize(nCoarseFaces);
//     coarseUpper.setSize(nCoarseFaces);
//
//     Pout<< "Going from" << nl
//         << "    nCells:" << mesh.lduAddr().size()
//         << " nFaces:" << mesh.lduAddr().lowerAddr().size() << nl
//         << "to" << nl
//         << "    nCells:" << nCoarseCells
//         << " nFaces:" << coarseLower.size() << endl;
//
//     autoPtr<lduPrimitiveMesh> coarseMeshPtr
//     (
//         new lduPrimitiveMesh
//         (
//             nCoarseCells,
//             coarseLower,
//             coarseUpper,
//             mesh.comm(),
//             true
//         )
//     );
//
//     // Debug
//     {
//         const auto& coarseMesh = coarseMeshPtr();
//         Pout<< "CoarseMesh:" << coarseMesh.lduAddr().size() << endl;
//         forAll(coarseMesh.lduAddr().lowerAddr(), facei)
//         {
//             Pout<< "    coarseface:" << facei
//                 << " owner:" << coarseMesh.lduAddr().lowerAddr()[facei]
//                 << " neighbour:"
//                 << coarseMesh.lduAddr().upperAddr()[facei]
//                 << endl;
//         }
//         Pout<< endl;
//     }
//     return coarseMeshPtr;
// }


// ************************************************************************* //
