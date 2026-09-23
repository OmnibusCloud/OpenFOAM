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

// OpenFOAM includes
#include "error.H"
#undef Log  // Avoid potential conflict with vtkLog...

// VTK includes
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

template<class Face>
vtkSmartPointer<vtkCellArray>
Foam::vtk::Tools::Faces(const UList<Face>& faces)
{
    #ifndef VTK_CELL_ARRAY_V2
    #error "No support for defunct (VTK <= 8.2.0) vtkCellArray content"
    return vtkSmartPointer<vtkCellArray>::New();
    #else

    // Offsets
    // [0, n1, n1+n2, n1+n2+n3... ]

    auto offsets = vtkSmartPointer<vtkIdTypeArray>::New();

    const vtkIdType nOffsets(faces.size()+1);

    vtkIdType nConnect(0);
    {
        offsets->SetNumberOfTuples(nOffsets);

        auto* iter = offsets->WritePointer(0, nOffsets);

        // Assign offsets, determine overall connectivity size

        *iter = 0;
        for (const auto& f : faces)
        {
            nConnect += f.size();

            *(++iter) = nConnect;
        }
    }


    // Cell connectivity for polygons
    // [verts..., verts... ]

    auto connect = vtkSmartPointer<vtkIdTypeArray>::New();

    {
        connect->SetNumberOfTuples(nConnect);

        auto* iter = connect->WritePointer(0, nConnect);

        // Fill in the connectivity array

        for (const auto& f : faces)
        {
            for (auto verti : f)
            {
                *(iter++) = verti;
            }
        }
    }

    // Move into a vtkCellArray

    auto cells = vtkSmartPointer<vtkCellArray>::New();

    cells->SetData(offsets, connect);

    return cells;
    #endif
}


template<class PatchType>
vtkSmartPointer<vtkPoints>
Foam::vtk::Tools::Patch::points(const PatchType& p)
{
    // Local patch points to vtkPoints
    return vtk::Tools::Points(p.localPoints());
}


template<class PatchType>
vtkSmartPointer<vtkCellArray>
Foam::vtk::Tools::Patch::faces(const PatchType& p)
{
    return vtk::Tools::Faces(p.localFaces());
}


template<class PatchType>
vtkSmartPointer<vtkPolyData>
Foam::vtk::Tools::Patch::mesh(const PatchType& p)
{
    auto vtkmesh = vtkSmartPointer<vtkPolyData>::New();

    vtkmesh->SetPoints(points(p));
    vtkmesh->SetPolys(faces(p));

    return vtkmesh;
}


template<class Face>
vtkSmartPointer<vtkPolyData>
Foam::vtk::Tools::Patch::mesh
(
    const UList<point>& pts,
    const UList<Face>& fcs
)
{
    auto vtkmesh = vtkSmartPointer<vtkPolyData>::New();

    vtkmesh->SetPoints(Tools::Points(pts));
    vtkmesh->SetPolys(Tools::Faces(fcs));

    return vtkmesh;
}


template<class PatchType>
vtkSmartPointer<vtkFloatArray>
Foam::vtk::Tools::Patch::faceNormals(const PatchType& p)
{
    auto array = vtkSmartPointer<vtkFloatArray>::New();

    array->SetNumberOfComponents(3);
    array->SetNumberOfTuples(p.size());

    // Unit normals for patch faces.
    // Cached values if available or loop over faces (avoid triggering cache)

    vtkIdType faceId = 0;

    if (p.hasFaceNormals())
    {
        for (const vector& n : p.faceNormals())
        {
            array->SetTuple(faceId++, n.cdata());
        }
    }
    else
    {
        for (const auto& f : p)
        {
            const vector n(f.unitNormal(p.points()));
            array->SetTuple(faceId++, n.cdata());
        }
    }

    return array;
}


template<class PatchType>
vtkSmartPointer<vtkPoints>
Foam::vtk::Tools::Patch::faceCentres(const PatchType& p)
{
    auto vtkpoints = vtk::Tools::NewPoints(p.size());

    // Use cached values if available or loop over faces
    // (avoid triggering cache)

    vtkIdType pointId = 0;

    if (p.hasFaceCentres())
    {
        for (const auto& pt : p.faceCentres())
        {
            vtkpoints->SetPoint(pointId++, pt.cdata());
        }
    }
    else
    {
        for (const auto& f : p)
        {
            const point pt(f.centre(p.points()));
            vtkpoints->SetPoint(pointId++, pt.cdata());
        }
    }

    return vtkpoints;
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
//
// Low-Level conversions
//

template<class Type>
Foam::label Foam::vtk::Tools::transcribeFloatData
(
    vtkDataArray* array,
    const UList<Type>& input,
    vtkIdType start
)
{
    if (!array)
    {
        // No destination : no-op
        return 0;
    }

    // Check for casting to vtkFloatArray or vtkDoubleArray ?

    if
    (
        array->GetNumberOfComponents()
     != static_cast<int>(pTraits<Type>::nComponents)
    )
    {
        FatalErrorInFunction
            << "vtk array '" << array->GetName()
            << "' has mismatch in number of components for type '"
            << pTraits<Type>::typeName
            << "' : target array has " << array->GetNumberOfComponents()
            << " components instead of "
            << static_cast<int>(pTraits<Type>::nComponents) << nl;
    }

    const vtkIdType maxSize = array->GetNumberOfTuples();

    if (!maxSize)
    {
        // no-op
        return 0;
    }
    else if (start < 0 || start >= maxSize)
    {
        WarningInFunction
            << "vtk array '" << array->GetName()
            << "' copy with out-of-range [0," << int64_t(maxSize) << ")"
            << " starting at " << int64_t(start)
            << nl;

        return 0;
    }
    else if
    (
        vtkIdType endPos = start+vtkIdType(input.size());
        endPos > maxSize
    )
    {
        WarningInFunction
            << "vtk array '" << array->GetName()
            << "' copy ends out-of-range (" << int64_t(maxSize) << ")"
            << " using sizing (start,size) = ("
            << int64_t(start) << "," << input.size() << ")"
            << nl;

        return 0;
    }

    double work[pTraits<Type>::nComponents];

    for (const Type& val : input)
    {
        vtk::Tools::copyTuple(work, val);
        array->SetTuple(start++, work);
    }

    return input.size();
}


template<class Type, class DataArrayType>
vtkSmartPointer<DataArrayType>
Foam::vtk::Tools::zeroField
(
    const word& name,
    const label size
)
{
    auto data = vtkSmartPointer<DataArrayType>::New();

    data->SetName(name.c_str());
    data->SetNumberOfComponents(static_cast<int>(pTraits<Type>::nComponents));
    data->SetNumberOfTuples(size);

    // Fill() was not available before VTK-8
    #if (VTK_MAJOR_VERSION < 8)
    for (int i = 0; i < data->GetNumberOfComponents(); ++i)
    {
        data->FillComponent(i, 0);
    }
    #else
    data->Fill(0);
    #endif

    return data;
}


template<class Type, class DataArrayType>
vtkSmartPointer<DataArrayType>
Foam::vtk::Tools::convertFieldToVTK
(
    const word& name,
    const UList<Type>& fld
)
{
    auto data = vtkSmartPointer<DataArrayType>::New();

    data->SetName(name.c_str());
    data->SetNumberOfComponents(static_cast<int>(pTraits<Type>::nComponents));
    data->SetNumberOfTuples(fld.size());


    // Transcribe data
    vtkIdType start = 0;

    double work[pTraits<Type>::nComponents];

    for (const Type& val : fld)
    {
        vtk::Tools::copyTuple(work, val);
        data->SetTuple(start++, work);
    }

    return data;
}


template<class Type, class DataArrayType>
vtkSmartPointer<DataArrayType>
Foam::vtk::Tools::convertFieldToVTK
(
    const word& name,
    const UList<Type>& fld,
    const labelUList& addr
)
{
    auto data = vtkSmartPointer<DataArrayType>::New();

    data->SetName(name.c_str());
    data->SetNumberOfComponents(static_cast<int>(pTraits<Type>::nComponents));
    data->SetNumberOfTuples(addr.size());


    // Transcribe data
    vtkIdType start = 0;

    double work[pTraits<Type>::nComponents];

    for (const label idx : addr)
    {
        vtk::Tools::copyTuple(work, fld[idx]);
        data->SetTuple(start++, work);
    }

    return data;
}


// ************************************************************************* //
