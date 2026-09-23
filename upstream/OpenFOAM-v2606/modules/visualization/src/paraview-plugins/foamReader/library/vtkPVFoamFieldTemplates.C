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

#ifndef Foam_vtkPVFoamFieldTemplates_C
#define Foam_vtkPVFoamFieldTemplates_C

// OpenFOAM includes
#include "error.H"
#include "emptyFvPatchField.H"
#include "wallPolyPatch.H"
#include "faceSet.H"
#include "volPointInterpolation.H"
#include "interpolatePointToCell.H"
#include "areaFaMesh.H"
#include "areaFields.H"
#undef Log  // Avoid potential conflict with vtkLog...

// VTK includes
#include <vtkFloatArray.h>
#include <vtkDoubleArray.h>
#include <vtkCellData.h>
#include <vtkPointData.h>
#include <vtkSmartPointer.h>

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
//
// volume-fields
//

template<class Type>
void Foam::vtkPVFoam::convertVolField
(
    const UPtrList<patchInterpolator>& patchInterpList,
    const GeometricField<Type, fvPatchField, volMesh>& fld
)
{
    const fvMesh& mesh = fld.mesh();
    const polyBoundaryMesh& pbm = mesh.boundaryMesh();

    const bool interpField = !patchInterpList.empty();
    const bool extrapPatch = reader_->GetExtrapolatePatches();

    // Interpolated field (demand driven)
    autoPtr<GeometricField<Type, pointPatchField, pointMesh>> ptfPtr;
    if (interpField)
    {
        DebugInfo << "convertVolField interpolating:" << fld.name() << nl;

        ptfPtr.reset
        (
            volPointInterpolation::New(mesh).interpolate(fld).ptr()
        );
    }

    convertVolFieldBlock(fld, ptfPtr, rangeVolume_);    // internalMesh
    convertVolFieldBlock(fld, ptfPtr, rangeCellZones_); // cellZones
    convertVolFieldBlock(fld, ptfPtr, rangeCellSets_);  // cellSets

    // Patches - currently skip field conversion for groups
    for
    (
        const auto partId
      : rangePatches_.intersection(selectedPartIds_)
    )
    {
        const auto& longName = selectedPartIds_[partId];

        auto iter = cachedVtp_.find(longName);
        if (!iter.good() || !iter.val().dataset)
        {
            // Should not happen, but for safety require a vtk geometry
            continue;
        }

        foamVtpData& vtpData = iter.val();
        auto dataset = vtpData.dataset;

        const labelUList& patchIds = vtpData.additionalIds();

        if (patchIds.empty())
        {
            continue;
        }


        // This is slightly roundabout, but we deal with groups and with
        // single patches.
        // For groups (spanning several patches) it is fairly messy to
        // get interpolated point fields. We would need to create a indirect
        // patch each time to obtain the mesh points. We thus skip that.
        //
        // To improve code reuse, we allocate the CellData as a zeroed-field
        // ahead of time.

        // The interpolated point values (single patch only)
        tmp<Field<Type>> tinterpolated;

        // The patch interpolator (single patch only)
        const patchInterpolator* patchInterp = nullptr;
        if (patchIds.size() == 1)
        {
            patchInterp = patchInterpList.get(patchIds[0]);
        }


        vtkSmartPointer<vtkFloatArray> cdata = vtk::Tools::zeroField<Type>
        (
            fld.name(),
            dataset->GetNumberOfPolys()
        );

        label coffset = 0; // the write offset into cell-data
        for (label patchId : patchIds)
        {
            const fvPatchField<Type>& ptf = fld.boundaryField()[patchId];

            // Normally just use the patch values directly
            tmp<Field<Type>> tpfld(ptf);

            if
            (
                isType<emptyFvPatchField<Type>>(ptf)
             ||
                (
                    extrapPatch
                 && !polyPatch::constraintType(pbm[patchId].type())
                )
            )
            {
                // Special cases, or explicitly requested to extrapolate
                // the internal field.

                // Start with a plain, unconstrained fvPatch
                fvPatch p(ptf.patch().patch(), mesh.boundary());

                tpfld = p.patchInternalField(fld);
            }


            // Process the patch values (regular or extrapolated)

            coffset +=
                vtk::Tools::transcribeFloatData
                (
                    cdata, tpfld(), coffset
                );

            // Interpolate the point field
            if (patchInterp)
            {
                tinterpolated = patchInterp->faceToPointInterpolate(tpfld);
            }
        }

        // Have (single-patch) interpolated field - convert it
        if (tinterpolated.good())
        {
            dataset->GetPointData()->AddArray
            (
                vtk::Tools::convertFieldToVTK
                (
                    fld.name(),
                    tinterpolated()
                )
            );
        }

        if (cdata)
        {
            dataset->GetCellData()->AddArray(cdata);
        }
    }


    // Face Zones
    for
    (
        const auto partId
      : rangeFaceZones_.intersection(selectedPartIds_)
    )
    {
        const auto& longName = selectedPartIds_[partId];
        const word zoneName = getFoamName(longName);

        auto iter = cachedVtp_.find(longName);
        if (!iter.good() || !iter.val().dataset)
        {
            // Should not happen, but for safety require a vtk geometry
            continue;
        }

        foamVtpData& vtpData = iter.val();
        auto dataset = vtpData.dataset;

        const faceZoneMesh& zMesh = mesh.faceZones();
        const label zoneId = zMesh.findZoneID(zoneName);

        if (zoneId < 0)
        {
            continue;
        }

        vtkSmartPointer<vtkFloatArray> cdata =
            convertFaceFieldToVTK
            (
                fld,
                zMesh[zoneId]
            );

        dataset->GetCellData()->AddArray(cdata);

        // TODO: point data
    }


    // Face Sets
    for
    (
        const auto partId
      : rangeFaceSets_.intersection(selectedPartIds_)
    )
    {
        const auto& longName = selectedPartIds_[partId];
        const word selectName = getFoamName(longName);

        auto iter = cachedVtp_.find(longName);
        if (!iter.good() || !iter.val().dataset)
        {
            // Should not happen, but for safety require a vtk geometry
            continue;
        }
        foamVtpData& vtpData = iter.val();
        auto dataset = vtpData.dataset;

        vtkSmartPointer<vtkFloatArray> cdata = convertFaceFieldToVTK
        (
            fld,
            vtpData.cellMap()
        );

        dataset->GetCellData()->AddArray(cdata);

        // TODO: point data
    }
}


template<class Type>
void Foam::vtkPVFoam::convertVolFields
(
    const fvMesh& mesh,
    const UPtrList<patchInterpolator>& patchInterpList,
    const IOobjectList& objects
)
{
    typedef GeometricField<Type, fvPatchField, volMesh> FieldType;

    forAllConstIters(objects, iter)
    {
        // Restrict to GeometricField<Type, ...>
        const auto& ioobj = *(iter.val());

        if (!ioobj.isHeaderClass<FieldType>())
        {
            continue;
        }

        // Throw FatalError, FatalIOError as exceptions
        const bool throwingError = FatalError.throwExceptions();
        const bool throwingIOerr = FatalIOError.throwExceptions();

        try
        {
            // Load field
            FieldType fld(ioobj, mesh);

            // Convert
            convertVolField(patchInterpList, fld);
        }
        catch (const Foam::IOerror& ioErr)
        {
            ioErr.write(Warning, false);
            Info<< nl << endl;
        }
        catch (const Foam::error& err)
        {
            // Bit of trickery to get the original message
            err.write(Warning, false);
            Info<< nl << endl;
        }

        // Restore previous exception throwing state
        FatalError.throwExceptions(throwingError);
        FatalIOError.throwExceptions(throwingIOerr);
    }
}


template<class Type>
void Foam::vtkPVFoam::convertDimFields
(
    const fvMesh& mesh,
    const UPtrList<patchInterpolator>& patchInterpList,
    const IOobjectList& objects
)
{
    typedef DimensionedField<Type, volMesh> FieldType;
    typedef GeometricField<Type, fvPatchField, volMesh> VolFieldType;

    forAllConstIters(objects, iter)
    {
        // Restrict to DimensionedField<Type, ...>
        const auto& ioobj = *(iter.val());

        if (!ioobj.isHeaderClass<FieldType>())
        {
            continue;
        }

        // Throw FatalError, FatalIOError as exceptions
        const bool throwingError = FatalError.throwExceptions();
        const bool throwingIOerr = FatalIOError.throwExceptions();

        try
        {
            // Load field
            FieldType dimFld(ioobj, mesh);

            // Construct volField with zero-gradient patch fields

            IOobject io(dimFld);
            io.readOpt() = IOobjectOption::NO_READ;

            PtrList<fvPatchField<Type>> patchFields(mesh.boundary().size());
            forAll(patchFields, patchI)
            {
                patchFields.set
                (
                    patchI,
                    fvPatchField<Type>::New
                    (
                        fvPatchFieldBase::zeroGradientType(),
                        mesh.boundary()[patchI],
                        dimFld
                    )
                );
            }

            VolFieldType volFld
            (
                io,
                dimFld.mesh(),
                dimFld.dimensions(),
                dimFld,
                patchFields
            );
            volFld.correctBoundaryConditions();

            convertVolField(patchInterpList, volFld);
        }
        catch (const Foam::IOerror& ioErr)
        {
            ioErr.write(Warning, false);
            Info<< nl << endl;
        }
        catch (const Foam::error& err)
        {
            // Bit of trickery to get the original message
            err.write(Warning, false);
            Info<< nl << endl;
        }

        // Restore previous exception throwing state
        FatalError.throwExceptions(throwingError);
        FatalIOError.throwExceptions(throwingIOerr);
    }
}


template<class Type>
void Foam::vtkPVFoam::convertVolFieldBlock
(
    const GeometricField<Type, fvPatchField, volMesh>& fld,
    autoPtr<GeometricField<Type, pointPatchField, pointMesh>>& ptfPtr,
    const arrayRange& range
)
{
    for
    (
        const auto partId
      : range.intersection(selectedPartIds_)
    )
    {
        const auto& longName = selectedPartIds_[partId];

        auto iter = cachedVtu_.find(longName);
        if (!iter.good() || !iter.val().dataset)
        {
            // Should not happen, but for safety require a vtk geometry
            continue;
        }

        foamVtuData& vtuData = iter.val();
        auto dataset = vtuData.dataset;

        dataset->GetCellData()->AddArray
        (
            vtuData.convertField(fld)
        );

        if (ptfPtr)
        {
            dataset->GetPointData()->AddArray
            (
                convertPointField(*ptfPtr, fld, vtuData)
            );
        }
    }
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
//
// area-fields
//

template<class Type>
void Foam::vtkPVFoam::convertAreaFields
(
    const faMesh& mesh,
    const IOobjectList& objects,
    const uindPatchInterpolator* patchInterp
)
{
    typedef GeometricField<Type, faPatchField, areaMesh> FieldType;

    const List<label> partIds =
        rangeArea_.intersection(selectedPartIds_);

    if (partIds.empty())
    {
        return;
    }

    label areaMeshPartId = -1;

    // Some of this was already done by the caller
    // but here we also restrict to the corresponding vtp target
    for (const label partId : partIds)
    {
        const auto& longName = selectedPartIds_[partId];
        const word areaName = getFoamAreaName(longName);

        if (areaName == mesh.name())
        {
            // Selection corresponds to the target mesh
            areaMeshPartId = partId;
            break;
        }
    }

    if (areaMeshPartId >= 0)
    {
        const auto& longName = selectedPartIds_[areaMeshPartId];

        auto iter = cachedVtp_.find(longName);
        if (!iter.good() || !iter.val().dataset)
        {
            // Should not happen, but for safety require a vtk
            // geometry
            return;
        }

        foamVtpData& vtpData = iter.val();
        auto dataset = vtpData.dataset;

        forAllConstIters(objects, iter)
        {
            // Restrict to GeometricField<Type, ...>
            const auto& ioobj = *(iter.val());

            if (!ioobj.isHeaderClass<FieldType>())
            {
                continue;
            }
            const word& fieldName = ioobj.name();

            bool haveFieldValues = false;
            Field<Type> fieldValues;

            // Throw FatalError, FatalIOError as exceptions
            const bool throwingError = FatalError.throwExceptions();
            const bool throwingIOerr = FatalIOError.throwExceptions();

            try
            {
                // Load field
                FieldType fld(ioobj, mesh);

                fieldValues = std::move(fld.primitiveFieldRef());
                haveFieldValues = true;
            }
            catch (const Foam::IOerror& ioErr)
            {
                ioErr.write(Warning, false);
                Info<< nl << endl;
            }
            catch (const Foam::error& err)
            {
                // Bit of trickery to get the original message
                err.write(Warning, false);
                Info<< nl << endl;
            }

            // Restore previous exception throwing state
            FatalError.throwExceptions(throwingError);
            FatalIOError.throwExceptions(throwingIOerr);

            if (!haveFieldValues)
            {
                continue;
            }

            vtkSmartPointer<vtkFloatArray> cdata =
                vtk::Tools::convertFieldToVTK
                (
                    fieldName,
                    fieldValues
                );

            dataset->GetCellData()->AddArray(cdata);

            // Interpolate a point field
            // if (patchInterp)
            // {
            //     tinterpolated =
            //     patchInterp->faceToPointInterpolate(fieldValues);
            // }
        }
    }
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

//
// point-fields
//
template<class Type>
void Foam::vtkPVFoam::convertPointFields
(
    const pointMesh& pMesh,
    const IOobjectList& objects
)
{
    const polyMesh& mesh = pMesh.mesh();

    typedef GeometricField<Type, pointPatchField, pointMesh> FieldType;

    forAllConstIters(objects, iter)
    {
        // Restrict to this GeometricField<Type, ...>
        const auto& ioobj = *(iter.val());

        if (!ioobj.isHeaderClass<FieldType>())
        {
            continue;
        }

        const word& fieldName = ioobj.name();

        DebugInfo << "convertPointFields : " << fieldName << nl;

        // Throw FatalError, FatalIOError as exceptions
        const bool throwingError = FatalError.throwExceptions();
        const bool throwingIOerr = FatalIOError.throwExceptions();

        try
        {
            FieldType pfld(ioobj, pMesh);

            convertPointFieldBlock(pfld, rangeVolume_);     // internalMesh
            convertPointFieldBlock(pfld, rangeCellZones_);  // cellZones
            convertPointFieldBlock(pfld, rangeCellSets_);   // cellSets

            // Patches - currently skip field conversion for groups
            for
            (
                const auto partId
              : rangePatches_.intersection(selectedPartIds_)
            )
            {
                const auto& longName = selectedPartIds_[partId];

                auto iter = cachedVtp_.find(longName);
                if (!iter.good() || !iter.val().dataset)
                {
                    // Should not happen, but for safety require a vtk geometry
                    continue;
                }

                foamVtpData& vtpData = iter.val();
                auto dataset = vtpData.dataset;

                const labelUList& patchIds = vtpData.additionalIds();
                if (patchIds.size() != 1)
                {
                    continue;
                }

                const label patchId = patchIds[0];

                const auto& bfld = pfld.boundaryField()[patchId];

                tmp<Field<Type>> tfieldValues;

                vtkSmartPointer<vtkFloatArray> pdata;

                // Only valuePointPatchField is actually derived from Field
                if (const auto* vpp = isA<Field<Type>>(bfld))
                {
                    tfieldValues.cref(*vpp);
                }
                else
                {
                    tfieldValues = bfld.patchInternalField();
                }

                {
                    dataset->GetPointData()->AddArray
                    (
                        vtk::Tools::convertFieldToVTK<Type, vtkFloatArray>
                        (
                            fieldName,
                            tfieldValues()
                        )
                    );
                }
            }

            // Face Zones
            for
            (
                const auto partId
              : rangeFaceZones_.intersection(selectedPartIds_)
            )
            {
                const auto& longName = selectedPartIds_[partId];
                const word zoneName = getFoamName(longName);

                auto iter = cachedVtp_.find(longName);
                if (!iter.good() || !iter.val().dataset)
                {
                    // Should not happen, but for safety require a vtk geometry
                    continue;
                }

                foamVtpData& vtpData = iter.val();
                auto dataset = vtpData.dataset;

                const label zoneId = mesh.faceZones().findZoneID(zoneName);

                if (zoneId < 0)
                {
                    continue;
                }

                // Extract the field on the zone

                // CAVEAT - does not properly handle valuePointPatchField,
                // uses internalField only

                Field<Type> znfld
                (
                    pfld.primitiveField(),
                    mesh.faceZones()[zoneId]().meshPoints()
                );

                vtkSmartPointer<vtkFloatArray> pdata =
                    vtk::Tools::convertFieldToVTK
                    (
                        fieldName,
                        znfld
                    );

                dataset->GetPointData()->AddArray(pdata);
            }
        }
        catch (const Foam::IOerror& ioErr)
        {
            ioErr.write(Warning, false);
            Info<< nl << endl;
        }
        catch (const Foam::error& err)
        {
            // Bit of trickery to get the original message
            err.write(Warning, false);
            Info<< nl << endl;
        }

        // Restore previous exception throwing state
        FatalError.throwExceptions(throwingError);
        FatalIOError.throwExceptions(throwingIOerr);
    }
}


template<class Type>
void Foam::vtkPVFoam::convertPointFieldBlock
(
    const GeometricField<Type, pointPatchField, pointMesh>& pfld,
    const arrayRange& range
)
{
    for
    (
        const auto partId
      : range.intersection(selectedPartIds_)
    )
    {
        const auto& longName = selectedPartIds_[partId];

        auto iter = cachedVtu_.find(longName);
        if (!iter.good() || !iter.val().dataset)
        {
            // Should not happen, but for safety require a vtk geometry
            continue;
        }

        foamVtuData& vtuData = iter.val();
        auto dataset = vtuData.dataset;

        vtkSmartPointer<vtkFloatArray> pdata = convertPointField
        (
            pfld,
            GeometricField<Type, fvPatchField, volMesh>::null(),
            vtuData
        );

        dataset->GetPointData()->AddArray(pdata);
    }
}


template<class Type, class DataArrayType>
vtkSmartPointer<DataArrayType>
Foam::vtkPVFoam::convertPointField
(
    const GeometricField<Type, pointPatchField, pointMesh>& pfld,
    const GeometricField<Type, fvPatchField, volMesh>& vfld,
    const foamVtuData& vtuData
)
{
    const labelUList& addPointCellLabels = vtuData.additionalIds();
    const labelUList& pointMap = vtuData.pointMap();

    // Use a pointMap or address directly into mesh
    const label nPoints = (pointMap.size() ? pointMap.size() : pfld.size());

    auto data = vtkSmartPointer<DataArrayType>::New();
    data->SetNumberOfComponents(static_cast<int>(pTraits<Type>::nComponents));
    data->SetNumberOfTuples(nPoints + addPointCellLabels.size());

    // Note: using the name of the original volField
    // not the name generated by the interpolation "volPointInterpolate(<name>)"

    if (notNull(vfld))
    {
        data->SetName(vfld.name().c_str());
    }
    else
    {
        data->SetName(pfld.name().c_str());
    }

    DebugInfo
        << "Convert point field: " << pfld.name()
        << " size="  << (nPoints + addPointCellLabels.size())
        << " (" << nPoints << " + " << addPointCellLabels.size()
        << ") nComp=" << static_cast<int>(pTraits<Type>::nComponents) << nl;


    double work[pTraits<Type>::nComponents];

    vtkIdType pointId = 0;
    if (pointMap.size())
    {
        for (const label pointi : pointMap)
        {
            vtk::Tools::copyTuple(work, pfld[pointi]);
            data->SetTuple(pointId++, work);
        }
    }
    else
    {
        for (const Type& val : pfld)
        {
            vtk::Tools::copyTuple(work, val);
            data->SetTuple(pointId++, work);
        }
    }

    // Continue with additional points
    // - correspond to cell centres

    if (notNull(vfld))
    {
        for (const label celli : addPointCellLabels)
        {
            vtk::Tools::copyTuple(work, vfld[celli]);
            data->SetTuple(pointId++, work);
        }
    }
    else
    {
        for (const label celli : addPointCellLabels)
        {
            vtk::Tools::copyTuple
            (
                work,
                interpolatePointToCell(pfld, celli)
            );
            data->SetTuple(pointId++, work);
        }
    }

    return data;
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
//
// lagrangian-fields
//

template<class Type>
void Foam::vtkPVFoam::convertLagrangianFields
(
    const IOobjectList& objects,
    vtkPolyData* vtkmesh
)
{
    forAllConstIters(objects, iter)
    {
        // Restrict to IOField<Type>
        const auto& ioobj = *(iter.val());

        if (!ioobj.isHeaderClass<IOField<Type>>())
        {
            continue;
        }

        // Throw FatalError, FatalIOError as exceptions
        const bool throwingError = FatalError.throwExceptions();
        const bool throwingIOerr = FatalIOError.throwExceptions();

        try
        {
            IOField<Type> fld(ioobj);

            {
                vtkSmartPointer<vtkFloatArray> data =
                    vtk::Tools::convertFieldToVTK
                    (
                        ioobj.name(),
                        fld
                    );

                // Provide identical data as cell and as point data
                vtkmesh->GetCellData()->AddArray(data);
                vtkmesh->GetPointData()->AddArray(data);
            }
        }
        catch (const Foam::IOerror& ioErr)
        {
            ioErr.write(Warning, false);
            Info<< nl << endl;
        }
        catch (const Foam::error& err)
        {
            // Bit of trickery to get the original message
            err.write(Warning, false);
            Info<< nl << endl;
        }

        // Restore previous exception throwing state
        FatalError.throwExceptions(throwingError);
        FatalIOError.throwExceptions(throwingIOerr);
    }
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
//
// low-level conversions
//

template<class Type, class DataArrayType>
vtkSmartPointer<DataArrayType>
Foam::vtkPVFoam::convertFaceFieldToVTK
(
    const GeometricField<Type, fvPatchField, volMesh>& fld,
    const labelUList& faceLabels
) const
{
    DebugInfo
        << "Convert face field: " << fld.name()
        << " size=" << faceLabels.size()
        << " nComp=" << static_cast<int>(pTraits<Type>::nComponents) << nl;

    const fvMesh& mesh = fld.mesh();

    const label nInternalFaces = mesh.nInternalFaces();
    const labelUList& faceOwner = mesh.faceOwner();
    const labelUList& faceNeigh = mesh.faceNeighbour();

    auto data = vtkSmartPointer<DataArrayType>::New();
    data->SetName(fld.name().c_str());
    data->SetNumberOfComponents(static_cast<int>(pTraits<Type>::nComponents));
    data->SetNumberOfTuples(faceLabels.size());

    // Interior faces: average owner/neighbour
    // Boundary faces: the owner value

    double work[pTraits<Type>::nComponents];

    vtkIdType faceId = 0;
    for (const label meshFacei : faceLabels)
    {
        if (meshFacei < nInternalFaces)
        {
            Type val =
                0.5*(fld[faceOwner[meshFacei]] + fld[faceNeigh[meshFacei]]);

            vtk::Tools::copyTuple(work, val);
        }
        else
        {
            const Type& val = fld[faceOwner[meshFacei]];
            vtk::Tools::copyTuple(work, val);
        }

        data->SetTuple(faceId++, work);
    }

    return data;
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
