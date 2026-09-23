/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
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

#include "faFaceZone.H"
#include "faFaceZoneMesh.H"
#include "faMesh.H"
#include "IOstream.H"
// #include "addToRunTimeSelectionTable.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
    defineTypeNameAndDebug(faFaceZone, 0);
    // defineRunTimeSelectionTable(faFaceZone, dictionary);
    // addToRunTimeSelectionTable(faFaceZone, faFaceZone, dictionary);
}

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::faFaceZone::faFaceZone(const faFaceZoneMesh& zm)
:
    zone(),
    zoneMesh_(zm)
{}


Foam::faFaceZone::faFaceZone
(
    const word& name,
    const label index,
    const faFaceZoneMesh& zm
)
:
    zone(name, index),
    zoneMesh_(zm)
{}


Foam::faFaceZone::faFaceZone
(
    const word& name,
    const labelUList& addr,
    const label index,
    const faFaceZoneMesh& zm
)
:
    zone(name, addr, index),
    zoneMesh_(zm)
{}


Foam::faFaceZone::faFaceZone
(
    const word& name,
    labelList&& addr,
    const label index,
    const faFaceZoneMesh& zm
)
:
    zone(name, std::move(addr), index),
    zoneMesh_(zm)
{}


Foam::faFaceZone::faFaceZone
(
    const word& name,
    const dictionary& dict,
    const label index,
    const faFaceZoneMesh& zm
)
:
    zone(name, dict, faFaceZone::labelsName(), index),
    zoneMesh_(zm)
{}


Foam::faFaceZone::faFaceZone
(
    const faFaceZone& originalZone,
    const faFaceZoneMesh& zm,
    const label newIndex
)
:
    zone(originalZone, newIndex),
    zoneMesh_(zm)
{}


Foam::faFaceZone::faFaceZone
(
    const faFaceZone& originalZone,
    Foam::zero,
    const faFaceZoneMesh& zm,
    const label newIndex
)
:
    zone(originalZone, labelList(), newIndex),
    zoneMesh_(zm)
{}


Foam::faFaceZone::faFaceZone
(
    const faFaceZone& originalZone,
    Foam::zero,
    const label index,
    const faFaceZoneMesh& zm
)
:
    zone(originalZone, labelList(), index),
    zoneMesh_(zm)
{}


Foam::faFaceZone::faFaceZone
(
    const faFaceZone& originalZone,
    const labelUList& addr,
    const label index,
    const faFaceZoneMesh& zm
)
:
    faFaceZone(originalZone, Foam::zero{}, index, zm)
{
    labelList::operator=(addr);
}


Foam::faFaceZone::faFaceZone
(
    const faFaceZone& originalZone,
    labelList&& addr,
    const label index,
    const faFaceZoneMesh& zm
)
:
    faFaceZone(originalZone, Foam::zero{}, index, zm)
{
    labelList::transfer(addr);
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

Foam::label Foam::faceZonecellZone::max_index() const noexcept
{
    return zoneMesh_.mesh().nFaces();
}


bool Foam::faFaceZone::checkDefinition(const bool report) const
{
    return zone::checkDefinition(max_index(), report);
}


void Foam::faFaceZone::resetAddressing(faFaceZone&& zn)
{
    if (this == &zn)
    {
        return;  // Self-assignment is a no-op
    }

    clearAddressing();
    labelList::transfer(static_cast<labelList&>(zn));
    zn.clearAddressing();
}


void Foam::faFaceZone::resetAddressing(const faFaceZone& zn)
{
    if (this == &zn)
    {
        return;  // Self-assignment is a no-op
    }

    clearAddressing();
    labelList::operator=(static_cast<const labelList&>(zn));
}


void Foam::faFaceZone::resetAddressing(const labelUList& addr)
{
    clearAddressing();
    labelList::operator=(addr);
}


void Foam::faFaceZone::resetAddressing(labelList&& addr)
{
    clearAddressing();
    labelList::transfer(addr);
}


void Foam::faFaceZone::write(Ostream& os) const
{
    zone::write(os);
    labelList::writeEntry(faFaceZone::labelsName(), os);
}


// * * * * * * * * * * * * * * * Member Operators  * * * * * * * * * * * * * //

void Foam::faFaceZone::operator=(const faFaceZone& zn)
{
    resetAddressing(zn);
}


void Foam::faFaceZone::operator=(const labelUList& addr)
{
    resetAddressing(addr);
}


void Foam::faFaceZone::operator=(labelList&& addr)
{
    resetAddressing(std::move(addr));
}


// * * * * * * * * * * * * * * * Ostream Operator  * * * * * * * * * * * * * //

Foam::Ostream& Foam::operator<<(Ostream& os, const faFaceZone& zn)
{
    zn.write(os);
    os.check(FUNCTION_NAME);
    return os;
}


// ************************************************************************* //
