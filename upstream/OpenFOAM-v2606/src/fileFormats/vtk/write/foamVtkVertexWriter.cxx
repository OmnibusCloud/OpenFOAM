/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
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

#include "foamVtkVertexWriter.H"

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::vtk::vertexWriter::vertexWriter
(
    const pointField& points,
    const vtk::outputOptions opts
)
:
    vtk::polyWriter(opts),

    points_(std::cref<pointField>(points)),
    instant_()
{}


Foam::vtk::vertexWriter::vertexWriter
(
    const pointField& points,
    const fileName& file,
    bool parallel
)
:
    vertexWriter(points)
{
    open(file, parallel);
}


Foam::vtk::vertexWriter::vertexWriter
(
    const pointField& points,
    const vtk::outputOptions opts,
    const fileName& file,
    bool parallel
)
:
    vertexWriter(points, opts)
{
    open(file, parallel);
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::vtk::vertexWriter::setTime(const instant& inst)
{
    instant_ = inst;
}


bool Foam::vtk::vertexWriter::beginFile(std::string title)
{
    if (title.size())
    {
        return vtk::fileWriter::beginFile(title);
    }

    if (!instant_.name().empty())
    {
        return vtk::fileWriter::beginFile
        (
            "time='" + instant_.name() + "'"
        );
    }

    // Provide default title
    return vtk::fileWriter::beginFile("vertices");
}


bool Foam::vtk::vertexWriter::writeGeometry()
{
    return writeVertGeometry(points_.get());
}


void Foam::vtk::vertexWriter::writeTimeValue()
{
    if (!instant_.name().empty())
    {
        vtk::fileWriter::writeTimeValue(instant_.value());
    }
}


void Foam::vtk::vertexWriter::piece
(
    const pointField& points
)
{
    endPiece();

    points_ = std::cref<pointField>(points);
}


// ************************************************************************* //
