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

#include "patchDistanceToCell.H"
#include "polyMesh.H"
#include "patchWave.H"
#include "addToRunTimeSelectionTable.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
    defineTypeNameAndDebug(patchDistanceToCell, 0);
    addToRunTimeSelectionTable(topoSetSource, patchDistanceToCell, word);
    addToRunTimeSelectionTable(topoSetSource, patchDistanceToCell, istream);
    addToRunTimeSelectionTable(topoSetCellSource, patchDistanceToCell, word);
    addToRunTimeSelectionTable(topoSetCellSource, patchDistanceToCell, istream);
}


Foam::topoSetSource::addToUsageTable Foam::patchDistanceToCell::usage_
(
    patchDistanceToCell::typeName,
    "\n    Usage: patchDistanceToCell patch distance\n\n"
    "    Select cells within the specified distance from the patch.\n\n"
);


// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

void Foam::patchDistanceToCell::combine(topoSet& set, const bool add) const
{
    labelHashSet patchIDs = mesh_.boundaryMesh().patchSet
    (
        selectedPatches_,
        true,           // warn if not found
        true            // use patch groups if available
    );

    if (patchIDs.empty())
    {
        WarningInFunction
            << "Cannot find any patches matching "
            << flatOutput(selectedPatches_) << nl
            << "Valid names: " << flatOutput(mesh_.boundaryMesh().names())
            << endl;
        return;
    }

    if (verbose_)
    {
        label nFaces = 0;
        for (const label patchi : patchIDs)
        {
            nFaces +=
                returnReduce
                (
                    mesh_.boundaryMesh()[patchi].size(),
                    sumOp<label>()
                );
        }
        Info<< "    Computing distance from "
            << patchIDs.size() << " patch(es) with "
            << nFaces << " faces total ..." << endl;
    }

    // Compute geometric distance from every cell to the selected patches
    // using MeshWave (same algorithm as turbulence wall distance).
    // correctWalls=true gives the exact nearest-point distance for
    // cells directly adjacent to the patch.
    const patchWave wave(mesh_, patchIDs, true);

    if (wave.nUnset() > 0 && verbose_)
    {
        WarningInFunction
            << wave.nUnset() << " cell(s) did not receive a valid distance."
            << " These cells will not be selected." << endl;
    }

    const scalarField& dist = wave.distance();

    forAll(dist, celli)
    {
        if (dist[celli] >= minDistance_ && dist[celli] <= distance_)
        {
            addOrDelete(set, celli, add);
        }
    }
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::patchDistanceToCell::patchDistanceToCell
(
    const polyMesh& mesh,
    const wordRe& patchName,
    const scalar distance,
    const scalar minDistance
)
:
    topoSetCellSource(mesh),
    selectedPatches_(one{}, patchName),
    distance_(distance),
    minDistance_(minDistance)
{}


Foam::patchDistanceToCell::patchDistanceToCell
(
    const polyMesh& mesh,
    const dictionary& dict
)
:
    topoSetCellSource(mesh, dict),
    selectedPatches_(),
    distance_(dict.getCheck<scalar>("distance", scalarMinMax::ge(0))),
    minDistance_
    (
        dict.getCheckOrDefault<scalar>("minDistance", 0, scalarMinMax::ge(0))
    )
{
    if (!dict.readIfPresent("patches", selectedPatches_))
    {
        selectedPatches_.resize(1);
        selectedPatches_.front() = dict.get<wordRe>("patch");
    }

    if (minDistance_ > distance_)
    {
        FatalIOErrorInFunction(dict)
            << "minDistance (" << minDistance_
            << ") must not exceed distance (" << distance_ << ")"
            << exit(FatalIOError);
    }
}


Foam::patchDistanceToCell::patchDistanceToCell
(
    const polyMesh& mesh,
    Istream& is
)
:
    topoSetCellSource(mesh),
    selectedPatches_(one{}, wordRe(checkIs(is))),
    distance_(readScalar(checkIs(is))),
    minDistance_(0)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::patchDistanceToCell::applyToSet
(
    const topoSetSource::setAction action,
    topoSet& set
) const
{
    if (action == topoSetSource::ADD || action == topoSetSource::NEW)
    {
        if (verbose_)
        {
            Info<< "    Adding cells within distance ["
                << minDistance_ << ", " << distance_
                << "] from patches: "
                << flatOutput(selectedPatches_) << " ..." << endl;
        }

        combine(set, true);
    }
    else if (action == topoSetSource::SUBTRACT)
    {
        if (verbose_)
        {
            Info<< "    Removing cells within distance ["
                << minDistance_ << ", " << distance_
                << "] from patches: "
                << flatOutput(selectedPatches_) << " ..." << endl;
        }

        combine(set, false);
    }
}


// ************************************************************************* //
