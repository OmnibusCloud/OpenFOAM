/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
    Copyright (C) 2017-2025 OpenCFD Ltd.
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

#include "pointZone.H"
#include "pointZoneMesh.H"
#include "polyMesh.H"
#include "syncTools.H"
#include "addToRunTimeSelectionTable.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
    defineTypeNameAndDebug(pointZone, 0);
    defineRunTimeSelectionTable(pointZone, dictionary);
    addToRunTimeSelectionTable(pointZone, pointZone, dictionary);
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::pointZone::pointZone(const pointZoneMesh& zm)
:
    zone(),
    zoneMesh_(zm)
{}


Foam::pointZone::pointZone
(
    const word& name,
    const label index,
    const pointZoneMesh& zm
)
:
    zone(name, index),
    zoneMesh_(zm)
{}


Foam::pointZone::pointZone
(
    const word& name,
    const labelUList& addr,
    const label index,
    const pointZoneMesh& zm
)
:
    zone(name, addr, index),
    zoneMesh_(zm)
{}


Foam::pointZone::pointZone
(
    const word& name,
    labelList&& addr,
    const label index,
    const pointZoneMesh& zm
)
:
    zone(name, std::move(addr), index),
    zoneMesh_(zm)
{}


Foam::pointZone::pointZone
(
    const word& name,
    const dictionary& dict,
    const label index,
    const pointZoneMesh& zm
)
:
    zone(name, dict, pointZone::labelsName(), index),
    zoneMesh_(zm)
{}


Foam::pointZone::pointZone
(
    const pointZone& originalZone,
    const pointZoneMesh& zm,
    const label newIndex
)
:
    zone(originalZone, newIndex),
    zoneMesh_(zm)
{}


Foam::pointZone::pointZone
(
    const pointZone& originalZone,
    Foam::zero,
    const pointZoneMesh& zm,
    const label newIndex
)
:
    zone(originalZone, labelList(), newIndex),
    zoneMesh_(zm)
{}


Foam::pointZone::pointZone
(
    const pointZone& originalZone,
    Foam::zero,
    const label newIndex,
    const pointZoneMesh& zm
)
:
    zone(originalZone, labelList(), newIndex),
    zoneMesh_(zm)
{}


Foam::pointZone::pointZone
(
    const pointZone& originalZone,
    const labelUList& addr,
    const label index,
    const pointZoneMesh& zm
)
:
    pointZone(originalZone, Foam::zero{}, index, zm)
{
    labelList::operator=(addr);
}


Foam::pointZone::pointZone
(
    const pointZone& originalZone,
    labelList&& addr,
    const label index,
    const pointZoneMesh& zm
)
:
    pointZone(originalZone, Foam::zero{}, index, zm)
{
    labelList::transfer(addr);
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

Foam::label Foam::pointZone::max_index() const noexcept
{
    return zoneMesh_.mesh().nPoints();
}


// void Foam::pointZone::sort()
// {
//     clearAddressing();
//     Foam::sort(static_cast<labelList&>(*this));
// }


bool Foam::pointZone::checkParallelSync(const bool report) const
{
    const polyMesh& mesh = zoneMesh().mesh();

    labelList maxZone(mesh.nPoints(), label(-1));
    labelList minZone(mesh.nPoints(), labelMax);

    const labelList& addr = *this;

    for (const label pointi : addr)
    {
        maxZone[pointi] = index();
        minZone[pointi] = index();
    }
    syncTools::syncPointList(mesh, maxZone, maxEqOp<label>(), label(-1));
    syncTools::syncPointList(mesh, minZone, minEqOp<label>(), labelMax);

    bool hasError = false;

    forAll(maxZone, pointi)
    {
        // Check point in same (or no) zone on all processors
        if
        (
            (
                maxZone[pointi] != -1
             || minZone[pointi] != labelMax
            )
         && (maxZone[pointi] != minZone[pointi])
        )
        {
            hasError = true;
            if (report)
            {
                Info<< " ***Problem with pointZone " << index()
                    << " named " << name()
                    << ". Point " << pointi
                    << " at " << mesh.points()[pointi]
                    << " is in zone "
                    << (minZone[pointi] == labelMax ? -1 : minZone[pointi])
                    << " on some processors and in zone "
                    << maxZone[pointi]
                    << " on some other processors." << nl
                    << "(suppressing further warnings)"
                    << endl;
            }
            break;  // Only report once
        }
    }

    return hasError;
}


void Foam::pointZone::resetAddressing(pointZone&& zn)
{
    if (this == &zn)
    {
        return;  // Self-assignment is a no-op
    }

    clearAddressing();
    labelList::transfer(static_cast<labelList&>(zn));
    zn.clearAddressing();
}


void Foam::pointZone::resetAddressing(const pointZone& zn)
{
    if (this == &zn)
    {
        return;  // Self-assignment is a no-op
    }

    clearAddressing();
    labelList::operator=(static_cast<const labelList&>(zn));
}


void Foam::pointZone::resetAddressing(const labelUList& addr)
{
    clearAddressing();
    labelList::operator=(addr);
}


void Foam::pointZone::resetAddressing(labelList&& addr)
{
    clearAddressing();
    labelList::transfer(addr);
}


void Foam::pointZone::write(Ostream& os) const
{
    zone::write(os);
    labelList::writeEntry(pointZone::labelsName(), os);
}


// * * * * * * * * * * * * * * * Member Operators  * * * * * * * * * * * * * //

void Foam::pointZone::operator=(const pointZone& zn)
{
    if (this == &zn)
    {
        return;  // Self-assignment is a no-op
    }

    clearAddressing();
    labelList::operator=(static_cast<const labelList&>(zn));
}


void Foam::pointZone::operator=(const labelUList& addr)
{
    clearAddressing();
    labelList::operator=(addr);
}


void Foam::pointZone::operator=(labelList&& addr)
{
    clearAddressing();
    labelList::transfer(addr);
}


// * * * * * * * * * * * * * * * Ostream Operator  * * * * * * * * * * * * * //

Foam::Ostream& Foam::operator<<(Ostream& os, const pointZone& zn)
{
    zn.write(os);
    os.check(FUNCTION_NAME);
    return os;
}


// ************************************************************************* //
