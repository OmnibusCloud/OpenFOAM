/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2016-2017 OpenFOAM Foundation
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

#include "wallHeatFlux.H"
#include "wallHeatFluxModel.H"
#include "addToRunTimeSelectionTable.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
namespace functionObjects
{
    defineTypeNameAndDebug(wallHeatFlux, 0);
    addToRunTimeSelectionTable(functionObject, wallHeatFlux, dictionary);
}
}

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::functionObjects::wallHeatFlux::wallHeatFlux
(
    const word& name,
    const Time& runTime,
    const dictionary& dict
)
:
    fvMeshFunctionObject(name, runTime, dict),
    qModelPtr_
    (
        wallHeatFluxModel::New
        (
            dict,
            mesh_,
            name,
            scopedName(typeName),
            *this
        )
    )
{
    read(dict);
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

bool Foam::functionObjects::wallHeatFlux::read(const dictionary& dict)
{
    Log << type() << ' ' << name() << " read:" << endl;
    if (!fvMeshFunctionObject::read(dict) || !qModelPtr_->read(dict))
    {
        return false;
    }
    return true;
}


bool Foam::functionObjects::wallHeatFlux::execute()
{
    Log << type() << ' ' << name() << " execute:" << endl;
    return qModelPtr_->execute();
}


bool Foam::functionObjects::wallHeatFlux::write()
{
    Log << type() << ' ' << name() << " write:" << endl;
    return qModelPtr_->write();
}


// ************************************************************************* //
