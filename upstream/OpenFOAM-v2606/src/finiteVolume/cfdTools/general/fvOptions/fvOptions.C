/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2017 OpenFOAM Foundation
    Copyright (C) 2019-2025 OpenCFD Ltd.
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

#include "fvOptions.H"
#include "fvMesh.H"
#include "Time.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
    namespace fv
    {
        defineTypeNameAndDebug(options, 0);
    }
}


// * * * * * * * * * * * * * * * Local Functions * * * * * * * * * * * * * * //

namespace
{

// Create IO object if dictionary is present
Foam::IOobject createIOobject
(
    const Foam::fvMesh& mesh,
    const Foam::word& baseName  // eg, fvOptions
)
{
    using namespace Foam;

    IOobject io
    (
        baseName,
        mesh.time().constant(),
        mesh.thisDb(),
        IOobjectOption::MUST_READ,
        IOobjectOption::NO_WRITE,
        IOobjectOption::REGISTER
    );

    if (io.typeHeaderOk<IOdictionary>(true))
    {
        io.readOpt(IOobjectOption::READ_MODIFIED);
    }
    else
    {
        // Check if the fvOptions file is in system
        io.instance() = mesh.time().system();

        if (io.typeHeaderOk<IOdictionary>(true))
        {
            io.readOpt(IOobjectOption::READ_MODIFIED);
        }
        else
        {
            io.readOpt(IOobjectOption::NO_READ);
        }
    }

    if (io.isAnyRead())
    {
        Info<< "Creating finite-volume options from "
            << io.instance()/io.name() << nl
            << endl;
    }

    return io;
}

} // End anonymous namespace


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::fv::options::options
(
    const fvMesh& mesh,
    const IOobject& io
)
:
    IOdictionary(io),
    fv::optionList(mesh, *this)
{}


Foam::fv::options::options
(
    const fvMesh& mesh
)
:
    IOdictionary(createIOobject(mesh, typeName)),
    fv::optionList(mesh, *this)
{}


Foam::fv::options& Foam::fv::options::New(const fvMesh& mesh)
{
    auto* ptr = mesh.thisDb().getObjectPtr<fv::options>(typeName);

    if (!ptr)
    {
        DebugInFunction
            << "Constructing " << typeName
            << " for region " << mesh.name() << nl;

        ptr = new fv::options(mesh);
        regIOobject::store(ptr);
    }

    return *ptr;
}


bool Foam::fv::options::read()
{
    if (IOdictionary::regIOobject::read())
    {
        fv::optionList::read(*this);
        return true;
    }

    return false;
}


// ************************************************************************* //
