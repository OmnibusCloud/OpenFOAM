/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2017 OpenFOAM Foundation
    Copyright (C) 2020-2025 OpenCFD Ltd.
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

Application
    Test-CompactIOList

Description
    Simple demonstration and test application for the CompactIOList container

\*---------------------------------------------------------------------------*/

#include "IOstreams.H"
#include "argList.H"
#include "Time.H"
#include "polyMesh.H"

using namespace Foam;

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //


//  Main program:

int main(int argc, char *argv[])
{
    argList::addBoolOption("ascii", "use ascii format");
    argList::addOption("count", "number of faces");

    #include "setRootCase.H"
    #include "createTime.H"

    IOstreamOption streamOpt(IOstreamOption::BINARY);

    if (args.found("ascii"))
    {
        streamOpt.format(IOstreamOption::ASCII);
    }

    const label size = args.getOrDefault<label>("count", 20000000);

    // Old format
    // ~~~~~~~~~~

    {
        // Construct big faceList in old format
        faceIOList faces2
        (
            IOobject
            (
                "faces2-plain",
                runTime.constant(),
                polyMesh::meshSubDir,
                runTime,
                IOobject::NO_READ,
                IOobject::NO_WRITE,
                IOobject::NO_REGISTER
            )
        );

        faces2.resize(size, face(identity(4)));

        Info<< "Plain format faceList " << faces2.objectRelPath() << nl;
        Info<< "    constructed in = " << runTime.cpuTimeIncrement()
            << " s" << endl;


        faces2.writeObject(streamOpt, true);

        Info<< "    wrote in = "
            << runTime.cpuTimeIncrement() << " s" << endl;

        // Read (size only)
        label count = faceIOList::readContentsSize
        (
            IOobject
            (
                "faces2-plain",
                runTime.constant(),
                polyMesh::meshSubDir,
                runTime,
                IOobject::MUST_READ
            )
        );

        Info<< "    counted " << count << " faces on disk in = "
            << runTime.cpuTimeIncrement() << " s" << endl;

        // Read
        faceIOList faces2b
        (
            IOobject
            (
                "faces2-plain",
                runTime.constant(),
                polyMesh::meshSubDir,
                runTime,
                IOobject::MUST_READ,
                IOobject::NO_WRITE,
                IOobject::NO_REGISTER
            )
        );

        Info<< "    read " << faces2b.size() << " faces in = "
            << runTime.cpuTimeIncrement() << " s" << nl << endl;
    }


    // New format
    // ~~~~~~~~~~

    {
        // Construct big faceList in compact format
        faceCompactIOList faces2
        (
            IOobject
            (
                "faces2-compact",
                runTime.constant(),
                polyMesh::meshSubDir,
                runTime,
                IOobject::NO_READ,
                IOobject::NO_WRITE,
                IOobject::NO_REGISTER
            )
        );

        faces2.resize(size, face(identity(4)));

        Info<< "Compact format faceList" << faces2.objectRelPath() << nl;
        Info<< "    constructed in = "
            << runTime.cpuTimeIncrement() << " s" << endl;


        faces2.writeObject(streamOpt, true);

        Info<< "    wrote in = "
            << runTime.cpuTimeIncrement() << " s" << endl;

        // Read (size only)
        label count = faceCompactIOList::readContentsSize
        (
            IOobject
            (
                "faces2-compact",
                runTime.constant(),
                polyMesh::meshSubDir,
                runTime,
                IOobject::MUST_READ
            )
        );
        Info<< "    counted " << count << " faces on disk in = "
            << runTime.cpuTimeIncrement() << " s" << endl;

        // Read
        faceCompactIOList faces2b
        (
            IOobject
            (
                "faces2-compact",
                runTime.constant(),
                polyMesh::meshSubDir,
                runTime,
                IOobject::MUST_READ,
                IOobject::NO_WRITE,
                IOobject::NO_REGISTER
            )
        );

        Info<< "    read " << faces2b.size() << " faces in = "
            << runTime.cpuTimeIncrement() << " s" << nl << endl;
    }

    return 0;
}


// ************************************************************************* //
