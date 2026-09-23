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

// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

inline vtkSmartPointer<vtkPoints>
Foam::vtk::vtuAdaptor::NewPoints
(
    vtkIdType count
) const
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
Foam::vtk::vtuAdaptor::points
(
    const polyMesh& mesh
) const
{
    // Convert OpenFOAM mesh vertices to VTK

    // Combine mesh points and any additional cellCentre points
    // into a single field

    const auto& pts = mesh.points();
    const auto& cc = mesh.cellCentres();

    // The additional cellCentre points
    const labelUList& addPoints = this->additionalIds();

    auto vtkpoints = NewPoints(pts.size() + addPoints.size());

    // Normal points
    vtkIdType pointId = 0;
    for (const auto& p : pts)
    {
        vtkpoints->SetPoint(pointId++, p.cdata());
    }

    // Cell centres
    for (const auto celli : addPoints)
    {
        const auto& p = cc[celli];
        vtkpoints->SetPoint(pointId++, p.cdata());
    }

    return vtkpoints;
}


inline vtkSmartPointer<vtkPoints>
Foam::vtk::vtuAdaptor::points
(
    const polyMesh& mesh,
    const labelUList& pointMap
) const
{
    // Convert OpenFOAM mesh vertices to VTK

    // Combine mapped points and additional cellCentre points
    // into a single field

    const auto& pts = mesh.points();
    const auto& cc = mesh.cellCentres();

    // The additional cellCentre points
    const labelUList& addPoints = this->additionalIds();

    auto vtkpoints = NewPoints(pointMap.size() + addPoints.size());

    // Normal points
    vtkIdType pointId = 0;
    for (const auto pointi : pointMap)
    {
        const auto& p = pts[pointi];
        vtkpoints->SetPoint(pointId++, p.cdata());
    }

    // Cell centres
    for (const auto celli : addPoints)
    {
        const auto& p = cc[celli];
        vtkpoints->SetPoint(pointId++, p.cdata());
    }

    return vtkpoints;
}


inline vtkSmartPointer<vtkUnstructuredGrid>
Foam::vtk::vtuAdaptor::internal
(
    const polyMesh& mesh,
    const bool decompPoly
)
{
    #ifndef VTK_CELL_ARRAY_V2
    #error "No support for defunct (VTK <= 8.2.0) vtkCellArray content"
    return vtkSmartPointer<vtkUnstructuredGrid>::New();
    #else

    constexpr vtk::vtuSizing::contentType output =
        vtk::vtuSizing::contentType::INTERNAL2;

    vtk::vtuSizing sizing(mesh, decompPoly);

    auto vtkmesh = vtkSmartPointer<vtkUnstructuredGrid>::New();

    auto cellTypes = vtkSmartPointer<vtkUnsignedCharArray>::New();

    UList<uint8_t> cellTypesUL
    (
        vtk::Tools::asUList(cellTypes, sizing.nFieldCells())
    );

    auto cells = vtkSmartPointer<vtkCellArray>::New();
    auto faces = vtkSmartPointer<vtkIdTypeArray>::New();

    auto cellOffsets   = vtkSmartPointer<vtkIdTypeArray>::New();
    auto faceLocations = vtkSmartPointer<vtkIdTypeArray>::New();

    const auto nConnect
    (
        sizing.sizeOf(output, vtk::vtuSizing::slotType::CELLS)
    );

    UList<vtkIdType> cellOffsetsUL
    (
        vtk::Tools::asUList
        (
            cellOffsets,
            sizing.sizeOf(output, vtk::vtuSizing::slotType::CELLS_OFFSETS)
        )
    );

    auto cellConnect = vtkSmartPointer<vtkIdTypeArray>::New();

    UList<vtkIdType> cellsUL
    (
        vtk::Tools::asUList(cellConnect, nConnect)
    );

    UList<vtkIdType> facesUL
    (
        vtk::Tools::asUList
        (
            faces,
            sizing.sizeOf(output, vtk::vtuSizing::slotType::FACES)
        )
    );

    UList<vtkIdType> faceLocationsUL
    (
        vtk::Tools::asUList
        (
            faceLocations,
            sizing.sizeOf(output, vtk::vtuSizing::slotType::FACES_OFFSETS)
        )
    );


    sizing.populateInternal
    (
        mesh,
        cellTypesUL,
        cellsUL, cellOffsetsUL,
        facesUL, faceLocationsUL,
        static_cast<foamVtkMeshMaps&>(*this),
        output
    );


    // Convert OpenFOAM mesh vertices to VTK
    // - can only do this *after* populating the decompInfo with cell-ids
    //   for any additional points (ie, mesh cell-centres)
    vtkmesh->SetPoints(this->points(mesh));

    // Move into a vtkCellArray
    cells->SetData(cellOffsets, cellConnect);

    // FUTURE: after VTK-9.4.0
    //   (VTK_MAJOR_VERSION >= 9) && (VTK_NINOR_VERSION >= 4)
    //   SetPolyhedralCells()

    if (facesUL.size())
    {
        vtkmesh->SetCells(cellTypes, cells, faceLocations, faces);
    }
    else
    {
        vtkmesh->SetCells(cellTypes, cells, nullptr, nullptr);
    }

    return vtkmesh;
    #endif
}


// ************************************************************************* //
