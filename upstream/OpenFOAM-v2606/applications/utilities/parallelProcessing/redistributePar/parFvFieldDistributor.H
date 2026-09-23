/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2015 OpenFOAM Foundation
    Copyright (C) 2018-2026 OpenCFD Ltd.
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
    Foam::parFvFieldDistributor

Description
    Finite volume reconstructor for volume and surface fields.

    Runs in parallel.
    Reconstructs/redistributes from procMesh to baseMesh.
    baseMesh is non-zero cells on processor0 only.

SourceFiles
    parFvFieldDistributor.cxx
    parFvFieldDistributor.txx
    parFvFieldDistributorFields.cxx

\*---------------------------------------------------------------------------*/

#ifndef Foam_parFvFieldDistributor_H
#define Foam_parFvFieldDistributor_H

#include "fvMesh.H"
#include "PtrList.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// Forward Declarations
class mapDistributePolyMesh;
class mapDistributeBase;
class IOobjectList;

/*---------------------------------------------------------------------------*\
                    Class parFvFieldDistributor Declaration
\*---------------------------------------------------------------------------*/

class parFvFieldDistributor
{
    // Private Data

        //- Processor mesh reference (source mesh)
        const fvMesh& srcMesh_;

        //- Destination mesh reference (eg, reconstructed mesh)
        fvMesh& tgtMesh_;

        //- Distribution map reference
        const mapDistributePolyMesh& distMap_;

        //- Patch mappers
        PtrList<mapDistributeBase> patchFaceMaps_;

        //- Storage for unused handler (when using bool control)
        const refPtr<fileOperation> noHandler_;

        //- Write control via a file handler
        const refPtr<fileOperation>& writeHandler_;

        //- Write control as a bool (instead of via file handler)
        bool isWriteProc_;


    // Private Member Functions

        //- Construct per-patch addressing
        void createPatchFaceMaps();

        //- Write using writeHandler_ or isWriteProc_ to guide the output
        template<class FieldType>
        void writeField(const FieldType& fld) const;


public:

    //- Output verbosity when writing
    static int verbose_;


    // Generated Methods

        //- No copy construct
        parFvFieldDistributor(const parFvFieldDistributor&) = delete;

        //- No copy assignment
        void operator=(const parFvFieldDistributor&) = delete;


    // Constructors

        //- Construct from components
        parFvFieldDistributor
        (
            //! The source mesh (eg, processor)
            const fvMesh& srcMesh,
            //! The target mesh (eg, reconstructed)
            fvMesh& tgtMesh,
            //! The distribution map
            const mapDistributePolyMesh& distMap,
            //! Tagged for output writing (on this proc)
            const bool isWriteProc = true
        );

        //- Construct from components
        parFvFieldDistributor
        (
            //! The source mesh (eg, processor)
            const fvMesh& srcMesh,
            //! The target mesh (eg, reconstructed)
            fvMesh& tgtMesh,
            //! The distribution map
            const mapDistributePolyMesh& distMap,
            //! Handler for output writing
            const refPtr<fileOperation>& writeHandler
        );


    // Member Functions

        //- The source mesh
        const fvMesh& sourceMesh() const noexcept { return srcMesh_; }

        //- Helper: reconstruct and write mesh points
        //  (note: should be moved to something like processorMeshes class)
        void reconstructPoints();

        //- Distribute all fields for known field types
        void distributeAllFields
        (
            const IOobjectList& objects,
            const wordRes& selectedFields
        ) const;

        //- Redistribute volume internal field
        template<class Type>
        tmp<DimensionedField<Type, volMesh>>
        distributeField
        (
            const DimensionedField<Type, volMesh>&
        ) const;

        //- Read and distribute volume internal field
        template<class Type>
        tmp<DimensionedField<Type, volMesh>>
        distributeInternalField(const IOobject& fieldObject) const;


        //- Redistribute volume field
        template<class Type>
        tmp<GeometricField<Type, fvPatchField, volMesh>>
        distributeField
        (
            const GeometricField<Type, fvPatchField, volMesh>& fld
        ) const;

        //- Read and distribute volume field
        template<class Type>
        tmp<GeometricField<Type, fvPatchField, volMesh>>
        distributeVolumeField(const IOobject& fieldObject) const;


        //- Redistribute surface field
        template<class Type>
        tmp<GeometricField<Type, fvsPatchField, surfaceMesh>>
        distributeField
        (
            const GeometricField<Type, fvsPatchField, surfaceMesh>&
        ) const;

        //- Read and distribute surface field
        template<class Type>
        tmp<GeometricField<Type, fvsPatchField, surfaceMesh>>
        distributeSurfaceField(const IOobject& fieldObject) const;


        //- Read, redistribute and write all/selected volume internal fields
        template<class Type>
        label distributeInternalFields
        (
            const IOobjectList& objects,
            const wordRes& selectedFields = wordRes()
        ) const;

        //- Read, redistribute and write all/selected volume fields
        template<class Type>
        label distributeVolumeFields
        (
            const IOobjectList& objects,
            const wordRes& selectedFields = wordRes()
        ) const;

        //- Read, reconstruct and write all/selected surface fields
        template<class Type>
        label distributeSurfaceFields
        (
            const IOobjectList& objects,
            const wordRes& selectedFields = wordRes()
        ) const;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#ifdef NoRepository
#   include "parFvFieldDistributor.txx"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
