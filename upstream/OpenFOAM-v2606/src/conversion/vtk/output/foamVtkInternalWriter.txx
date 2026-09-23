/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2017-2026 OpenCFD Ltd.
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

#include "foamVtkInternalWriter.H"
#include "foamVtkOutput.H"
#include "volPointInterpolation.H"
#include "interpolatePointToCell.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

template<class Type, template<class> class PatchField>
void Foam::vtk::internalWriter::write
(
    const GeometricField<Type, PatchField, pointMesh>& field
)
{
    // Extra values corresponding to the decomposed cell centres
    const Field<Type> extra
    (
        interpolatePointToCell(field, vtuCells_.addPointCellLabels())
    );

    writePointData(field.name(), field, extra);
}


template<class Type>
void Foam::vtk::internalWriter::write
(
    const DimensionedField<Type, volMesh>& field
)
{
    writeCellData(field.name(), field.field());
}


template<class Type, template<class> class PatchField>
void Foam::vtk::internalWriter::write
(
    const GeometricField<Type, PatchField, volMesh>& field
)
{
    writeCellData(field.name(), field.primitiveField());
}


template<class Type>
void Foam::vtk::internalWriter::write
(
    const DimensionedField<Type, volMesh>& vfield,
    const volPointInterpolation& pInterp
)
{
    typedef DimensionedField<Type, pointMesh> PointFieldType;

    // Use tmp intermediate. Compiler sometimes weird otherwise.
    const tmp<PointFieldType> tfield = pInterp.interpolate(vfield);
    const auto& pfield = tfield();

    // Extra values corresponding to the decomposed cell centres
    const List<Type> extra
    (
        vfield.field(),
        vtuCells_.addPointCellLabels()
    );

    writePointData(vfield.name(), pfield, extra);
}


template<class Type>
void Foam::vtk::internalWriter::write
(
    const GeometricField<Type, fvPatchField, volMesh>& vfield,
    const volPointInterpolation& pInterp
)
{
    typedef GeometricField<Type, pointPatchField, pointMesh> PointFieldType;

    // Use tmp intermediate. Compiler sometimes weird otherwise.
    const tmp<PointFieldType> tfield = pInterp.interpolate(vfield);
    const auto& pfield = tfield();

    // Extra values corresponding to the decomposed cell centres
    const List<Type> extra
    (
        vfield.primitiveField(),
        vtuCells_.addPointCellLabels()
    );

    writePointData(vfield.name(), pfield, extra);
}


// ************************************************************************* //
