/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
    Copyright (C) 2026 OpenCFD Ltd.
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

#include "interpolatePointToCell.H"

// * * * * * * * * * * * * * * * Global Functions  * * * * * * * * * * * * * //

template<class Type>
Type Foam::interpolatePointToCell
(
    const DimensionedField<Type, pointMesh>& ptf,
    const label celli
)
{
    const primitiveMesh& mesh = ptf.mesh()();

    labelHashSet usedPoints;

    Type sum = Zero;

    for (const label facei : mesh.cells()[celli])
    {
        for (const label pointi : mesh.faces()[facei])
        {
            if (usedPoints.insert(pointi))
            {
                sum += ptf[pointi];
            }
        }
    }

    if (label npts = usedPoints.size(); npts > 0)
    {
        return sum/npts;
    }
    else
    {
        return sum;
    }
}


template<class Type>
Foam::Field<Type> Foam::interpolatePointToCell
(
    const DimensionedField<Type, pointMesh>& ptf,
    const labelUList& cellIds
)
{
    const primitiveMesh& mesh = ptf.mesh()();

    labelHashSet usedPoints;

    Field<Type> result(cellIds.size(), Foam::zero{});
    auto iter = result.begin();

    for (const label celli : cellIds)
    {
        auto& sum = *iter;
        ++iter;

        usedPoints.clear();

        for (const label facei : mesh.cells()[celli])
        {
            for (const label pointi : mesh.faces()[facei])
            {
                if (usedPoints.insert(pointi))
                {
                    sum += ptf[pointi];
                }
            }
        }

        if (label npts = usedPoints.size(); npts > 0)
        {
            sum /= npts;
        }
    }

    return result;
}


// ************************************************************************* //
