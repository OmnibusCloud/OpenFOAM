/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2016-2025 OpenCFD Ltd.
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
    Foam::vtk::patchMeshWriter

Description
    Write OpenFOAM patches and patch fields in VTP or legacy vtk format.

    The file output states are managed by the Foam::vtk::fileWriter class.
    FieldData (eg, TimeValue) must appear before any geometry pieces.

Note
    Parallel output is combined into a single Piece without point merging,
    which is similar to using multi-piece data sets, but allows more
    convenient creation as a streaming process.
    In the future, the duplicate points at processor connections
    may be addressed using ghost points.

See Also
    Foam::vtk::patchWriter

SourceFiles
    foamVtkPatchMeshWriter.cxx
    foamVtkPatchMeshWriter.txx

\*---------------------------------------------------------------------------*/

#ifndef Foam_vtk_PatchMeshWriter_H
#define Foam_vtk_PatchMeshWriter_H

#include "foamVtkFileWriter.H"
#include "polyMesh.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace vtk
{

/*---------------------------------------------------------------------------*\
                    Class vtk::patchMeshWriter Declaration
\*---------------------------------------------------------------------------*/

class patchMeshWriter
:
    public vtk::fileWriter
{
protected:

    // Protected Member Data

        //- Reference to the OpenFOAM mesh (or subset)
        const polyMesh& mesh_;

        //- The selected patch ids
        labelList patchIDs_;

        //- Slab addressing for field points of the current Piece
        OffsetRange<label> pointSlab_;

        //- Slab addressing for field cells (faces) for the current Piece
        OffsetRange<label> cellSlab_;

        //- Slab addressing for cell (face) connectivity count,
        //- which is just the sum of the face sizes.
        OffsetRange<label> connectivitySlab_;


    // Protected Member Functions

        //- Total number of field points for the current Piece
        label nTotalPoints() const noexcept { return pointSlab_.total(); }

        //- Total number of field cells (faces) for the current Piece
        label nTotalCells() const noexcept { return cellSlab_.total(); }

        //- Determine sizes and begin piece.
        void beginPiece();

        //- Write patch points
        void writePoints();

        //- Write patch faces, legacy format
        void writePolys_legacy();

        //- Write patch faces
        void writePolys();

public:

    // Generated Methods

        //- No copy construct
        patchMeshWriter(const patchMeshWriter&) = delete;

        //- No copy assignment
        void operator=(const patchMeshWriter&) = delete;


    // Constructors

        //- Construct from components (default format INLINE_BASE64)
        patchMeshWriter
        (
            const polyMesh& mesh,
            const labelUList& patchIDs,
            const vtk::outputOptions opts = vtk::formatType::INLINE_BASE64
        );

        //- Construct from components (default format INLINE_BASE64),
        //- and open the file for writing.
        //  The file name is with/without an extension.
        patchMeshWriter
        (
            const polyMesh& mesh,
            const labelUList& patchIDs,
            const fileName& file,
            bool parallel = UPstream::parRun()
        );

        //- Construct from components and open the file for writing.
        //  The file name is with/without an extension.
        patchMeshWriter
        (
            const polyMesh& mesh,
            const labelUList& patchIDs,
            const vtk::outputOptions opts,
            const fileName& file,
            bool parallel = UPstream::parRun()
        );


    //- Destructor
    virtual ~patchMeshWriter() = default;


    // Member Functions

        //- File extension for current format type.
        using vtk::fileWriter::ext;

        //- File extension for given output type
        inline static word ext(vtk::outputOptions opts)
        {
            return opts.ext(vtk::fileTag::POLY_DATA);
        }

        //- The patch IDs
        const labelList& patchIDs() const noexcept
        {
            return patchIDs_;
        }

        //- Write file header (non-collective)
        //  \note Expected calling states: (OPENED).
        virtual bool beginFile(std::string title = "");

        //- Write patch topology
        //  Also writes the file header if not previously written.
        //  \note Must be called prior to writing CellData or PointData
        virtual bool writeGeometry();

        //- Begin CellData output section for specified number of fields.
        //  Must be called prior to writing any cell data fields.
        //  \param nFields is for legacy format only.
        //      When nFields=0, this a no-op for legacy format.
        //  \note Expected calling states: (PIECE | POINT_DATA).
        //
        //  \return True if the state changed
        virtual bool beginCellData(label nFields = 0);

        //- Begin PointData for specified number of fields.
        //  Must be called prior to writing any point data fields.
        //  \param nFields is for legacy format only.
        //      When nFields=0, this a no-op for legacy format.
        //  \note Expected calling states: (PIECE | CELL_DATA).
        //
        //  \return True if the state changed
        virtual bool beginPointData(label nFields = 0);


        //- Write patch ids as CellData.
        //  Must be called within the CELL_DATA state.
        void writePatchIDs();

        //- Write processor ids for each line as CellData or for each point
        //- as PointData, depending on isPointData() state. No-op in serial.
        bool writeProcIDs();

        //- Write processor neighbour ids as CellData. This is no-op in serial.
        //  Must be called within the CELL_DATA state.
        bool writeNeighIDs();


    // Write

        //- Write a uniform field of Cell (Face) or Point values
        template<class Type>
        void writeUniform(const word& fieldName, const Type& val);
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace vtk
} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#ifdef NoRepository
    #include "foamVtkPatchMeshWriter.txx"
#endif


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
