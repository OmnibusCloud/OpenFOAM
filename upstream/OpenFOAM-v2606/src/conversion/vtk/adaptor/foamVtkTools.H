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

Namespace
    Foam::vtk::Tools

Description
    A collection of static methods to assist converting OpenFOAM data
    structures into VTK internal data structures.

    Remapping of the symmTensor order is required in input or output
    directions. OpenFOAM uses (XX, XY, XZ, YY, YZ, ZZ) order,
    VTK uses (XX, YY, ZZ, XY, YZ, XZ) order.

Note
    This file is implemented as headers-only.

SourceFiles
    foamVtkToolsI.H
    foamVtkTools.txx

\*---------------------------------------------------------------------------*/

#ifndef Foam_vtk_vtkTools_H
#define Foam_vtk_vtkTools_H

#include "faceList.H"
#include "pointField.H"
#include "symmTensor.H"
#include "MinMax.H"
#include "foamVtkCore.H"
#undef Log  // Avoid potential conflict with vtkLog...

// VTK includes
#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkSmartPointer.h"
#include "vtkUnsignedCharArray.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

// Forward Declarations
class vtkDataSet;
class vtkCellData;
class vtkPointData;

namespace Foam
{
namespace vtk
{

/*---------------------------------------------------------------------------*\
                        Class vtk::Caching Declaration
\*---------------------------------------------------------------------------*/

//- Bookkeeping for internal caching.
//  Retain an original copy of the geometry as well as a shallow copy
//  with the output fields.
//  The original copy is reused for different timesteps etc.
template<class DataType>
struct Caching
{
    //- The VTK geometry data type
    typedef DataType dataType;

    //- The geometry, without any cell/point data
    vtkSmartPointer<dataType> vtkgeom;

    //- The shallow-copy of geometry, plus additional data
    vtkSmartPointer<dataType> dataset;

    //- Number of points associated with the geometry
    inline uint64_t nPoints() const
    {
        return vtkgeom ? vtkgeom->GetNumberOfPoints() : 0;
    }

    //- Clear geometry and dataset
    void clearGeom()
    {
        vtkgeom = nullptr;
        dataset = nullptr;
    }

    //- Return a shallow copy of vtkgeom for manipulation
    vtkSmartPointer<dataType> getCopy() const
    {
        auto copy = vtkSmartPointer<dataType>::New();
        if (vtkgeom)
        {
            copy->ShallowCopy(vtkgeom);
        }
        return copy;
    }

    //- Make a shallow copy of vtkgeom into dataset
    void reuse()
    {
        dataset = vtkSmartPointer<dataType>::New();
        if (vtkgeom)
        {
            dataset->ShallowCopy(vtkgeom);
        }
    }

    //- Set the geometry and make a shallow copy to dataset
    void set(vtkSmartPointer<dataType> geom)
    {
        vtkgeom = geom;
        reuse();
    }

    //- Report basic information to output
    void PrintSelf(std::ostream& os) const
    {
        os << "geom" << nl;
        if (vtkgeom)
        {
            vtkgeom->PrintSelf(std::cout, vtkIndent(2));
        }
        else
        {
            os << "nullptr";
        }
        os << nl;

        os << "copy" << nl;
        if (dataset)
        {
            dataset->PrintSelf(std::cout, vtkIndent(2));
        }
        else
        {
            os << "nullptr";
        }
        os << nl;
    }
};


/*---------------------------------------------------------------------------*\
                             Namespace vtk::Tools
\*---------------------------------------------------------------------------*/

namespace Tools
{
    //- Wrap vtkUnsignedCharArray as a UList
    inline UList<uint8_t> asUList
    (
        vtkUnsignedCharArray* array,
        const label size
    );

    //- Wrap vtkIdTypeArray as a UList
    inline UList<vtkIdType> asUList
    (
        vtkIdTypeArray* array,
        const label size
    );

    //- Allocate vtkPoints
    inline vtkSmartPointer<vtkPoints>
    NewPoints(vtkIdType count);

    //- Return a list of points as vtkPoints
    inline vtkSmartPointer<vtkPoints>
    Points(const UList<point>& pts);

    //- Return an indirect list of points as vtkPoints
    inline vtkSmartPointer<vtkPoints>
    Points
    (
        const UList<point>& pts,
        const labelUList& addr
    );

    //- Convert a list of faces (or triFaces) to vtk polygon cells
    template<class Face>
    vtkSmartPointer<vtkCellArray>
    Faces(const UList<Face>& faces);

    //- Return vtkPolyData of vertices for each point
    inline vtkSmartPointer<vtkPolyData>
    Vertices
    (
        const UList<point>& pts
    );

    //- Return vtkPolyData of vertices for each point
    inline vtkSmartPointer<vtkPolyData>
    Vertices
    (
        const UList<point>& pts,
        const labelUList& addr
    );

    //- Min/Max of scalar, or mag() of non-scalars. Includes nullptr check.
    inline scalarMinMax rangeOf(vtkDataArray* data);


    //- Convert OpenFOAM patch to vtkPolyData
    struct Patch
    {
        //- Return local patch points as vtkPoints
        template<class PatchType>
        static vtkSmartPointer<vtkPoints>
        points(const PatchType& p);

        //- Convert patch faces to vtk polygon cells
        template<class PatchType>
        static vtkSmartPointer<vtkCellArray>
        faces(const PatchType& p);

        //- Convert patch points/faces to vtkPolyData
        template<class PatchType>
        static vtkSmartPointer<vtkPolyData>
        mesh(const PatchType& p);

        //- Convert patch face normals to vtkFloatArray
        template<class PatchType>
        static vtkSmartPointer<vtkFloatArray>
        faceNormals(const PatchType& p);

        //- Return patch face centres as vtkPoints
        template<class PatchType>
        static vtkSmartPointer<vtkPoints>
        faceCentres(const PatchType& p);

        //- Convert points/faces component to vtkPolyData
        template<class Face>
        static vtkSmartPointer<vtkPolyData>
        mesh
        (
            const UList<point>& pts,
            const UList<Face>& fcs
        );
    };


    //- Older name for copyTuple
    template<class Type>
    void foamToVtkTuple(float output[], const Type& value)
    {
        vtk::Tools::copyTuple(output, value);
    }

    //- Older name for copyTuple
    template<class Type>
    void foamToVtkTuple(double output[], const Type& value)
    {
        vtk::Tools::copyTuple(output, value);
    }


    // Field Conversion Functions

    //- Copy list to pre-allocated vtk array.
    //  \return number of input items copied
    template<class Type>
    label transcribeFloatData
    (
        //! [out] destination (vtkFloatArray or vtkDoubleArray)
        //! can also be a nullptr (no-op)
        vtkDataArray* array,
        const UList<Type>& input,
        vtkIdType start = 0         //!< The write offset into output array
    );

    //- Create named field initialized to zero
    template<class Type, class DataArrayType = vtkFloatArray>
    vtkSmartPointer<DataArrayType>
    zeroField
    (
        const word& name,
        const label size
    );

    //- Convert field data to a vtkFloatArray
    template<class Type, class DataArrayType = vtkFloatArray>
    vtkSmartPointer<DataArrayType>
    convertFieldToVTK
    (
        const word& name,
        const UList<Type>& fld
    );

    //- Convert field data to a vtkFloatArray, using indirect addressing
    template<class Type, class DataArrayType = vtkFloatArray>
    vtkSmartPointer<DataArrayType>
    convertFieldToVTK
    (
        const word& name,
        const UList<Type>& fld,
        const labelUList& addr
    );

    //- An identity list of VTK_VERTEX
    inline vtkSmartPointer<vtkCellArray>
    identityVertices(const vtkIdType size);

} // End namespace Tools

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace vtk
} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#include "foamVtkToolsI.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#ifdef NoRepository
    #include "foamVtkTools.txx"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
