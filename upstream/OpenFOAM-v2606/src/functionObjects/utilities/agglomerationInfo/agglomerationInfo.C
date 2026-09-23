/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2026 Keysight Technologies
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

#include "agglomerationInfo.H"
#include "addToRunTimeSelectionTable.H"
#include "Pstream.H"
#include "fvMesh.H"
#include "volFields.H"
#include "GAMGSolver.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
namespace functionObjects
{
    defineTypeNameAndDebug(agglomerationInfo, 0);

    addToRunTimeSelectionTable
    (
        functionObject,
        agglomerationInfo,
        dictionary
    );
}
}


// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

const Foam::GAMGAgglomeration&
Foam::functionObjects::agglomerationInfo::agglomeration() const
{
    const auto* agglomPtr = mesh_.cfindObject<GAMGAgglomeration>(agglomName_);

    if (!agglomPtr)
    {
        FatalErrorInFunction
            << "GAMGAgglomeration '" << agglomName_ << "' not found. "
            << "Set cacheAgglomeration true for field " << fieldName_
            << " in fvSolution"
            << exit(FatalError);
    }

    return *agglomPtr;
}


void Foam::functionObjects::agglomerationInfo::writeLevel0()
{
    // Level 0: original fine (un-coarsened) mesh
    // Note: each cell maps to its own globally-unique index
    // Values are just the per-proc cell counts
    const label statsComm = mesh_.comm();
    labelList allNCells0(UPstream::nProcs(statsComm), Zero);
    allNCells0[UPstream::myProcNo(statsComm)] = mesh_.nCells();
    Pstream::gatherList(allNCells0, UPstream::msgType(), statsComm);
    Pstream::broadcast(allNCells0, statsComm);

    label total0 = 0;
    label min0 = labelMax;
    label max0 = 0;
    label nActive0 = 0;
    for (label n : allNCells0)
    {
        if (n >= 0)
        {
            total0 += n;
            min0 = min(min0, n);
            max0 = max(max0, n);
            ++nActive0;
        }
    }
    const scalar avg0 = nActive0 > 0 ? scalar(total0)/nActive0 : 0;

    // Globally-unique cell ID = proc prefix-sum offset + local index.
    label myOffseti0 = 0;
    for (label p = 0; p < UPstream::myProcNo(statsComm); ++p)
    {
        myOffseti0 += allNCells0[p];
    }

    const word writtenName0 = "GAMGAgglom_" + fieldName_ + "_level0_mesh";

    volScalarField fineMesh
    (
        IOobject
        (
            writtenName0,
            mesh_.time().timeName(),
            mesh_,
            IOobject::NO_READ,
            IOobject::NO_WRITE,
            IOobject::NO_REGISTER
        ),
        mesh_,
        dimensionedScalar(dimless, Zero)
    );

    forAll(fineMesh, celli)
    {
        fineMesh[celli] = scalar(myOffseti0 + celli);
    }

    const label nP0 = label(allNCells0.size());
    const word assignedStr0 = Foam::name(nP0) + " proc" + (nP0 != 1 ? "s" : "");

    auto& os = file();

    os  << "0" << tab
        << total0 << tab
        << min0 << tab
        << label(avg0) << tab
        << max0 << tab
        << assignedStr0 << tab
        << writtenName0 << tab
        << nl;

    fineMesh.write();
}


void Foam::functionObjects::agglomerationInfo::setCellsPerLevel
(
    const GAMGAgglomeration& agglom,
    const label leveli,
    const label& procAgglomLeveli,
    labelField& cellsPerLevel
) const
{
    if (leveli == 0) return;

    if (agglom.hasProcMesh(leveli))
    {
        // Processor agglomeration
        // Note: master holds the combined restrict addressing for all
        // processors' cells.  Non-master processors had their level data
        // cleared
        const label comm = agglom.agglomCommunicator(leveli);
        const label nPart = UPstream::nProcs(comm);
        const label myRank = UPstream::myProcNo(comm);

        // Step 1: gather per-proc fine-cell counts within group
        labelList fineSizes(nPart, Zero);
        fineSizes[myRank] = agglom.nCells(leveli - 1);
        Pstream::gatherList(fineSizes, UPstream::msgType(), comm);
        Pstream::broadcast(fineSizes, comm);

        // Compute this proc offset into combined addr
        label myOffseti = 0;
        for (label p = 0; p < myRank; ++p)
        {
            myOffseti += fineSizes[p];
        }

        // Step 2: broadcast combined addr from master
        labelField combinedAddr;

        if (UPstream::myProcNo(comm) == 0) // master in agglomComm
        {
            combinedAddr = agglom.restrictAddressing(leveli);
        }
        Pstream::broadcast(combinedAddr, comm);

        // Local fine coarse index -> group-local combined coarse index
        labelField next(cellsPerLevel.size());
        forAll(next, celli)
        {
            next[celli] = combinedAddr[myOffseti + cellsPerLevel[celli]];
        }
        cellsPerLevel.transfer(next);
    }
    else if (procAgglomLeveli >= 0 && leveli > procAgglomLeveli)
    {
        // Post-proc-agglomeration level
        // These levels were further coarsened on master only after
        // proc agglomeration; restrict addressing is null on
        // non-masters
        const label comm = agglom.agglomCommunicator(procAgglomLeveli);
        labelField addr;
        if (UPstream::myProcNo(comm) == 0)  // master in agglomComm
        {
            addr = agglom.restrictAddressing(leveli);
        }
        Pstream::broadcast(addr, comm);

        for (auto& addri : cellsPerLevel)
        {
            addri = addr[addri];
        }
    }
    else
    {
        // Normal level, no proc agglom crossed yet
        const labelField& addr = agglom.restrictAddressing(leveli);

        for (auto& addri : cellsPerLevel)
        {
            addri = addr[addri];
        }
    }
}


Foam::tmp<Foam::volScalarField>
Foam::functionObjects::agglomerationInfo::initAgglomField
(
    const GAMGAgglomeration& agglom,
    const label leveli,
    const label procAgglomLeveli
) const
{
    // Note:
    // - Coarsest level gets "_coarsest" suffix
    // - Append "_procAgglom" is added if proc-agglom was active
    word levelSuffix = "";

    if (leveli == agglom.size() - 1)
    {
        levelSuffix = "_coarsest";
        if (procAgglomLeveli >= 0)
        {
            levelSuffix += "_procAgglom";
        }
    }

    const word writtenName =
        "GAMGAgglom_" + fieldName_
       + "_level" + Foam::name(leveli + 1)
       + levelSuffix;

    // Actual agglomeration field per level
    return tmp<volScalarField>::New
    (
        IOobject
        (
            writtenName,
            mesh_.time().timeName(),
            mesh_,
            IOobject::NO_READ,
            IOobject::NO_WRITE,
            IOobject::NO_REGISTER
        ),
        mesh_,
        dimensionedScalar(dimless, Zero)
    );
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::functionObjects::agglomerationInfo::agglomerationInfo
(
    const word& name,
    const Time& runTime,
    const dictionary& dict
)
:
    fvMeshFunctionObject(name, runTime, dict),
    writeFile(runTime, name, typeName, dict),
    fieldName_(),
    agglomName_()
{
    read(dict);
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

bool Foam::functionObjects::agglomerationInfo::read(const dictionary& dict)
{
    if (fvMeshFunctionObject::read(dict) && writeFile::read(dict))
    {
        fieldName_ = dict.get<word>("field");

        const auto& solverDict = mesh_.solution().solver(fieldName_);

        const word solverType = solverDict.get<word>("solver");

        if (solverType != GAMGSolver::typeName)
        {
            FatalIOErrorInFunction(dict)
                << "Solver for field '" << fieldName_ << "' is not a "
                << GAMGSolver::typeName << " solver. "
                << "Agglomeration info is only available for GAMG."
                << exit(FatalIOError);
        }

        agglomName_ =
            solverDict.getOrDefault<word>("name", GAMGAgglomeration::typeName);

        return true;
    }

    return false;
}


bool Foam::functionObjects::agglomerationInfo::write()
{
    Log << type() << " " << name() << ":" << nl;

    auto& os = file();

    const GAMGAgglomeration& agglom = this->agglomeration();

    // Number of agglomeration levels
    const label nLevels = agglom.size();

    if (nLevels == 0)
    {
        WarningInFunction
            << "GAMGAgglomeration '" << agglomName_ << "' has no levels. "
            << "Nothing to write." << nl;

        return true;
    }

    // Find the proc-agglomeration level  (if it exists)
    label procAgglomLeveli = -1;
    for (label leveli = 0; leveli < nLevels; ++leveli)
    {
        if (agglom.hasProcMesh(leveli))
        {
            procAgglomLeveli = leveli;
            break;
        }
    }

    writeHeaderValue(os, "name", name());
    writeHeaderValue(os, "time", mesh_.time().timeName());
    writeHeaderValue(os, "field", fieldName_);
    writeHeaderValue(os, "levels", nLevels+1);

    if (procAgglomLeveli >= 0)
    {
        writeHeaderValue(os, "proc-agglom@level", procAgglomLeveli+1);
    }

    writeCommented(os, "level");
    writeTabbed(os, "total");
    writeTabbed(os, "min");
    writeTabbed(os, "avg");
    writeTabbed(os, "max");
    writeTabbed(os, "assigned");
    writeTabbed(os, "field");
    os << nl;

    writeLevel0();

    // Initialise 'cellsPerLevel' with cell labels for agglomeration level 0
    // Note: always has size == mesh_.nCells()
    labelField cellsPerLevel(agglom.restrictAddressing(0));

    // Unified stats gather over the full mesh communicator
    // - Pre-agglom: all procs have valid nCells.
    // - Proc-agglom / post-agglom: masters have valid nCells, non-masters
    //   have n < 0
    const label statsComm = mesh_.comm();

    for (label leveli = 0; leveli < nLevels; ++leveli)
    {
        setCellsPerLevel(agglom, leveli, procAgglomLeveli, cellsPerLevel);

        labelList allNCells(UPstream::nProcs(statsComm), Zero);
        allNCells[UPstream::myProcNo(statsComm)] = agglom.nCells(leveli);

        Pstream::gatherList(allNCells, UPstream::msgType(), statsComm);
        Pstream::broadcast(allNCells, statsComm);
        label displayNCells = 0;
        label minNCells = labelMax;
        label maxNCells = 0;
        label nActive = 0;

        for (label n : allNCells)
        {
            if (n >= 0)
            {
                displayNCells += n;
                minNCells = min(minNCells, n);
                maxNCells = max(maxNCells, n);

                ++nActive;
            }
        }

        const scalar avgNCells =
            nActive > 0 ? scalar(displayNCells)/nActive : 0;

        // For pre-proc-agglom levels, cellsPerLevel holds proc-local
        // 0-based cluster indices. Add per-proc rank offset to produce
        // globally unique indices for visualisation after reconstruction

        labelField writeField(cellsPerLevel);

        if (procAgglomLeveli < 0)
        {
            // Pre-agglom: offset by count of cells on lower-ranked procs.
            label myOffseti = 0;
            for (label p = 0; p < UPstream::myProcNo(statsComm); ++p)
            {
                myOffseti += allNCells[p];
            }

            if (myOffseti > 0)
            {
                for (auto& wf : writeField)
                {
                    wf += myOffseti;
                }
            }
        }
        else
        {
            // Post-proc-agglom: coarse IDs are group-local; add a per-
            // group prefix-sum offset so that IDs are globally unique.
            // agglomProcIDs(procAgglomLeveli)[0] is the world rank of this
            // group's master - used to index into allNCells (rank-ordered).
            const label myMasterRank =
            agglom.agglomProcIDs(procAgglomLeveli)[0];
            label groupOffseti = 0;
            for (label p = 0; p < myMasterRank; ++p)
            {
                if (allNCells[p] >= 0)  // valid master entry
                {
                    groupOffseti += allNCells[p];
                }
            }
            if (groupOffseti > 0)
            {
                for (auto& wf : writeField)
                {
                    wf += groupOffseti;
                }
            }
        }

        auto tagglomField = initAgglomField(agglom, leveli, procAgglomLeveli);
        auto& agglomField = tagglomField.ref();

        // Fill agglomeration field with globally-unique coarse cluster ids
        forAll(agglomField, celli)
        {
            agglomField[celli] = scalar(writeField[celli]);
        }

        // "assigned" column: which procs/masters are active at this level
        // - pre-agglom  : "N_procs"
        // - proc-agglom : "N_procs-M_masters"
        // - post-agglom : "M_masters"

        const word levelStr = Foam::name(leveli + 1);
        word assignedStr;

        if (procAgglomLeveli < 0 || leveli < procAgglomLeveli)
        {
            const label nP = label(allNCells.size());
            assignedStr = Foam::name(nP) + "_procs";
        }
        else if (leveli == procAgglomLeveli)
        {
            assignedStr =
                Foam::name(allNCells.size()) + "_procs-"
              + Foam::name(nActive) + "_masters";
        }
        else
        {
            assignedStr = Foam::name(nActive) + "_masters";
        }

        os  << levelStr << tab
            << displayNCells << tab
            << minNCells << tab
            << label(avgNCells) << tab
            << maxNCells << tab
            << assignedStr << tab
            << agglomField.name() << tab
            << (leveli == nLevels - 1 ? "\n\n" : "\n");

        Info<< "    Writing agglomeration field for level " << leveli + 1
            << " : " << agglomField.name() << endl;

        agglomField.write();
    }

    Info<< "    Written file: " << os.name() << nl << endl;

    return true;
}


// ************************************************************************* //
