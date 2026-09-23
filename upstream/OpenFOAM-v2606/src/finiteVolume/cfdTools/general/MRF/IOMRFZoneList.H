/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2012 OpenFOAM Foundation
    Copyright (C) 2019 OpenCFD Ltd.
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

Class
    Foam::IOMRFZoneList

Description
    List of MRF zones with IO functionality.  MRF zones are specified by a list
    of dictionary entries, e.g.

    \verbatim
    zone1
    {
        cellZone    rotor1;
        active      yes;
        ...
    }

    zone2
    {
        cellZone    rotor2;
        active      yes;
        ...
    }
    \endverbatim

SourceFiles
    IOMRFZoneList.cxx

\*---------------------------------------------------------------------------*/

#ifndef Foam_IOMRFZoneList_H
#define Foam_IOMRFZoneList_H

#include "IOdictionary.H"
#include "MRFZoneList.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                        Class IOMRFZoneList Declaration
\*---------------------------------------------------------------------------*/

class IOMRFZoneList
:
    public IOdictionary,
    public MRFZoneList
{
    // Private Member Functions

        //- Construct with specified IOobject for the IOdictionary
        //- (somewhat fragile)
        IOMRFZoneList(const fvMesh& mesh, const IOobject& io);

public:

    // Generated Methods

        //- No copy construct
        IOMRFZoneList(const IOMRFZoneList&) = delete;

        //- No copy assignment
        void operator=(const IOMRFZoneList&) = delete;


    // Constructors

        //- Construct from mesh, optional for specified solver name
        explicit IOMRFZoneList
        (
            const fvMesh& mesh,
            const word& solverName = word::null
        );


        //- Destructor
        virtual ~IOMRFZoneList() = default;


    // Member Functions

        //- Read dictionary
        virtual bool read();
};

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
