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

Class
    Foam::vtk::vertexWriter

Description
    Write vertex/points (optionally with fields)
    as a vtp file or a legacy vtk file.

    The file output states are managed by the Foam::vtk::fileWriter class.
    FieldData (eg, TimeValue) must appear before any geometry pieces.

Note
    Parallel output is combined into a single Piece without point merging,
    which is similar to using multi-piece data sets, but allows more
    convenient creation as a streaming process.

SourceFiles
    foamVtkVertexWriter.cxx

\*---------------------------------------------------------------------------*/

#ifndef Foam_vtk_vertexWriter_H
#define Foam_vtk_vertexWriter_H

#include "foamVtkPolyWriter.H"
#include "instant.H"

#include <functional>

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace vtk
{

/*---------------------------------------------------------------------------*\
                       Class vtk::vertexWriter Declaration
\*---------------------------------------------------------------------------*/

class vertexWriter
:
    public vtk::polyWriter
{
    // Private Member Data

        //- Reference to the points
        std::reference_wrapper<const pointField> points_;

        //- Time name/value
        instant instant_;


    // Private Member Functions

        //- No copy construct
        vertexWriter(const vertexWriter&) = delete;

        //- No copy assignment
        void operator=(const vertexWriter&) = delete;


public:

    // Constructors

        //- Construct from components (default format INLINE_BASE64)
        explicit vertexWriter
        (
            const pointField& pts,
            const vtk::outputOptions opts = vtk::formatType::INLINE_BASE64
        );

        //- Construct from components (default format INLINE_BASE64),
        //- and open the file for writing.
        //  The file name is with/without an extension.
        vertexWriter
        (
            const pointField& pts,
            const fileName& file,
            bool parallel = UPstream::parRun()
        );

        //- Construct from components and open the file for writing.
        //  The file name is with/without an extension.
        vertexWriter
        (
            const pointField& pts,
            const vtk::outputOptions opts,
            const fileName& file,
            bool parallel = UPstream::parRun()
        );


    //- Destructor
    virtual ~vertexWriter() = default;


    // Member Functions

        //- Define a time name/value for the output
        virtual void setTime(const instant& inst);

        //- Write file header (non-collective)
        //  \note Expected calling states: (OPENED).
        virtual bool beginFile(std::string title = "");

        //- Write vertex topology
        //  Also writes the file header if not previously written.
        //  \note Must be called prior to writing CellData or PointData
        virtual bool writeGeometry();

        //- Write "TimeValue" FieldData (name as per Catalyst output)
        //  Must be called within the FIELD_DATA state.
        //  \note As a convenience this can also be called from
        //      (OPENED | DECLARED) states, in which case it invokes
        //      beginFieldData(1) internally.
        using vtk::fileWriter::writeTimeValue;

        //- Write the currently set time as "TimeValue" FieldData
        void writeTimeValue();

        //- Reset point references to begin a new piece
        void piece(const pointField& points);
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace vtk
} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
