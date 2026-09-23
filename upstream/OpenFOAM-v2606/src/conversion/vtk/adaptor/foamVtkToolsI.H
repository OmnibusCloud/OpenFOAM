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

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

inline Foam::UList<uint8_t> Foam::vtk::Tools::asUList
(
    vtkUnsignedCharArray* array,
    const label size
)
{
    array->SetNumberOfComponents(1);
    array->SetNumberOfTuples(size);

    return UList<uint8_t>(array->WritePointer(0, size), size);
}


inline Foam::UList<vtkIdType> Foam::vtk::Tools::asUList
(
    vtkIdTypeArray* array,
    const label size
)
{
    array->SetNumberOfComponents(1);
    array->SetNumberOfTuples(size);

    return UList<vtkIdType>(array->WritePointer(0, size), size);
}


inline vtkSmartPointer<vtkPoints>
Foam::vtk::Tools::NewPoints
(
    vtkIdType count
)
{
    auto vtkpoints = vtkSmartPointer<vtkPoints>::New();
    // if (narrowing || (sizeof(float) == sizeof(Foam::scalar)))
    // {
    //     vtkpoints->SetDataTypeToFloat();
    // }
    // else
    // {
    //     vtkpoints->SetDataTypeToDouble();
    // }
    vtkpoints->SetNumberOfPoints(count);

    return vtkpoints;
}


inline vtkSmartPointer<vtkPoints>
Foam::vtk::Tools::Points
(
    const UList<point>& pts
)
{
    auto vtkpoints = vtk::Tools::NewPoints(pts.size());

    vtkIdType pointId = 0;
    for (const auto& p : pts)
    {
        vtkpoints->SetPoint(pointId++, p.cdata());
    }

    return vtkpoints;
}


inline vtkSmartPointer<vtkPoints>
Foam::vtk::Tools::Points
(
    const UList<point>& pts,
    const labelUList& addr
)
{
    auto vtkpoints = vtk::Tools::NewPoints(addr.size());

    vtkIdType pointId = 0;
    for (const auto pointi : addr)
    {
        const auto& p = pts[pointi];
        vtkpoints->SetPoint(pointId++, p.cdata());
    }

    return vtkpoints;
}


inline vtkSmartPointer<vtkPolyData>
Foam::vtk::Tools::Vertices
(
    const UList<point>& pts
)
{
    auto vtkmesh = vtkSmartPointer<vtkPolyData>::New();

    vtkmesh->SetPoints(Tools::Points(pts));
    vtkmesh->SetVerts(Tools::identityVertices(pts.size()));

    return vtkmesh;
}


inline vtkSmartPointer<vtkPolyData>
Foam::vtk::Tools::Vertices
(
    const UList<point>& pts,
    const labelUList& addr
)
{
    auto vtkmesh = vtkSmartPointer<vtkPolyData>::New();

    vtkmesh->SetPoints(Tools::Points(pts, addr));
    vtkmesh->SetVerts(Tools::identityVertices(addr.size()));

    return vtkmesh;
}


inline Foam::scalarMinMax Foam::vtk::Tools::rangeOf(vtkDataArray* data)
{
    double range[2]{GREAT, -GREAT};

    if (data)
    {
        if (data->GetNumberOfComponents() == 1)
        {
            data->GetRange(range);
        }
        else
        {
            // Mag
            data->GetRange(range, -1);
        }
    }

    return scalarMinMax(range[0], range[1]);
}


inline vtkSmartPointer<vtkCellArray>
Foam::vtk::Tools::identityVertices
(
    const vtkIdType size
)
{
    #ifndef VTK_CELL_ARRAY_V2
    #error "No support for defunct (VTK <= 8.2.0) vtkCellArray content"
    return vtkSmartPointer<vtkCellArray>::New();
    #else

    auto offsets = vtkSmartPointer<vtkIdTypeArray>::New();

    // Offsets
    // [0, n1, n1+n2, n1+n2+n3... ]
    {
        const vtkIdType nOffsets(size+1);

        offsets->SetNumberOfTuples(nOffsets);

        vtkIdType* iter = offsets->WritePointer(0, nOffsets);

        std::iota(iter, (iter + nOffsets), vtkIdType(0));
    }


    auto connect = vtkSmartPointer<vtkIdTypeArray>::New();

    // Connectivity
    {
        const vtkIdType nConnect(size);

        connect->SetNumberOfTuples(nConnect);

        vtkIdType* iter = connect->WritePointer(0, nConnect);

        std::iota(iter, (iter + nConnect), vtkIdType(0));
    }

    // Move into a vtkCellArray

    auto cells = vtkSmartPointer<vtkCellArray>::New();
    cells->SetData(offsets, connect);

    return cells;
    #endif
}


// ************************************************************************* //
