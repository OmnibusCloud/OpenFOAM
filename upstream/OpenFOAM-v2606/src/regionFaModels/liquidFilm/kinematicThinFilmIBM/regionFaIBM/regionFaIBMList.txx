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

template<class Type>
void Foam::regionModels::areaSurfaceFilmModels::regionFaIBMList::zeroFilter
(
    Type& fld
) const
{
    const auto& list = this->ibm_list();

    if (list.size() == 1)
    {
        list[0].zeroFilter(fld);
    }
    else if (list.size() > 1)
    {
        // Multiple items. Combine as a mask operation
        areaScalarField mask
        (
            IOobject
            (
                "mask",
                fld.time().timeName(),
                fld.mesh(),
                IOobjectOption::NO_READ,
                IOobjectOption::NO_WRITE,
                IOobjectOption::NO_REGISTER
            ),
            fld.mesh(),
            Foam::zero{},
            dimless
        );

        for (const auto& item : list)
        {
            item.addToMask(mask);
        }

        fld *= mask;
    }
}


// ************************************************************************* //
