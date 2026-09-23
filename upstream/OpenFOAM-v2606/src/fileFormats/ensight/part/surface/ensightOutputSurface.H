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

Class
    Foam::ensightOutputSurface

Description
    A variant of ensightFaces that holds references to contiguous
    points/faces with its own encapsulated write methods.
    The surface is assumed to have been merged prior, thus the output is
    serial-only.

Note
    The primary use is for the Foam::surfaceWriters::ensightWriter
    but can be used independently as well.

SourceFiles
    ensightOutputSurface.C
    ensightOutputSurface.txx

\*---------------------------------------------------------------------------*/

#ifndef Foam_ensightOutputSurface_H
#define Foam_ensightOutputSurface_H

#include "ensightFaces.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                    Class ensightOutputSurface Declaration
\*---------------------------------------------------------------------------*/

class ensightOutputSurface
:
    public ensightFaces
{
    // Private Data

        //- The referenced pointField
        const pointField& points_;

        //- The referenced faces
        const faceList& faces_;

        //- Output as 1D vertex/point elements instead of faces
        bool vertexOutput_;


public:

    // Generated Methods

        //- No copy construct
        ensightOutputSurface(const ensightOutputSurface&) = delete;

        //- No copy assignment
        void operator=(const ensightOutputSurface&) = delete;


    // Constructors

        //- Construct from points and faces.
        //- Part receives the specified name (default: "surface").
        ensightOutputSurface
        (
            const pointField& points,
            const faceList& faces,
            const string& description = "surface"
        );


    //- Destructor
    virtual ~ensightOutputSurface() = default;


    // Member Functions

        //- Output as 1D vertex/point elements instead of faces
        bool vertexOutput() const noexcept { return vertexOutput_; }

        //- Output as 1D vertex/point elements instead of faces
        bool vertexOutput(bool on) noexcept
        {
            bool old(vertexOutput_);
            vertexOutput_ = on;
            return old;
        }

        //- Processor-local test for any elements
        bool empty() const noexcept
        {
            return (vertexOutput_ ? points_.empty() : ensightFaces::empty());
        }

        //- Processor-local size of all elements
        label size() const noexcept
        {
            return (vertexOutput_ ? points_.size() : ensightFaces::size());
        }


    // Member Functions

        //- Write processor-local geometry (serial-only).
        //- No beginGeometry() marker.
        void write(ensightGeoFile& os) const;

        //- Write a field of cell or point values (serial-only)
        template<class Type>
        void writeData
        (
            ensightFile& os,
            const Field<Type>& fld,
            const bool isPointData = false
        ) const;

        //- Write a field of cell (face or vertex) values (serial-only)
        template<class Type>
        void writeCellData(ensightFile& os, const Field<Type>& fld) const;

        //- Write a field of point values (serial-only)
        template<class Type>
        void writePointData(ensightFile& os, const Field<Type>& fld) const;


    // Housekeeping

        //- Cannot write geometry with a mesh reference.
        //- No beginGeometry() marker.
        virtual void write(ensightGeoFile&, const polyMesh&, bool) const
        {}
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#ifdef NoRepository
    #include "ensightOutputSurface.txx"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
