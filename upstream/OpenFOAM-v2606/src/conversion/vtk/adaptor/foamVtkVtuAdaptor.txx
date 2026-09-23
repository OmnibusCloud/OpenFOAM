/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2017-2025 OpenCFD Ltd.
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

// VTK includes
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"

// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class Type, class DataArrayType>
vtkSmartPointer<DataArrayType>
Foam::vtk::vtuAdaptor::convertField
(
    const DimensionedField<Type, volMesh>& fld
) const
{
    return vtk::Tools::convertFieldToVTK<Type, DataArrayType>
    (
        fld.name(),
        fld.field(),
        this->cellMap()
    );
}


template<class Type, class DataArrayType>
vtkSmartPointer<DataArrayType>
Foam::vtk::vtuAdaptor::convertField
(
    const GeometricField<Type, fvPatchField, volMesh>& fld
) const
{
    return vtk::Tools::convertFieldToVTK<Type, DataArrayType>
    (
        fld.name(),
        fld.primitiveField(),
        this->cellMap()
    );
}


// ************************************************************************* //
