/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2020 OpenCFD Ltd.
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

#include "ensightOutputSurface.H"
#include "ensightOutput.H"

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::ensightOutputSurface::ensightOutputSurface
(
    const pointField& points,
    const faceList& faces,
    const string& description
)
:
    ensightFaces(description),
    points_(points),
    faces_(faces),
    vertexOutput_(false)
{
    // Classify face types
    classify(faces);
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::ensightOutputSurface::write(ensightGeoFile& os) const
{
    const bool hasGeometry =
    (
        vertexOutput_ ? !points_.empty() : (total() > 0)
    );

    if (!hasGeometry)
    {
        return;
    }

    // Coordinates
    ensightOutput::Detail::writeCoordinates
    (
        os,
        index(),
        name(),
        points_.size(),
        points_,
        false // serial
    );

    if (vertexOutput_)
    {
        if
        (
            const label nTotalVerts = points_.size();
            (nTotalVerts && UPstream::master()) // serial only
        )
        {
            os.writeKeyword(ensightFaces::kw_vertex());
            os.write(nTotalVerts);
            os.newline();

            for (label pointi = 0; pointi < nTotalVerts; ++pointi)
            {
                os.write(pointi+1);  // From 0-based to 1-based index
                os.newline();
            }
        }
    }
    else
    {
        // Faces
        ensightOutput::writeFaceConnectivity
        (
            os,
            *this,
            faces_,
            false  // serial
        );
    }
}


// ************************************************************************* //
