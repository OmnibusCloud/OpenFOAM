/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2022-2025 OpenCFD Ltd.
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

#include "regionFaIBMList.H"
#include "Time.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace regionModels
{
namespace areaSurfaceFilmModels
{

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

regionFaIBMList::regionFaIBMList(const Time& runTime, const faMesh& aMesh)
:
    IOdictionary
    (
        IOobject
        (
            "regionFaIBMProperties",
            runTime.constant(),
            aMesh.mesh(),
            IOobjectOption::MUST_READ,
            IOobjectOption::NO_WRITE,
            IOobjectOption::REGISTER
        )
    ),
    Ustar_
    (
        IOobject
        (
            "Ustar",
            runTime.timeName(),
            aMesh,
            IOobjectOption::NO_READ,
            IOobjectOption::AUTO_WRITE,
            IOobjectOption::REGISTER
        ),
        aMesh,
        Foam::zero{},
        dimVelocity,
        faPatchFieldBase::zeroGradientType()
    )
{
    const auto& dict = static_cast<IOdictionary&>(*this);
    auto& list = this->ibm_list();

    list.resize(dict.size());

    label count = 0;
    for (const entry& e : dict)
    {
        if (const auto* dictptr = e.dictPtr())
        {
            Info<< "Found regionFaIBM model " << e.keyword() << endl;

            list.emplace_set(count, runTime, aMesh, *dictptr);
            ++count;
        }
        else
        {
            // Perhaps support additional switches in the future?
            WarningInFunction
                << "Entry " << e.keyword() << " is not a dictionary" << endl;
        }
    }

    list.resize(count);
}


void regionFaIBMList::updateMesh()
{
    for (auto& item : this->ibm_list())
    {
        item.updateMesh();
    }
}


bool regionFaIBMList::correct(const areaVectorField& Us)
{
    bool corrected = false;

    // Initialise Ustar from current velocity
    Ustar_ = Us;

    for (auto& item : this->ibm_list())
    {
        if (item.correct(Us, Ustar_))
        {
            corrected = true;
        }
    }

    return corrected;
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace areaSurfaceFilmModels
} // End namespace regionModels
} // End namespace Foam

// ************************************************************************* //
