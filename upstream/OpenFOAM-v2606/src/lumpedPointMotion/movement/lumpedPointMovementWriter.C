/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2016-2026 OpenCFD Ltd.
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

#include "lumpedPointMovement.H"
#include "polyMesh.H"
#include "pointMesh.H"
#include "foamVtkSurfaceWriter.H"
#include "foamVtkVertexWriter.H"

// * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * * //

void Foam::lumpedPointMovement::writeStateVTP
(
    const lumpedPointState& state,
    const fileName& file
) const
{
    if (!UPstream::master())
    {
        // No extra information available from slaves, write on master only.
        return;
    }

    labelListList lines;

    if (label nLines = controllers_.size(); nLines > 0)
    {
        lines.resize(nLines);
        nLines = 0;

        for (const word& ctrlName : controllers_.sortedToc())
        {
            lines[nLines] = controllers_[ctrlName]->pointLabels();
            ++nLines;
        }
    }
    else
    {
        // Default - global with all points as a single line
        lines.resize(1);
        lines.first() = identity(state.size());
    }

    state.writeVTP(file, lines, originalIds_);
}


void Foam::lumpedPointMovement::writeStateVTP(const fileName& file) const
{
    writeStateVTP(state(), file);
}


void Foam::lumpedPointMovement::writeForcesAndMomentsVTP
(
    const fileName& file,
    const UList<vector>& forces,
    const UList<vector>& moments
) const
{
    if (!UPstream::master())
    {
        // Force, moments already reduced
        return;
    }

    vtk::vertexWriter writer
    (
        state().points(),
        vtk::formatType::INLINE_ASCII,
        file,
        false   // non-parallel
    );

    writer.writeGeometry();

    const auto nPoints = state().points().size();

    int nFields(0);
    if (forces.size() == nPoints) ++nFields;
    if (moments.size() == nPoints) ++nFields;

    if (!nFields)
    {
        return;
    }

    // CellData
    writer.beginCellData(nFields);
    if (forces.size() == nPoints)
    {
        writer.writeCellData("forces", forces);
    }
    if (moments.size() == nPoints)
    {
        writer.writeCellData("moments", moments);
    }

    // PointData
    writer.beginPointData(nFields);
    if (forces.size() == nPoints)
    {
        writer.writePointData("forces", forces);
    }
    if (moments.size() == nPoints)
    {
        writer.writePointData("moments", moments);
    }
}


void Foam::lumpedPointMovement::writeZonesVTP
(
    const fileName& file,
    const polyMesh& mesh,
    const pointField& points0
) const
{
    const polyBoundaryMesh& patches = mesh.boundaryMesh();
    const labelList patchIds(patchControls_.sortedToc());

    vtk::surfaceWriter writer
    (
        pointField::null(),
        faceList::null(),
        vtk::formatType::INLINE_ASCII,
        file
    );

    for (const label patchi : patchIds)
    {
        const labelList& faceToPoint = patchControls_[patchi].faceToPoint_;

        primitivePatch pp(patches[patchi].faces(), points0);

        writer.piece(pp.localPoints(), pp.localFaces());

        writer.writeGeometry();

        writer.beginCellData(2);

        writer.writeUniform("patchId", patchi);
        writer.write("lumpedId", faceToPoint);

        writer.endCellData();
    }
}


void Foam::lumpedPointMovement::writeVTP
(
    const fileName& file,
    const polyMesh& mesh,
    const pointField& points0
) const
{
    writeVTP(file, state(), mesh, points0);
}


void Foam::lumpedPointMovement::writeVTP
(
    const fileName& file,
    const lumpedPointState& state,
    const polyMesh& mesh,
    const pointField& points0
) const
{
    const polyBoundaryMesh& patches = mesh.boundaryMesh();
    const labelList patchIds(patchControls_.sortedToc());

    pointMesh ptMesh(mesh);

    vtk::surfaceWriter writer
    (
        pointField::null(),
        faceList::null(),
        vtk::formatType::INLINE_ASCII,
        file
    );

    for (const label patchi : patchIds)
    {
        const polyPatch& pp = patches[patchi];

        const pointPatch& ptPatch = ptMesh.boundary()[patchi];

        // Current position (not displacement)
        tmp<pointField> tpts = pointsPosition(state, ptPatch, points0);

        writer.piece(tpts(), pp.localFaces());

        writer.writeGeometry();

        // Face mapping
        const labelList& faceToPoint = patchControls_[patchi].faceToPoint_;

        writer.beginCellData(2);

        writer.writeUniform("patchId", patchi);
        writer.write("lumpedId", faceToPoint);

        writer.endCellData();

        // The interpolator
        const List<lumpedPointInterpolator>& interpList
            = patchControls_[patchi].interp_;

        writer.beginPointData(3);

        // Nearest, Next
        {
            labelList intData(interpList.size());

            forAll(interpList, i)
            {
                intData[i] = interpList[i].nearest();
            }
            writer.write("nearest", intData);

            forAll(interpList, i)
            {
                intData[i] = interpList[i].next1();
            }
            writer.write("next1", intData);


            forAll(interpList, i)
            {
                intData[i] = interpList[i].next2();
            }
            writer.write("next2", intData);
        }

        // Weights
        {
            scalarList floatData(interpList.size());

            forAll(interpList, i)
            {
                floatData[i] = interpList[i].weight0();
            }
            writer.write("weight", floatData);

            forAll(interpList, i)
            {
                floatData[i] = interpList[i].weight1();
            }
            writer.write("weight1", floatData);

            forAll(interpList, i)
            {
                floatData[i] = interpList[i].weight2();
            }
            writer.write("weight2", floatData);
        }

        writer.endPointData();
    }
}


// ************************************************************************* //
